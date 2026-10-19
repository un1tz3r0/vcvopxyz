#pragma once
#include <array>
#include <cmath>

namespace xyz {

inline constexpr int SINE_TABLE = 4096;
inline const std::array<float, SINE_TABLE + 1> SINE_VALUES = []() {
	std::array<float, SINE_TABLE + 1> t;
	for (int i = 0; i <= SINE_TABLE; i++)
		t[i] = std::sin(2.f * float(M_PI) * i / SINE_TABLE);
	return t;
}();

/** sin(2πx), interpolated from a table: within 3e-7 of the real thing, for the phases of FM operators. */
inline float sine(float x) {
	float f = (x - std::floor(x)) * SINE_TABLE;
	int i = int(f);
	float fraction = f - i;
	i &= SINE_TABLE - 1; // an x a hair under a whole number rounds up to f = SINE_TABLE
	return SINE_VALUES[i] + (SINE_VALUES[i + 1] - SINE_VALUES[i]) * fraction;
}

/** tanh, near enough for a soft clip: a rational approximation, exact at 0 and within 3% up to ±3, then held at ±1. */
inline float softClip(float x) {
	x = std::fmax(std::fmin(x, 3.f), -3.f);
	return x * (27.f + x * x) / (27.f + 9.f * x * x);
}

} // namespace xyz
