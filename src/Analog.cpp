/*
Analog: the OP-Z's subtractive engine, a saw, a square an octave below it and noise, through a low-pass that the
amp envelope opens. MIX fades from the saw alone to the saw and the sub, then on to the sub and noise. ENVELOPE is
how far the filter closes between notes: none leaves it open, full closes it down to the note's fundamental and lets
the envelope sweep it eight octaves up.
*/
#include "opz/SynthTrack.hpp"
#include "dsp/Blep.hpp"

struct Analog {
	static constexpr const char* SLUG = "Analog";

	static void configSound(Module* m, int p1, int p2) {
		m->configParam(p1, 0.f, 1.f, 0.25f, "Mix", "%", 0.f, 100.f)->description = "Saw, then saw and sub, then sub and noise";
		m->configParam(p2, 0.f, 1.f, 0.5f, "Envelope", "%", 0.f, 100.f)->description = "How far the envelope sweeps the filter";
	}

	struct Voice {
		float phase = 0.f;
		float sub = -1.f;     // the square an octave down, which flips at every cycle of the saw
		uint32_t noise = 0x9E3779B9u;
		xyz::Svf filter;
		int untilTune = 0;

		float process(float frequency, float sampleTime, float mix, float amount, float envelope) {
			float increment = std::fmin(frequency * sampleTime, 0.5f);
			phase += increment;
			if (phase >= 1.f) {
				phase -= 1.f;
				sub = -sub;
			}
			float saw = xyz::blepSaw(phase, increment);
			// The sub's edges fall on the saw's: before one, it is about to jump against its sign; after, it just
			// jumped with it.
			float square = sub + (phase < increment ? sub : -sub) * xyz::polyBlep(phase, increment);
			noise ^= noise << 13;
			noise ^= noise >> 17;
			noise ^= noise << 5;
			float white = int32_t(noise) * (1.f / 2147483648.f);
			float x = std::fmin(2.f - 2.f * mix, 1.f) * saw + std::fmin(2.f * mix, 1.f) * square + std::fmax(2.f * mix - 1.f, 0.f) * white;

			if (--untilTune <= 0) {
				untilTune = 8;
				filter.tune(frequency * sampleTime * std::exp2(8.f - 8.f * amount * (1.f - envelope)), 1.f);
			}
			return 0.6f * filter.process(x).lp;
		}
	};
};


static opz::SynthPlug<Analog> analogPlug;
