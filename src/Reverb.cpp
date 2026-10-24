/*
Reverb: the OP-Z's clean reverb with light modulation, on an FX track. It's Dattorro's plate: the send, predelayed and
diffused, circulates through a figure-of-eight tank of delays and allpasses, two of them slowly swaying, and comes out
from taps across it, different on each side. DECAY is how long the tank rings, TONE how much of the top it keeps. The
keys set the predelay, from none on the lowest to 230 ms on the highest.
*/
#include "opz/FxTrack.hpp"

struct Reverb {
	static constexpr const char* SLUG = "Reverb";
	static constexpr int TIME_KEY = 3; // 30 ms, the OP-Z's black key 2
	static constexpr float RATE = 29761.f; // the sample rate Dattorro's lengths are given at

	static void configSound(Module* m, int p1, int p2) {
		m->configParam(p1, 0.f, 1.f, 0.5f, "Decay", "%", 0.f, 100.f)->description = "How long it rings";
		m->configParam(p2, 0.f, 1.f, 0.6f, "Tone", "%", 0.f, 100.f)->description = "Dark to bright";
	}

	static float seconds(int key, float) {
		return 0.01f * key;
	}

	static std::string timeName(int key) {
		return string::f("predelay %d ms", 10 * key);
	}

	struct Unit {
		xyz::DelayLine predelay, diffusers[4], tank[8]; // per half: allpass, delay, allpass, delay
		xyz::OnePole bandwidth, damping[2];
		float scale = 1.f, phase = 0.f, last[2] = {};

		void prepare(float sampleRate) {
			scale = sampleRate / RATE;
			predelay.resize((size_t) (0.5f * sampleRate));
			static const int diffusion[4] = {142, 107, 379, 277};
			for (int i = 0; i < 4; i++)
				diffusers[i].resize(length(diffusion[i]));
			// The swaying allpasses read up to 16 samples beyond their length.
			static const int lengths[8] = {672 + 17, 4453, 1800, 3720, 908 + 17, 4217, 2656, 3163};
			for (int i = 0; i < 8; i++)
				tank[i].resize(length(lengths[i]));
			last[0] = last[1] = 0.f;
		}

		size_t length(float samples) const {
			return (size_t) std::ceil(samples * scale) + 1;
		}

		float tap(const xyz::DelayLine& l, int n) const {
			return l.tap(std::max<size_t>((size_t) std::lround(n * scale), 1));
		}

		void process(const float in[2], float out[2], const opz::FxControls& c) {
			float decay = 0.2f + 0.78f * c.p1, dark = 1.f - c.p2;
			float damp = 1.f - (0.0005f + 0.85f * dark * dark);
			predelay.push(0.5f * (in[0] + in[1]) / 5.f);
			float x = bandwidth.process(predelay.read(clamp(c.time * c.sampleRate, 1.f, predelay.longest())), 0.9995f);
			static const float gains[4] = {0.75f, 0.75f, 0.625f, 0.625f};
			static const int diffusion[4] = {142, 107, 379, 277};
			for (int i = 0; i < 4; i++)
				x = diffusers[i].allpass(x, std::round(diffusion[i] * scale), gains[i]);

			// The two halves, each fed by the other's end; the light modulation sways their first allpasses.
			phase = std::fmod(phase + 0.5f * c.sampleTime, 1.f);
			float sway[2] = {std::sin(2.f * float(M_PI) * phase), std::cos(2.f * float(M_PI) * phase)};
			static const int first[2] = {672, 908}, second[2] = {1800, 2656}, delays[2][2] = {{4453, 3720}, {4217, 3163}};
			float ends[2];
			for (int h = 0; h < 2; h++) {
				xyz::DelayLine* t = tank + 4 * h;
				float v = x + decay * last[1 - h];
				v = t[0].allpass(v, (first[h] + 8.f + 8.f * sway[h]) * scale, -0.7f);
				float d = t[1].tap(std::lround(delays[h][0] * scale));
				t[1].push(v);
				d = damping[h].process(d, damp) * decay;
				d = t[2].allpass(d, std::round(second[h] * scale), 0.5f);
				ends[h] = t[3].tap(std::lround(delays[h][1] * scale));
				t[3].push(d);
			}
			last[0] = ends[0];
			last[1] = ends[1];

			const xyz::DelayLine* t = tank;
			out[0] = tap(t[5], 266) + tap(t[5], 2974) - tap(t[6], 1913) + tap(t[7], 1996) - tap(t[1], 1990) - tap(t[2], 187) - tap(t[3], 1066);
			out[1] = tap(t[1], 353) + tap(t[1], 3627) - tap(t[2], 1228) + tap(t[3], 2673) - tap(t[5], 2111) - tap(t[6], 335) - tap(t[7], 121);
			for (int s = 0; s < 2; s++)
				out[s] = std::isfinite(out[s]) ? 0.6f * 5.f * out[s] : 0.f;
		}
	};
};


static opz::FxPlug<Reverb> reverbPlug(35);
