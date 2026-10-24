/*
Shade: the OP-Z's smooth piano, a soft FM electric piano. Each note is a sine modulated by another at its own
pitch, struck bright and mellowing as its envelope falls. DETUNE plays a second piano against the first, from
unison to a wide, beating chorus; DRIVE pushes both into a soft clip, from clean to growling.
*/
#include "opz/SynthTrack.hpp"
#include "dsp/Sine.hpp"

struct Shade {
	static constexpr const char* SLUG = "Shade";

	static void configSound(Module* m, int p1, int p2) {
		m->configParam(p1, 0.f, 1.f, 0.2f, "Detune", "%", 0.f, 100.f)->description = "A second piano, detuned against the first";
		m->configParam(p2, 0.f, 1.f, 0.2f, "Drive", "%", 0.f, 100.f)->description = "From clean to growling";
	}

	struct Voice {
		float phases[2] = {};

		float process(float frequency, float sampleTime, float detune, float drive, float envelope) {
			float index = 0.03f + 0.15f * envelope * envelope, x = 0.f;
			for (int i = 0; i < 2; i++) {
				float cents = (i ? 12.5f : -12.5f) * detune;
				phases[i] += std::fmin(frequency * std::exp2(cents / 1200.f) * sampleTime, 0.5f);
				phases[i] -= std::floor(phases[i]);
				x += xyz::sine(phases[i] + index * xyz::sine(phases[i]));
			}
			float gain = 1.f + 7.f * drive;
			return xyz::softClip(0.5f * gain * x) / xyz::softClip(gain);
		}
	};
};


static opz::SynthPlug<Shade> shadePlug(31);
