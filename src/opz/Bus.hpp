#pragma once
#include "../plugin.hpp"
#include "Sequence.hpp"
#include <atomic>

/*
The bus that runs along a row of OP-Z modules: the Sequencer at its left end as the master, then the engines it
plays and any Screens beside them. A module from another plugin ends it.

The engines take their tracks by kind and order: the first drum engine plays track 1 (kick), the next track 2, and
so on to track 4, the first synth engine plays track 5 (bass), the next track 6, and so on to track 8, and the first
FX module plays FX 1, the next FX 2.

The master holds the bus itself (Hub): what it sends every engine, and a slot per track for what that track's engine
sends back. Both are double-buffered by Rack's frame count: in frame n the master writes the n & 1 copy of each and
reads the other, written in frame n - 1, and the engines do the same from their side. Rack finishes every module's
frame before it starts the next, so no copy is ever written and read at once, however it spreads modules over
threads, and nothing is copied along the row: every engine is one sample behind the master, wherever it sits.

Which master and track an engine has only changes when modules move, so each engine walks left to find them again
only after Rack has given some module on a bus a new neighbour.

The engines' FX sends come back with their sound, and the master sends them on to the FX modules, whose sound joins
the mix, so a module's place on the bus doesn't matter.
*/
namespace opz {

constexpr int PAGES = 4;              // pages of dials per track
constexpr int NEXT_PAGE = -1;         // a page request that steps on to the next one

/** What the master sends every engine. */
struct Downstream {
	struct Lane {
		int channels = 0;             // the track's polyphony; 0 until the master first writes it
		float pitch[CHANNELS] = {}, velocity[CHANNELS] = {};
		bool gate[CHANNELS] = {};
		float sweep[SWEEP_TARGETS] = {}; // how far the track's sweep moves its engine's dials, as a share of their travel
		float level = 1.f;            // the mixer's gain for the track: its group's level, or 0 with its sound muted
		uint32_t locks = 0;           // the dials the step playing locks, by the synth track's param
		float lockValues[SOUND_PARAMS] = {}; // where it holds them, 0 to 1 over each one's travel
	};
	Lane lanes[LANES];                // by the master's lanes; the master track's goes unused
	float send[FX_TRACKS] = {};       // what the engines send each FX module, as they sent it back
	float secondsPerBeat = 0.5f;
	int selected = DRUM_TRACKS;       // the track the master's dials and keys are on
	bool trackMode = false;
	int page = NEXT_PAGE;             // the page last asked of the selected track's engine
	uint32_t pageRequests = 0;        // counts the requests, so each one is seen once
};

/** What a track's engine sends the master. */
struct Slot {
	int64_t frame = -1;               // when it was written: an older slot's engine has since left the track
	float left = 0.f, right = 0.f;    // its sound, while its own outputs are unpatched
	float send[FX_TRACKS] = {};       // its FX sends, while its own FX outputs are unpatched
	int8_t page = -1;                 // its page, or -1 in track mode
	int8_t timeKey = -1;              // the key an FX module's notes last set its time with, or -1
	float dials[4] = {-1.f, -1.f, -1.f, -1.f}; // its dials, 0 to 1, or -1 for none
	float arp[4] = {};                // its arp dials, as turned (the master applies a step's locks)
};

/** What every engine sent back, gathered by the master. */
struct Upstream {
	float left = 0.f, right = 0.f;    // the mix of every engine whose own outputs are unpatched
	float send[FX_TRACKS] = {};       // the FX sends of every engine whose own FX outputs are unpatched
	int8_t page[LANES];               // each engine's page, or -1 for a track without one
	int8_t timeKey[FX_TRACKS];        // the key each FX module's notes last set its time with, or -1
	float dials[4] = {-1.f, -1.f, -1.f, -1.f}; // the selected track's dials, 0 to 1, or -1 for none
	float arp[4] = {};                // the arp track's engine's arp dials, as turned; off with no engine

	Upstream() {
		std::fill(std::begin(page), std::end(page), -1);
		std::fill(std::begin(timeKey), std::end(timeKey), -1);
	}
};

/** Whether a neighbour carries the bus on: every module in this plugin does, and checking the plugin is cheaper than
a dynamic_cast. */
inline bool onBus(Module* m) {
	return m && m->model->plugin == pluginInstance;
}

/** Counts the times Rack has given a module on a bus a new neighbour, which it does between blocks, never mid-frame. */
inline std::atomic<uint32_t> busLayout{0};

/** For every module on a bus: when its neighbours change, every engine finds its place again. */
inline void busMoved() {
	busLayout.fetch_add(1, std::memory_order_relaxed);
}

/** The bus, held by the master. */
struct Hub {
	Downstream down[2];
	Slot up[2][LANES];

	/** Where the master writes in this frame, and where the engines read what it wrote in the frame before. */
	Downstream& downstream(int64_t frame) { return down[frame & 1]; }
	const Downstream& lastDownstream(int64_t frame) const { return down[(frame + 1) & 1]; }

	/** Where a track's engine writes in this frame. */
	Slot& slot(int track, int64_t frame) { return up[frame & 1][track]; }

	/** What the engines wrote in the frame before. */
	Upstream gather(int64_t frame, int selected) const {
		Upstream u;
		for (int t = 0; t < LANES; t++) {
			const Slot& s = up[(frame + 1) & 1][t];
			if (s.frame != frame - 1)
				continue;
			u.left += s.left;
			u.right += s.right;
			for (int k = 0; k < FX_TRACKS; k++)
				u.send[k] += s.send[k];
			u.page[t] = s.page;
			if (t >= FX1)
				u.timeKey[t - FX1] = s.timeKey;
			if (t == selected)
				std::copy(std::begin(s.dials), std::end(s.dials), u.dials);
			if (t == ARP_TRACK)
				std::copy(std::begin(s.arp), std::end(s.arp), u.arp);
		}
		return u;
	}
};

} // namespace opz
