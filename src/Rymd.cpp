/*
Rymd: the OP-Z's older digital reverb on an FX track, "rymd" being Swedish for space. A Schroeder-Moorer room after
Jezar's Freeverb: eight damped combs in parallel, then four allpasses in series, the right side's a few samples longer
than the left's. AMOUNT sets the room's size, how long the combs ring; CUTOFF how bright they stay.
*/
#include "opz/FxTrack.hpp"

struct Rymd {
	static constexpr const char* SLUG = "Rymd";
	static constexpr int TIME_KEY = -1;
	static constexpr float RATE = 44100.f; // the sample rate Freeverb's lengths are given at
	static constexpr int COMBS = 8, ALLPASSES = 4, SPREAD = 23;
	static constexpr int COMB_LENGTHS[COMBS] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
	static constexpr int ALLPASS_LENGTHS[ALLPASSES] = {556, 441, 341, 225};

	static void configSound(Module* m, int p1, int p2) {
		m->configParam(p1, 0.f, 1.f, 0.5f, "Amount", "%", 0.f, 100.f)->description = "The room's size";
		m->configParam(p2, 0.f, 1.f, 0.5f, "Cutoff", "%", 0.f, 100.f)->description = "Dark to bright";
	}

	static float seconds(int, float) {
		return 0.f;
	}

	static std::string timeName(int) {
		return "";
	}

	struct Unit {
		xyz::DelayLine combs[2][COMBS], allpasses[2][ALLPASSES];
		xyz::OnePole damping[2][COMBS];
		int combLength[2][COMBS] = {}, allpassLength[2][ALLPASSES] = {};

		void prepare(float sampleRate) {
			float scale = sampleRate / RATE;
			for (int s = 0; s < 2; s++) {
				for (int i = 0; i < COMBS; i++) {
					combLength[s][i] = std::max((int) std::lround((COMB_LENGTHS[i] + s * SPREAD) * scale), 1);
					combs[s][i].resize(combLength[s][i]);
					damping[s][i] = xyz::OnePole();
				}
				for (int i = 0; i < ALLPASSES; i++) {
					allpassLength[s][i] = std::max((int) std::lround((ALLPASS_LENGTHS[i] + s * SPREAD) * scale), 1);
					allpasses[s][i].resize(allpassLength[s][i]);
				}
			}
		}

		void process(const float in[2], float out[2], const opz::FxControls& c) {
			float feedback = 0.7f + 0.28f * c.p1, a = 0.4f + 0.6f * c.p2;
			for (int s = 0; s < 2; s++) {
				float x = 0.015f * (in[0] + in[1]), y = 0.f;
				for (int i = 0; i < COMBS; i++) {
					xyz::DelayLine& comb = combs[s][i];
					float echo = comb.tap(combLength[s][i]);
					y += echo;
					comb.push(x + feedback * damping[s][i].process(echo, a));
				}
				// Freeverb's allpasses: the sign of the usual form flipped, which colours them a little, as its own do.
				for (int i = 0; i < ALLPASSES; i++) {
					xyz::DelayLine& ap = allpasses[s][i];
					float delayed = ap.tap(allpassLength[s][i]);
					ap.push(y + 0.5f * delayed);
					y = delayed - y;
				}
				out[s] = 3.f * y;
			}
		}
	};
};


static opz::FxPlug<Rymd> rymdPlug;
