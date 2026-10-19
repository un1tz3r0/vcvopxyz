/*
Volt: the OP-Z's multi-oscillator electric synthesis, a saw, a pulse and a triangle sub. VARIATION tunes the pulse
up from the saw, from unison through every interval to two octaves, and brings the sub in as it goes; MODULATION
sweeps the pulse's width with a slow LFO of the voice's own and rings the saw and pulse together.
*/
#include "opz/SynthTrackWidget.hpp"
#include "dsp/Blep.hpp"
#include "dsp/Sine.hpp"

struct Volt {
	static constexpr const char* SLUG = "Volt";
	static constexpr float LFO_HZ = 0.7f;

	static void configSound(Module* m, int p1, int p2) {
		m->configParam(p1, 0.f, 1.f, 7.f / 24.f, "Variation", " semitones", 0.f, 24.f)->description = "The pulse's interval above the saw, and the sub under them";
		m->configParam(p2, 0.f, 1.f, 0.3f, "Modulation", "%", 0.f, 100.f)->description = "Pulse width sweep and ring modulation";
	}

	struct Voice {
		float saw = 0.f, pulse = 0.f, sub = 0.f, lfo = random::uniform();

		static float advance(float& phase, float increment) {
			phase += increment;
			phase -= std::floor(phase);
			return increment;
		}

		float process(float frequency, float sampleTime, float variation, float modulation, float) {
			float sawStep = advance(saw, std::fmin(frequency * sampleTime, 0.5f));
			float pulseStep = advance(pulse, std::fmin(frequency * std::exp2(2.f * variation) * sampleTime, 0.5f));
			advance(sub, std::fmin(0.5f * frequency * sampleTime, 0.5f));
			advance(lfo, LFO_HZ * sampleTime);
			float width = 0.5f - 0.42f * modulation * (0.5f + 0.5f * xyz::sine(lfo));
			float s = xyz::blepSaw(saw, sawStep);
			float lagging = pulse - width + 1.f;
			float p = 0.5f * (xyz::blepSaw(pulse, pulseStep) - xyz::blepSaw(lagging - std::floor(lagging), pulseStep));
			float t = 1.f - 4.f * std::fabs(sub - 0.5f);
			return 0.4f * (s + p + variation * t + modulation * s * p);
		}
	};
};


Model* modelVolt = createModel<opz::SynthTrack<Volt>, opz::SynthTrackWidget<Volt>>(Volt::SLUG);
