/*
Saw: the OP-Z's filtered waves, two saws a few cents apart through a resonant low-pass. TONE sets where the filter
sits over the note and how much it rings; ENVELOPE is how far the amp envelope opens it from there, sharply at the
start of a note and easing off as it decays, for plucks at one end and swells at the other.
*/
#include "opz/SynthTrackWidget.hpp"
#include "dsp/Blep.hpp"

struct Saw {
	static constexpr const char* SLUG = "Saw";
	static constexpr float DETUNE = 0.0035f; // each saw's offset from the note, about 6 cents

	static void configSound(Module* m, int p1, int p2) {
		m->configParam(p1, 0.f, 1.f, 0.5f, "Envelope", "%", 0.f, 100.f)->description = "How far the envelope opens the filter";
		m->configParam(p2, 0.f, 1.f, 0.4f, "Tone", "%", 0.f, 100.f)->description = "The filter's cutoff and resonance";
	}

	struct Voice {
		float phases[2] = {0.f, 0.5f};
		xyz::Svf filter;
		int untilTune = 0;

		float process(float frequency, float sampleTime, float amount, float tone, float envelope) {
			float x = 0.f;
			for (int i = 0; i < 2; i++) {
				float increment = std::fmin(frequency * (1.f + (i ? DETUNE : -DETUNE)) * sampleTime, 0.5f);
				phases[i] += increment;
				if (phases[i] >= 1.f)
					phases[i] -= 1.f;
				x += xyz::blepSaw(phases[i], increment);
			}
			if (--untilTune <= 0) {
				untilTune = 8;
				float octaves = 1.f + 6.f * tone + 6.f * amount * envelope * envelope;
				filter.tune(frequency * sampleTime * std::exp2(octaves), 1.5f - 1.1f * tone);
			}
			return 0.6f * filter.process(x).lp;
		}
	};
};


Model* modelSaw = createModel<opz::SynthTrack<Saw>, opz::SynthTrackWidget<Saw>>(Saw::SLUG);
