/*
Digital: the OP-Z's raw digital engine on an OP-Z synth track.

Two oscillators, each a 32-bit binary counter whose top five bits index a 32-step, 4-bit sine with no
interpolation, so the steps and their aliasing are part of the sound. One modulates the phase of the other:
OCTAVE sets the modulator's frequency in octaves above or below the carrier, FEEDBACK how hard it modulates.
*/
#include "opz/SynthTrack.hpp"
#include <array>

struct Digital {
	static constexpr const char* SLUG = "Digital";

	static void configSound(Module* m, int p1, int p2) {
		m->configSwitch(p1, -2.f, 3.f, 1.f, "Octave", {"-2", "-1", "0", "+1", "+2", "+3"})->description = "Modulator pitch, in octaves from the note";
		m->configParam(p2, 0.f, 1.f, 0.3f, "Feedback", "%", 0.f, 100.f)->description = "Depth of the frequency modulation";
	}

	/** A sine sampled at the middle of each of 32 steps and rounded to 16 levels, symmetric so it has no DC. */
	static inline const std::array<float, 32> SINE = []() {
		std::array<float, 32> t;
		for (int i = 0; i < 32; i++)
			t[i] = (std::round(7.5f * std::sin(2.f * float(M_PI) * (i + 0.5f) / 32.f) + 7.5f) - 7.5f) / 7.5f;
		return t;
	}();

	static float wave(uint32_t phase) {
		return SINE[phase >> 27];
	}

	struct Voice {
		uint32_t carrier = 0, modulator = 0;

		float process(float frequency, float sampleTime, float octave, float feedback) {
			uint32_t increment = uint32_t(std::fmin(frequency * sampleTime, 0.5f) * 4294967296.f);
			int shift = (int) octave;
			carrier += increment;
			// Shifting the increment moves the modulator by whole octaves. Above Nyquist it wraps, like the hardware counter would.
			modulator += shift >= 0 ? increment << shift : increment >> -shift;
			// Up to two whole cycles of phase deviation, squared for a gentle start.
			float depth = 2.f * feedback * feedback * wave(modulator);
			return wave(carrier + uint32_t(int64_t(depth * 4294967296.f)));
		}
	};
};


Model* modelDigital = createModel<opz::SynthTrack<Digital>, opz::SynthTrackWidget<Digital>>(Digital::SLUG);
