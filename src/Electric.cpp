/*
Electric: the OP-Z's complex and transforming engine, two sines that modulate each other's phase. CROSS MOD is how
hard, from two pure tones to a snarl that never quite settles; the amp envelope scales it too, so a note opens up as
it starts and calms as it fades. X MOD tunes the second sine against the first, from an octave below to two above,
through the inharmonic ratios between. It runs at twice the sample rate, to keep the sidebands from folding back.
*/
#include "opz/SynthTrackWidget.hpp"
#include "dsp/Sine.hpp"

struct Electric {
	static constexpr const char* SLUG = "Electric";

	static void configSound(Module* m, int p1, int p2) {
		m->configParam(p1, 0.f, 1.f, 0.3f, "Cross mod", "%", 0.f, 100.f)->description = "How hard the two sines modulate each other";
		m->configParam(p2, 0.f, 1.f, 1.f / 3.f, "X mod", "", 8.f, 0.5f)->description = "The second sine's ratio to the note, from 0.5 to 4";
	}

	struct Voice {
		float a = 0.f, b = 0.f, outA = 0.f, outB = 0.f;
		dsp::Decimator<2, 8> decimator;

		float process(float frequency, float sampleTime, float cross, float xmod, float envelope) {
			float increment = std::fmin(0.5f * frequency * sampleTime, 0.25f);
			float ratio = 0.5f * std::exp2(3.f * xmod), index = 1.5f * cross * cross * (0.35f + 0.65f * envelope);
			float y[2];
			for (float& out : y) {
				a += increment;
				b += increment * ratio;
				a -= std::floor(a);
				b -= std::floor(b);
				float nextA = xyz::sine(a + index * outB), nextB = xyz::sine(b + 0.7f * index * outA);
				outA = nextA;
				outB = nextB;
				out = 0.65f * outA + 0.35f * outB;
			}
			return decimator.process(y);
		}
	};
};


Model* modelElectric = createModel<opz::SynthTrack<Electric>, opz::SynthTrackWidget<Electric>>(Electric::SLUG);
