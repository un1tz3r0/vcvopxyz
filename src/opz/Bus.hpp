#pragma once
#include "../plugin.hpp"
#include "Sequence.hpp"
#include <atomic>

/*
What the Sequencer hands each track's core every frame (Downstream): the lane of notes, the locks and sweep, the
tempo and the master's keys; and what a core hands back (Slot): its sound and FX sends, for the mix. Both stay inside
the Sequencer, which plays every track itself in one frame: the lanes first, then the drum and synth tracks, then the
FX tracks with this frame's sends, then the mix.

The bus along the row is the hub (Hub.hpp): the breakouts and Screens to the Sequencer's right find it by walking
left across this plugin's modules, again only after Rack has given one of them a new neighbour.
*/
namespace opz {

constexpr int PAGES = 4;              // pages of dials per track
constexpr int NEXT_PAGE = -1;         // a page request that steps on to the next one

/** What the Sequencer hands every track's core. */
struct Downstream {
	struct Lane {
		int channels = 0;             // the track's polyphony; 0 until the master first writes it
		float pitch[CHANNELS] = {}, velocity[CHANNELS] = {};
		bool gate[CHANNELS] = {};
		float sweep[SWEEP_TARGETS] = {}; // how far the track's sweep moves its dials, as a share of their travel
		float level = 1.f;            // the mixer's gain for the track: its group's level, or 0 with its sound muted
		uint32_t locks = 0;           // the dials the step playing locks, by the synth track's param
		float lockValues[SOUND_PARAMS] = {}; // where it holds them, 0 to 1 over each one's travel
	};
	Lane lanes[LANES];                // by the Sequencer's lanes; the master track's goes unused
	float send[FX_TRACKS] = {};       // what the tracks send each FX track this frame
	float secondsPerBeat = 0.5f;
	int selected = DRUM_TRACKS;       // the track the master's dials and keys are on
	bool trackMode = false;
	int page = NEXT_PAGE;             // the page last asked of the selected track
	uint32_t pageRequests = 0;        // counts the requests, so each one is seen once
};

/** What a track's core hands back. */
struct Slot {
	int64_t frame = -1;               // when it was written
	float left = 0.f, right = 0.f;    // its sound, while its breakout's audio outputs are unpatched
	float send[FX_TRACKS] = {};       // its FX sends, while its breakout's FX outputs are unpatched
};

/** Whether a neighbour carries the bus on: every module in this plugin does, and checking the plugin is cheaper than
a dynamic_cast. */
inline bool onBus(Module* m) {
	return m && m->model->plugin == pluginInstance;
}

/** Counts the times Rack has given a module on a bus a new neighbour, which it does between blocks, never mid-frame. */
inline std::atomic<uint32_t> busLayout{0};

/** For every module on a bus: when its neighbours change, every breakout and Screen finds its place again. */
inline void busMoved() {
	busLayout.fetch_add(1, std::memory_order_relaxed);
}

} // namespace opz
