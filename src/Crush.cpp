/*
Crush: the OP-Z's "vector semilinear crusher" on an FX track. It holds each sample for longer as AMOUNT rises, down to
a 64th of the sample rate, and rounds it to fewer levels, from 12 bits to 2, on a companded scale that's linear for
quiet sounds and logarithmic for loud ones, so a quiet tail crumbles into steps before a loud one does. CUTOFF is a
low-pass after it, to tame the aliases or leave them bright.
*/
#include "opz/FxTrack.hpp"

struct Crush {
	static constexpr const char* SLUG = "Crush";
	static constexpr int TIME_KEY = -1;
	static constexpr float MU = 255.f; // the compander's curve, μ-law's

	static void configSound(Module* m, int p1, int p2) {
		m->configParam(p1, 0.f, 1.f, 0.5f, "Amount", "%", 0.f, 100.f)->description = "Lower rate and fewer levels";
		m->configParam<FilterHz>(p2, 0.f, 1.f, 0.8f, "Cutoff")->description = "A low-pass after the crushing";
	}

	static float cutoffHz(float v) {
		return 100.f * std::pow(200.f, v);
	}

	struct FilterHz : ParamQuantity {
		std::string getDisplayValueString() override {
			return opz::formatHz(cutoffHz(getValue()));
		}
	};

	static float seconds(int, float) {
		return 0.f;
	}

	static std::string timeName(int) {
		return "";
	}

	struct Unit {
		float phase = 1.f, held[2] = {};
		xyz::Svf filters[2];

		void prepare(float) {
			phase = 1.f;
		}

		static float crush(float x, float levels) {
			float v = clamp(x / 5.f, -1.f, 1.f);
			float c = std::copysign(std::log1p(MU * std::fabs(v)) / std::log1p(MU), v);
			c = std::round(c * levels) / levels;
			return 5.f * std::copysign(std::expm1(std::fabs(c) * std::log1p(MU)) / MU, c);
		}

		void process(const float in[2], float out[2], const opz::FxControls& c) {
			float hold = 1.f + 63.f * c.p1 * c.p1, levels = std::exp2(11.f - 10.f * c.p1);
			phase += 1.f / hold;
			if (phase >= 1.f) {
				phase -= std::floor(phase);
				for (int s = 0; s < 2; s++)
					held[s] = crush(in[s], levels);
			}
			for (int s = 0; s < 2; s++) {
				filters[s].tune(cutoffHz(c.p2) * c.sampleTime, float(M_SQRT2));
				out[s] = filters[s].process(held[s]).lp;
			}
		}
	};
};


static opz::FxPlug<Crush> crushPlug(109);
