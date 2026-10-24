#pragma once
#include "Plugs.hpp"

/*
An OP-Z drum track (TrackCore.hpp), whatever plays its sounds: a kit (KitCore.hpp), or a Drum Breakout's modules
outside. It plays one of tracks 1 to 4, its keys pick one of 24 sounds, its sound dials are PITCH and REVERSE, its
note styles are the OP-Z's drum ones, and it has no portamento.
*/
namespace opz {

struct DrumCore : TrackCore {
	enum NoteStyle { RETRIG, MONO, GATE, LOOP };
	static constexpr int SOUNDS = 24;

	DrumCore() {
		drum = true;
	}

	static void configSound(Module* m, int p1, int p2) {
		m->configParam(p1, -12.f, 12.f, 0.f, "Pitch", " semitones")->description = "Tunes every sound, up to an octave either way";
		m->configSwitch(p2, 0.f, 1.f, 0.f, "Reverse", {"Off", "On"})->description = "Plays each sound from its end, as it starts";
	}

	/** Drum tracks have no portamento. */
	int dial(int page, int d) const override {
		return page == PAGE_TRACK && d == 3 ? -1 : DIALS[page][d];
	}

	/** The sound a note plays: the keys F3 to E5 play the 24 in turn, and the octaves either side play them again. */
	static int soundOf(float pitch) {
		int key = (int) std::round(12.f * pitch) + 7;
		return (key % SOUNDS + SOUNDS) % SOUNDS;
	}

	std::string soundName(int pitch) override {
		return string::f("sound %d", soundOf((pitch - 60) / 12.f) + 1);
	}
};

} // namespace opz
