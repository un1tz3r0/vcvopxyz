// How often Organ's sine() lands on index 4096 (one past the interpolation pair) in ordinary play.
#include <cmath>
#include <cstdio>
struct A { float ratios[4], levels[4]; float modulator, depth; };
static const A ALG[] = {{{1, 2}, {1, .5f}, 1, .4f}, {{.5f, 1, 1.5f}, {.8f, 1, .8f}, 2, .3f}, {{1, 2, 3, 4}, {1, .7f, .6f, .5f}, 4, .25f},
	{{.5f, 1, 2, 8}, {1, 1, .6f, .35f}, 1, .5f}, {{1}, {1}, 1, 1.5f}, {{1, 2}, {1, .4f}, 2, 1.2f}, {{1, 3}, {1, .5f}, 3, 1}, {{1, 2}, {1, .5f}, 3.5f, .8f}};
int main() {
	long hits = 0, calls = 0;
	double seconds = 0;
	for (int alg = 0; alg < 8; alg++)
		for (int note = 36; note < 96; note += 7) {
			float f = 440.f * std::pow(2.f, (note - 69) / 12.f), phase = 0.f, tweak = 0.2f;
			for (int i = 0; i < 48000 * 60; i++) {
				phase += std::fmin(f / 48000.f, 0.5f);
				if (phase >= 2.f) phase -= 2.f;
				float x0 = ALG[alg].modulator * phase;
				float m = tweak * ALG[alg].depth * std::sin(2 * float(M_PI) * x0);
				for (int k = 0; k < 4 && ALG[alg].levels[k] > 0; k++) {
					float x = ALG[alg].ratios[k] * phase + m;
					calls++;
					if (int((x - std::floor(x)) * 4096) == 4096) hits++;
				}
			}
			seconds += 60;
		}
	printf("%.0f voice-seconds at tweak 0.2: %ld of %ld lookups hit index 4096 (one every %.0f voice-seconds)\n", seconds, hits, calls, hits ? seconds / hits : 0.0);
}
