/*
EP: the OP-Z's FM electric piano, eight algorithms of a sine carrier and its modulator, some with a tine's click
over the top. The amp envelope drives the modulation, so a note strikes bright and mellows as it fades. ALGORITHM
picks the piano, from tines and reeds to bells, glass and a toy; TONE is how hard the hammer strikes.
*/
#include "opz/SynthTrackWidget.hpp"
#include "dsp/Sine.hpp"

struct EP {
	static constexpr const char* SLUG = "EP";

	/** Ratios to the note are multiples of a half, so every one repeats in two of the note's cycles. */
	struct Algorithm {
		const char* name;
		float modulator, depth;      // the modulator's ratio, and its index at full tone in cycles
		float shape;                 // how fast the index falls with the envelope: its power
		float tine, tineLevel;       // a click's ratio and level, gone faster than the note
	};
	static constexpr Algorithm ALGORITHMS[] = {
		{"Tine", 1.f, 1.f, 2.f, 14.f, 0.12f},
		{"Bright tine", 1.f, 1.6f, 1.5f, 14.f, 0.2f},
		{"Dyno", 1.f, 2.2f, 3.f, 14.f, 0.3f},
		{"Reed", 1.f, 1.4f, 1.f, 0.f, 0.f},
		{"Soft", 1.f, 0.5f, 2.f, 0.f, 0.f},
		{"Bell", 3.5f, 0.9f, 2.f, 0.f, 0.f},
		{"Glass", 7.f, 0.6f, 2.5f, 0.f, 0.f},
		{"Toy", 4.f, 0.8f, 3.f, 9.f, 0.25f},
	};
	static constexpr int ALGORITHMS_LEN = 8;

	static void configSound(Module* m, int p1, int p2) {
		std::vector<std::string> names;
		for (const Algorithm& a : ALGORITHMS)
			names.push_back(a.name);
		m->configSwitch(p1, 0.f, ALGORITHMS_LEN - 1, 0.f, "Algorithm", names);
		m->configParam(p2, 0.f, 1.f, 0.5f, "Tone", "%", 0.f, 100.f)->description = "How hard the hammer strikes";
	}

	struct Voice {
		float phase = 0.f; // in the note's cycles, 0 to 2

		float process(float frequency, float sampleTime, float algorithm, float tone, float envelope) {
			const Algorithm& a = ALGORITHMS[clamp(int(algorithm), 0, ALGORITHMS_LEN - 1)];
			phase += std::fmin(frequency * sampleTime, 0.5f);
			if (phase >= 2.f)
				phase -= 2.f;
			float index = 0.04f + a.depth * (0.2f + 0.8f * tone) * std::pow(envelope, a.shape);
			float e2 = envelope * envelope;
			float x = xyz::sine(phase + index * xyz::sine(a.modulator * phase)) + a.tineLevel * tone * e2 * e2 * xyz::sine(a.tine * phase);
			return x / (1.f + a.tineLevel);
		}
	};
};


Model* modelEP = createModel<opz::SynthTrack<EP>, opz::SynthTrackWidget<EP>>(EP::SLUG);
