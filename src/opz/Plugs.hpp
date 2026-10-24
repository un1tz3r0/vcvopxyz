#pragma once
#include "TrackCore.hpp"
#include <memory>

/*
The plugs: every sound a track can play, as the OP-Z calls its synth engines, its drum player and its effects, and
each kind's breakout, whose voice is made of other modules. An engine registers its plug from its own file
(SynthPlug, FxPlug); the Sequencer makes a core of every plug of a track's kind for each track (Host.hpp).

A plug's two sound dials are configured, as its engine configures them, on a module of their own, made once on the
UI thread when first wanted. The Sequencer's own sound dials run 0 to 1 and show themselves through these (PlugDial),
and a core takes its dials' travel from them (SoundRange).
*/
namespace opz {

enum Kind { SYNTH, DRUM, FX, KINDS };

/** A track's kind by its lane. */
constexpr int laneKind(int lane) {
	return lane >= FX1 ? FX : lane < DRUM_TRACKS ? DRUM : SYNTH;
}

struct Plug {
	std::string slug;            // the engine module's slug before 2.1.0, as patches and row records keep it
	int kind = SYNTH;
	int opz = 0;                 // its plug id in an OP-Z project file, or 0 where none is known
	bool breakout = false;       // the kind's breakout
	void (*configSound)(Module* m, int p1, int p2) = nullptr;
	std::unique_ptr<TrackCore> (*make)() = nullptr;

	/** Its sound dial `i`, as its engine configures it; for the UI thread. */
	ParamQuantity* dial(int i) const;
	SoundRange range(int i) const;
	/** Where its sound dial starts, as a share of its travel. */
	float defaultShare(int i) const;
};

/** Every plug of a kind but its breakout, in order of slug; the order a track keeps its cores in. */
const std::vector<const Plug*>& plugs(int kind);
const Plug* breakoutPlug(int kind);
/** A plug's place in plugs(kind), or -1. */
int plugIndex(int kind, const std::string& slug);
/** A plug's place among every plug, breakouts too, for tables kept beside them, and how many there are. */
int plugNumber(const Plug* p);
const Plug* numberedPlug(int n);
int plugCount();

/** Adds a plug to the list, from a file's static initialization. */
struct PlugRegistration {
	explicit PlugRegistration(const Plug& p);
};

template <class Core>
std::unique_ptr<TrackCore> makeCore() {
	return std::make_unique<Core>();
}

} // namespace opz
