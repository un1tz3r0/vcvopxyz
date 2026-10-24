/*
Uranus: the OP-Z's clean bass, a sine operator that can feed back on itself, as a DX7's does. TONE adds the second
and third harmonics, from a pure sine to a round, present bass; FEEDBACK bends the operator's own phase with its
output, leaning it towards a saw.
*/
#include "opz/SynthTrack.hpp"
#include "dsp/Sine.hpp"

struct Uranus {
	static constexpr const char* SLUG = "Uranus";

	static void configSound(Module* m, int p1, int p2) {
		m->configParam(p1, 0.f, 1.f, 0.3f, "Tone", "%", 0.f, 100.f)->description = "Adds the second and third harmonics";
		m->configParam(p2, 0.f, 1.f, 0.1f, "Feedback", "%", 0.f, 100.f)->description = "From a sine towards a saw";
	}

	struct Voice {
		float phase = 0.f, last1 = 0.f, last2 = 0.f, lastIn = 0.f, lastOut = 0.f;

		float process(float frequency, float sampleTime, float tone, float feedback, float) {
			phase += std::fmin(frequency * sampleTime, 0.5f);
			phase -= std::floor(phase);
			// Feeding back the mean of the last two outputs, as the DX7 does, keeps high feedback from chattering.
			float s = xyz::sine(phase + 0.22f * feedback * (last1 + last2));
			last2 = last1;
			last1 = s;
			float x = s + 0.5f * tone * xyz::sine(2.f * phase) + 0.3f * tone * tone * xyz::sine(3.f * phase);
			x /= 1.f + 0.5f * tone + 0.3f * tone * tone;
			// Feedback skews the wave, and with it its mean; a 10 Hz high-pass takes the offset back out.
			lastOut = x - lastIn + (1.f - 2.f * float(M_PI) * 10.f * sampleTime) * lastOut;
			lastIn = x;
			return lastOut;
		}
	};
};


static opz::SynthPlug<Uranus> uranusPlug(129);
