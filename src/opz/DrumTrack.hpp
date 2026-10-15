#pragma once
#include "Track.hpp"

/*
An OP-Z drum track (Track.hpp), whatever plays its sounds: Drum's kit, or Drum Breakout's modules outside. It takes
one of tracks 1 to 4, its keys pick one of 24 sounds, its sound dials are PITCH and REVERSE, its note styles are the
OP-Z's drum ones, and it has no portamento.
*/
namespace opz {

struct DrumTrack : Track {
	enum NoteStyle { RETRIG, MONO, GATE, LOOP };
	static constexpr int SOUNDS = 24;

	DrumTrack() {
		drum = true;
		configParam(P1_PARAM, -12.f, 12.f, 0.f, "Pitch", " semitones")->description = "Tunes every sound, up to an octave either way";
		configSwitch(P2_PARAM, 0.f, 1.f, 0.f, "Reverse", {"Off", "On"})->description = "Plays each sound from its end, as it starts";
		configTrack({"Retrig", "Mono", "Gate", "Loop"},
			"Retrig plays each sound to its end, and mono cuts the last one off; gate stops it with its note, and loop repeats it while the note is held",
			{"Pitch", "Reverse", "Filter", "Resonance", "Attack", "Pitch, 2 octaves", "Pan", "Volume"});
		configDefault(SUSTAIN_PARAM, 1.f);
		configDefault(RELEASE_PARAM, 0.4f);
		getParamQuantity(GLIDE_PARAM)->description = "Synth tracks only";
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
