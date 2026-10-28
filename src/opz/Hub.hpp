#pragma once
#include "TrackCore.hpp"
#include "Project.hpp"

/*
The bus along a row of OP-Z modules: the Sequencer, which plays every track, then the breakouts that hand tracks to
other modules and the Screens that show the selected one. A module from another plugin ends it. To the Sequencer's
left, a Remotes module plays its patterns from jacks and a Song Editor edits its songs, each finding the hub by walking
right.

The Sequencer holds the hub. For each track it carries the jacks of the breakout bound to it both ways: the track's
outputs out to the breakout, and the breakout's inputs, and which of its outputs are patched, back in. Both are
double-buffered by Rack's frame count: in frame n each side writes the n & 1 copy and reads the other, written in
frame n - 1, and Rack finishes every module's frame before it starts the next, so no copy is ever written and read
at once, however it spreads modules over threads. A breakout's jacks are one sample behind the track, and what comes
back through its return two.

The UI thread reaches the tracks through the hub too, for a Screen and a breakout's status screen.
*/
namespace opz {

/** What a track sends out through its breakout: its outputs, those patched only. */
struct JacksOut {
	Port outputs[TrackIds::OUTPUTS_LEN];
	float activity = 0.f;        // its voices' loudest, for the breakout's NOTE light
	bool selected = false;
};

/** What a breakout sends its track: its inputs, and which of its outputs are patched. */
struct JacksIn {
	int64_t frame = -1;          // when it was written: an older one's breakout has since left the track
	uint32_t patched = 0;        // a bit per output
	Port inputs[TrackIds::INPUTS_LEN];
};

/** What a Remotes module to the Sequencer's left sends it: its inputs, as they are. */
struct RemoteIn {
	enum Jack { SELECT, NEXT, PREV, BAR, SONG, LOAD, MODE, JACKS };
	int64_t frame = -1;          // when it was written: an older one's Remotes has gone
	uint32_t patched = 0;        // a bit per jack
	int selectChannels = 0;
	float select[PORT_MAX_CHANNELS] = {};
	float volts[JACKS] = {};     // the other jacks' voltages
	float window = 0.005f;       // how late a trigger may come and still count as on time, in seconds
};

/** What the Sequencer sends a Remotes module: counts of the events its outputs pulse for, and the pattern playing. */
struct RemoteOut {
	uint32_t starts = 0, ends = 0, songEnds = 0;
	int pattern = 0;
};

/** What a Song Editor sends the Sequencer each frame: the row it has selected, and what its controls did since its last
frame. Row -1 is the song's settings above the rows, whose fade in, fade out and tempo the column wheels turn. */
struct SongIn {
	static constexpr int SETTINGS = -1;
	int64_t frame = -1;
	int row = 0;
	int modes = 0;               // presses of MODE
	int songs = 0;               // steps of the SONG wheel
	int patterns = 0, lengths = 0, mutes = 0; // steps of the column wheels
	int inserts = 0, removes = 0;
	int jump = -1;               // a row double-clicked, to play from
};

/** What the Sequencer sends a Song Editor's lights. */
struct SongOut {
	bool songMode = false;
};

/** A Song Editor's screen, as the UI thread copies it out of the Sequencer: times in sixteenths and seconds. */
struct SongView {
	bool songMode = false, playing = false, external = false, loop = false;
	int song = 0, rowPlaying = 0;
	float bpm = 120.f, progress = 0.f; // the tempo now; how far the row playing is through its length
	double length = 0.0, at = 0.0;     // the song's whole length and the playhead, in sixteenths
	double seconds = 0.0, secondsAt = 0.0;
	double rowLengths[SONG_ROWS] = {}; // sixteenths
	Project::Chain chain;
};

struct Hub {
	JacksOut out[2][LANES];
	JacksIn in[2][LANES];
	RemoteIn remoteIn[2];
	RemoteOut remoteOut[2];
	SongIn songIn[2];
	SongOut songOut[2];

	virtual ~Hub() = default;

	/** Where the Sequencer writes a track's outputs in this frame, and what a breakout reads, written in the frame
	before. */
	JacksOut& jacksOut(int track, int64_t frame) { return out[frame & 1][track]; }
	const JacksOut& lastJacksOut(int track, int64_t frame) const { return out[(frame + 1) & 1][track]; }

	/** Where a breakout writes its jacks in this frame, and what the Sequencer reads: the frame before's, or null
	where no breakout wrote them. */
	JacksIn& jacksIn(int track, int64_t frame) { return in[frame & 1][track]; }
	const JacksIn* lastJacksIn(int track, int64_t frame) const {
		const JacksIn& j = in[(frame + 1) & 1][track];
		return j.frame == frame - 1 ? &j : nullptr;
	}

	/** A Remotes module's side of the hub, double-buffered by frame like the breakouts'. */
	RemoteIn& remoteInFor(int64_t frame) { return remoteIn[frame & 1]; }
	const RemoteIn* lastRemoteIn(int64_t frame) const {
		const RemoteIn& r = remoteIn[(frame + 1) & 1];
		return r.frame == frame - 1 ? &r : nullptr;
	}
	RemoteOut& remoteOutFor(int64_t frame) { return remoteOut[frame & 1]; }
	const RemoteOut& lastRemoteOut(int64_t frame) const { return remoteOut[(frame + 1) & 1]; }

	SongIn& songInFor(int64_t frame) { return songIn[frame & 1]; }
	const SongIn* lastSongIn(int64_t frame) const {
		const SongIn& s = songIn[(frame + 1) & 1];
		return s.frame == frame - 1 ? &s : nullptr;
	}
	SongOut& songOutFor(int64_t frame) { return songOut[frame & 1]; }
	const SongOut& lastSongOut(int64_t frame) const { return songOut[(frame + 1) & 1]; }

	/** For the UI thread: a Song Editor's screen. False if the Sequencer is busy with its patterns just now. */
	virtual bool songView(SongView& view) = 0;

	// For the UI thread: the core playing a track (null for the master track), its plug's name, and the track
	// selected.
	virtual TrackCore* trackCore(int track) = 0;
	virtual std::string plugName(int track) = 0;
	virtual int selectedTrack() = 0;
};

/** The hub of the row a module is on, found by walking left: for the UI thread, which can't keep it, since the
Sequencer can go between two frames. */
inline Hub* findHub(Module* m) {
	for (m = m ? m->leftExpander.module : nullptr; onBus(m); m = m->leftExpander.module)
		if (Hub* h = dynamic_cast<Hub*>(m))
			return h;
	return nullptr;
}

} // namespace opz
