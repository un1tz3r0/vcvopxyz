#pragma once

namespace xyz {

/** The polynomial correction for a downward step of 2 in a waveform at phase 0, `phase` and `increment` in cycles:
subtracting it from a naive saw, or adding it at a square's rising edge, rounds the step off and takes most of its
aliasing with it. */
inline float polyBlep(float phase, float increment) {
	if (phase < increment) {
		float t = phase / increment;
		return t + t - t * t - 1.f;
	}
	if (phase > 1.f - increment) {
		float t = (phase - 1.f) / increment;
		return t * t + t + t + 1.f;
	}
	return 0.f;
}

/** A saw from -1 to 1 with its step smoothed. */
inline float blepSaw(float phase, float increment) {
	return 2.f * phase - 1.f - polyBlep(phase, increment);
}

} // namespace xyz
