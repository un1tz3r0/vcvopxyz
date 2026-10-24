#pragma once
#include "TrackCore.hpp"

/*
The bus along a row of OP-Z modules: the Sequencer at its left end, which plays every track, then the breakouts that
hand tracks to other modules and the Screens that show the selected one. A module from another plugin ends it.

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

struct Hub {
	JacksOut out[2][LANES];
	JacksIn in[2][LANES];

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
