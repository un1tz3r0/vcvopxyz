/*
Chorus-80: the OP-Z's old-school chorus on an FX track, after the bucket-brigade choruses of early-80s polysynths. The
send runs through a short delay on each side, swept by one triangle LFO, the left up as the right comes down, and
darkened as a bucket brigade would. Mixed back with the dry sound, the two copies drifting in pitch either side of it
widen and thicken it. SPEED is the LFO's rate, DEPTH how far it sweeps.
*/
#include "opz/FxTrack.hpp"

struct Chorus {
	static constexpr const char* SLUG = "Chorus";
	static constexpr int TIME_KEY = -1;
	static constexpr float CENTRE = 0.0035f, SWING = 0.003f; // seconds: the delay, and the most it moves either way

	static void configSound(Module* m, int p1, int p2) {
		m->configParam<Speed>(p1, 0.f, 1.f, 0.35f, "Speed")->description = "The sweep's rate, 0.05 to 8 Hz";
		m->configParam(p2, 0.f, 1.f, 0.5f, "Depth", "%", 0.f, 100.f)->description = "How far it sweeps";
	}

	static float hz(float speed) {
		return 0.05f * std::pow(160.f, speed);
	}

	struct Speed : ParamQuantity {
		std::string getDisplayValueString() override {
			return string::f("%.2f Hz", hz(getValue()));
		}
	};

	static float seconds(int, float) {
		return 0.f;
	}

	static std::string timeName(int) {
		return "";
	}

	struct Unit {
		xyz::DelayLine lines[2];
		xyz::Svf bucket[2];
		float phase = 0.f;

		void prepare(float sampleRate) {
			for (xyz::DelayLine& l : lines)
				l.resize((size_t) ((CENTRE + SWING) * sampleRate) + 4);
			for (xyz::Svf& f : bucket)
				f.reset();
		}

		void process(const float in[2], float out[2], const opz::FxControls& c) {
			phase = std::fmod(phase + hz(c.p1) * c.sampleTime, 1.f);
			float triangle = 4.f * std::fabs(phase - 0.5f) - 1.f, depth = SWING * c.p2;
			for (int s = 0; s < 2; s++) {
				bucket[s].tune(9000.f * c.sampleTime, 1.2f);
				lines[s].push(bucket[s].process(in[s]).lp);
				float delay = (CENTRE + (s ? -depth : depth) * triangle) * c.sampleRate;
				out[s] = lines[s].read(clamp(delay, 1.f, lines[s].longest()));
			}
		}
	};
};


Model* modelChorus = createModel<opz::FxTrack<Chorus>, opz::FxTrackWidget<Chorus>>(Chorus::SLUG);
