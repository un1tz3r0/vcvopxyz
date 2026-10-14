/*
Organ: the OP-Z's FM organ, eight algorithms of up to four sine carriers sharing one modulator. ALGORITHM picks
the registration, from drawbar organs to FM reeds and bells; TWEAK is how hard the modulator drives the carriers,
from pure drawbars to grit and bark.
*/
#include "opz/SynthTrackWidget.hpp"
#include <array>

struct Organ {
	static constexpr const char* SLUG = "Organ";

	/** Carriers' and the modulator's ratios to the note, all multiples of a half so every one repeats in two of the
	note's cycles. */
	struct Algorithm {
		const char* name;
		float ratios[4], levels[4];
		float modulator, depth;      // the modulator's ratio, and its index at full tweak in cycles
	};
	static constexpr Algorithm ALGORITHMS[] = {
		{"Flute 8' 4'", {1.f, 2.f}, {1.f, 0.5f}, 1.f, 0.4f},
		{"Jazz 16' 8' 5 1/3'", {0.5f, 1.f, 1.5f}, {0.8f, 1.f, 0.8f}, 2.f, 0.3f},
		{"Full 8' 4' 2 2/3' 2'", {1.f, 2.f, 3.f, 4.f}, {1.f, 0.7f, 0.6f, 0.5f}, 4.f, 0.25f},
		{"Gospel 16' 8' 4' 1'", {0.5f, 1.f, 2.f, 8.f}, {1.f, 1.f, 0.6f, 0.35f}, 1.f, 0.5f},
		{"Reed", {1.f}, {1.f}, 1.f, 1.5f},
		{"Pipe", {1.f, 2.f}, {1.f, 0.4f}, 2.f, 1.2f},
		{"Combo", {1.f, 3.f}, {1.f, 0.5f}, 3.f, 1.f},
		{"Bell", {1.f, 2.f}, {1.f, 0.5f}, 3.5f, 0.8f},
	};
	static constexpr int ALGORITHMS_LEN = 8;

	static void configSound(Module* m, int p1, int p2) {
		std::vector<std::string> names;
		for (const Algorithm& a : ALGORITHMS)
			names.push_back(a.name);
		m->configSwitch(p1, 0.f, ALGORITHMS_LEN - 1, 0.f, "Algorithm", names);
		m->configParam(p2, 0.f, 1.f, 0.2f, "Tweak", "%", 0.f, 100.f)->description = "Frequency modulation depth";
	}

	static constexpr int TABLE = 4096;
	static inline const std::array<float, TABLE + 1> SINE = []() {
		std::array<float, TABLE + 1> t;
		for (int i = 0; i <= TABLE; i++)
			t[i] = std::sin(2.f * float(M_PI) * i / TABLE);
		return t;
	}();

	/** sin(2πx), interpolated from the table. */
	static float sine(float x) {
		float f = (x - std::floor(x)) * TABLE;
		int i = int(f);
		float fraction = f - i;
		i &= TABLE - 1; // an x a hair under a whole number rounds up to f = TABLE
		return SINE[i] + (SINE[i + 1] - SINE[i]) * fraction;
	}

	struct Voice {
		float phase = 0.f; // in the note's cycles, 0 to 2

		float process(float frequency, float sampleTime, float algorithm, float tweak, float) {
			const Algorithm& a = ALGORITHMS[clamp(int(algorithm), 0, ALGORITHMS_LEN - 1)];
			phase += std::fmin(frequency * sampleTime, 0.5f);
			if (phase >= 2.f)
				phase -= 2.f;
			float modulation = tweak * a.depth * sine(a.modulator * phase);
			float x = 0.f, total = 0.f;
			for (int i = 0; i < 4 && a.levels[i] > 0.f; i++) {
				x += a.levels[i] * sine(a.ratios[i] * phase + modulation);
				total += a.levels[i];
			}
			return x / total;
		}
	};
};


Model* modelOrgan = createModel<opz::SynthTrack<Organ>, opz::SynthTrackWidget<Organ>>(Organ::SLUG);
