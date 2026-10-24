/*
Dist: the OP-Z's overdrive on an FX track. AMOUNT drives the send up to 40 dB into a soft, slightly lopsided clipper,
whose even harmonics warm it as a valve's would, run at four times the sample rate to keep the aliases down; CUTOFF
is a low-pass after it, the tone control.
*/
#include "opz/FxTrack.hpp"

struct Dist {
	static constexpr const char* SLUG = "Dist";
	static constexpr int TIME_KEY = -1;
	static constexpr int OVERSAMPLE = 4, QUALITY = 8;
	static constexpr float BIAS = 0.2f; // the clipper's lean, for the even harmonics

	static void configSound(Module* m, int p1, int p2) {
		m->configParam(p1, 0.f, 1.f, 0.4f, "Amount", "%", 0.f, 100.f)->description = "Drive, up to 40 dB";
		m->configParam<FilterHz>(p2, 0.f, 1.f, 0.7f, "Cutoff")->description = "Tone: a low-pass after the clipper";
	}

	static float cutoffHz(float v) {
		return 300.f * std::pow(60.f, v);
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
		dsp::Upsampler<OVERSAMPLE, QUALITY> up[2];
		dsp::Decimator<OVERSAMPLE, QUALITY> down[2];
		xyz::Svf tone[2];
		float lastIn[2] = {}, lastOut[2] = {};

		void prepare(float) {}

		void process(const float in[2], float out[2], const opz::FxControls& c) {
			float drive = std::pow(100.f, c.p1);
			// Louder drive clips harder, not just louder: the output stays near the send's level.
			float makeup = 1.f / (1.f + 0.25f * std::log2(drive));
			for (int s = 0; s < 2; s++) {
				float x[OVERSAMPLE];
				up[s].process(in[s] / 5.f * drive, x);
				for (float& v : x)
					v = std::tanh(v + BIAS) - std::tanh(BIAS);
				float y = down[s].process(x);
				// A DC blocker, for what the lean leaves behind.
				float blocked = y - lastIn[s] + 0.995f * lastOut[s];
				lastIn[s] = y;
				lastOut[s] = blocked;
				tone[s].tune(cutoffHz(c.p2) * c.sampleTime, float(M_SQRT2));
				out[s] = 5.f * makeup * tone[s].process(blocked).lp;
			}
		}
	};
};


static opz::FxPlug<Dist> distPlug(99);
