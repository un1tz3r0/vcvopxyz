#pragma once
#include "../plugin.hpp"
#include "Sequence.hpp"

/*
The bus that runs along a row of OP-Z modules: the Sequencer at its left end as the master, then the engines it
plays and any Screens beside them. The master sends every track's notes, the tempo and its controls rightward; the
engines send their audio and state back leftward. Each module passes on what it gets, a sample later, through
Rack's expander messages, so the bus is safe however Rack spreads modules over threads. A module from another
plugin ends it.

The engines take their tracks by kind and order: the first synth engine plays track 5 (bass), the next track 6, and
so on, as drum engines will take tracks 1 to 4.
*/
namespace opz {

constexpr int PAGES = 4;              // pages of dials per track
constexpr int NEXT_PAGE = -1;         // a page request that steps on to the next one

/** What the master sends rightward. */
struct Downstream {
	struct Lane {
		int channels = 0;             // 0 while the lane's GATE output is patched, which takes its engine off the bus
		float pitch[CHANNELS] = {}, velocity[CHANNELS] = {};
		bool gate[CHANNELS] = {};
		float sweep[SWEEP_TARGETS] = {}; // how far the track's sweep moves its engine's dials, as a share of their travel
		float level = 1.f;            // the mixer's gain for the track: its group's level, or 0 with its sound muted
	};
	Lane lanes[TRACKS];
	float secondsPerBeat = 0.5f;
	int selected = DRUM_TRACKS;       // the track the master's dials and keys are on
	bool trackMode = false;
	int page = NEXT_PAGE;             // the page last asked of the selected track's engine
	uint32_t pageRequests = 0;        // counts the requests, so each one is seen once
	int synths = 0;                   // synth engines passed so far
};

/** What the engines send leftward. */
struct Upstream {
	float left = 0.f, right = 0.f;    // the mix of every engine whose own outputs are unpatched
	int8_t page[TRACKS];              // each engine's page, or -1 for a track without one
	float dials[4] = {-1.f, -1.f, -1.f, -1.f}; // the selected track's dials, 0 to 1, or -1 for none

	Upstream() {
		std::fill(std::begin(page), std::end(page), -1);
	}
};

/** A module on the bus. It owns the buffers Rack flips: its left side receives Downstream, its right side Upstream. */
struct BusNode {
	Downstream down[2];
	Upstream up[2];

	void attach(Module* m) {
		m->leftExpander.producerMessage = &down[0];
		m->leftExpander.consumerMessage = &down[1];
		m->rightExpander.producerMessage = &up[0];
		m->rightExpander.consumerMessage = &up[1];
	}

	static bool onBus(Module* m) {
		return m && m->model->plugin == pluginInstance && dynamic_cast<BusNode*>(m);
	}

	/** What came from the left, or null off the bus. */
	static const Downstream* fromLeft(Module* m) {
		return onBus(m->leftExpander.module) ? static_cast<const Downstream*>(m->leftExpander.consumerMessage) : nullptr;
	}
	static const Upstream* fromRight(Module* m) {
		return onBus(m->rightExpander.module) ? static_cast<const Upstream*>(m->rightExpander.consumerMessage) : nullptr;
	}

	/** Where to write what goes right, or null with no module there to take it. */
	static Downstream* toRight(Module* m) {
		Module* r = m->rightExpander.module;
		if (!onBus(r))
			return nullptr;
		r->leftExpander.requestMessageFlip();
		return static_cast<Downstream*>(r->leftExpander.producerMessage);
	}
	static Upstream* toLeft(Module* m) {
		Module* l = m->leftExpander.module;
		if (!onBus(l))
			return nullptr;
		l->rightExpander.requestMessageFlip();
		return static_cast<Upstream*>(l->rightExpander.producerMessage);
	}

	/** Passes both directions on unchanged, for a module that only sits on the bus. */
	static void relay(Module* m) {
		const Downstream* d = fromLeft(m);
		if (Downstream* out = d ? toRight(m) : nullptr)
			*out = *d;
		if (Upstream* out = toLeft(m)) {
			const Upstream* u = fromRight(m);
			*out = u ? *u : Upstream();
		}
	}
};

} // namespace opz
