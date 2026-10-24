/*
Digital: the OP-Z's raw digital engine on an OP-Z synth track.

A sine read from a 32-bit binary counter, driven past full scale so it wraps around rather than clipping, as integer
overflow would: OCTAVE sets how many times each peak wraps. FEEDBACK adds the sine back to its own phase, as an FM
operator modulating itself, which leans it towards a saw before the wrap. The wrap runs at twice the sample rate, with
first-order antiderivative anti-aliasing on its edges, so it keeps its hard edge without the worst of its aliasing.
*/
#include "opz/SynthTrack.hpp"
#include <array>

struct Digital {
	static constexpr const char* SLUG = "Digital";
	static constexpr float WRAPS = 7.f;

	static void configSound(Module* m, int p1, int p2) {
		m->configParam(p1, 0.f, 1.f, 1.f / WRAPS, "Octave", " wraps", 0.f, WRAPS)->description = "How many times each peak of the sine wraps around";
		m->configParam(p2, 0.f, 1.f, 0.2f, "Feedback", "%", 0.f, 100.f)->description = "How hard the output modulates its own phase";
	}

	static constexpr int SIZE = 2048;
	static inline const std::array<float, SIZE + 1> SINE = []() {
		std::array<float, SIZE + 1> t;
		for (int i = 0; i <= SIZE; i++)
			t[i] = std::sin(2.f * float(M_PI) * i / SIZE);
		return t;
	}();

	static float sine(uint32_t phase) {
		uint32_t i = phase >> 21;
		float f = (phase & 0x1fffff) * (1.f / 0x200000);
		return SINE[i] + (SINE[i + 1] - SINE[i]) * f;
	}

	/** Wraps x into [-1, 1), and its antiderivative, which is continuous since both sides of an edge square to 1. */
	static float wrap(float x) {
		return x - 2.f * std::floor(0.5f * (x + 1.f));
	}
	static float wrapIntegral(float x) {
		float w = wrap(x);
		return 0.5f * w * w;
	}

	struct Voice {
		uint32_t phase = 0;
		float sine1 = 0.f, sine2 = 0.f, x = 0.f;
		dsp::Decimator<2, 8> decimator;

		float process(float frequency, float sampleTime, float octave, float feedback, float) {
			uint32_t increment = uint32_t(std::fmin(0.5f * frequency * sampleTime, 0.25f) * 4294967296.f);
			// Up to three tenths of a cycle of self-modulation, squared for a gentle start, from the last two values
			// averaged so it doesn't flip between two at the top.
			float depth = 0.3f * feedback * feedback * 4294967296.f;
			float drive = 0.99f + 2.f * WRAPS * octave;
			float y[2];
			for (float& out : y) {
				phase += increment;
				float s = sine(phase + uint32_t(int64_t(depth * 0.5f * (sine1 + sine2))));
				sine2 = sine1;
				sine1 = s;
				float x1 = drive * s, dx = x1 - x;
				out = std::fabs(dx) > 1e-4f ? (wrapIntegral(x1) - wrapIntegral(x)) / dx : wrap(0.5f * (x1 + x));
				x = x1;
			}
			return decimator.process(y);
		}
	};
};


static opz::SynthPlug<Digital> digitalPlug(25);
