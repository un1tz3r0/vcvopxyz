#pragma once
#include <algorithm>
#include <cmath>

namespace xyz {

/** Two-pole state-variable filter in Zavalishin's topology-preserving form, so it stays stable and quiet while its
cutoff is swept fast. Low-pass, band-pass and high-pass come from the same state. */
struct Svf {
	struct Out {
		float lp, bp, hp;
	};

	float a1 = 1.f, a2 = 0.f, a3 = 0.f, k = 2.f;
	float s1 = 0.f, s2 = 0.f;

	/** `cutoff` is a fraction of the sample rate. `damping` is 1/Q: 2 has no peak, √2 is Butterworth, and it rings
	longer as it falls towards 0. */
	void tune(float cutoff, float damping) {
		float g = std::tan(3.14159265f * std::clamp(cutoff, 1e-5f, 0.49f));
		k = damping;
		a1 = 1.f / (1.f + g * (g + k));
		a2 = g * a1;
		a3 = g * a2;
	}

	Out process(float x) {
		float v3 = x - s2;
		float v1 = a1 * s1 + a2 * v3;
		float v2 = s2 + a2 * s1 + a3 * v3;
		s1 = 2.f * v1 - s1;
		s2 = 2.f * v2 - s2;
		return {v2, v1, x - k * v1 - v2};
	}

	void reset() {
		s1 = s2 = 0.f;
	}
};

} // namespace xyz
