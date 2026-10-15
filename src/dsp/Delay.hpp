#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace xyz {

/** A delay line, read back a whole or fractional number of samples after a sample went in. Its length rounds up to a
power of two, so wrapping is a mask. */
struct DelayLine {
	std::vector<float> buffer = std::vector<float>(4, 0.f);
	size_t mask = 3, at = 0;

	/** Room for `samples` of delay, cleared. */
	void resize(size_t samples) {
		size_t size = 4;
		while (size < samples + 4)
			size <<= 1;
		buffer.assign(size, 0.f);
		mask = size - 1;
		at = 0;
	}

	/** The longest delay it holds. */
	float longest() const {
		return float(mask - 3);
	}

	void push(float x) {
		buffer[at] = x;
		at = (at + 1) & mask;
	}

	/** What went in `n` pushes ago: 1 is the last. */
	float tap(size_t n) const {
		return buffer[(at - n) & mask];
	}

	/** Between taps, by Hermite interpolation; `n` from 1 to longest(). */
	float read(float n) const {
		float whole = std::floor(n), t = n - whole;
		size_t i = (size_t) whole;
		float y0 = tap(i - 1), y1 = tap(i), y2 = tap(i + 1), y3 = tap(i + 2);
		float c1 = 0.5f * (y2 - y0), c2 = y0 - 2.5f * y1 + 2.f * y2 - 0.5f * y3, c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
		return ((c3 * t + c2) * t + c1) * t + y1;
	}

	/** A Schroeder allpass of `n` samples around this line: the input goes in with `g` of what comes out fed back. */
	float allpass(float x, float n, float g) {
		float delayed = n == std::floor(n) ? tap((size_t) n) : read(n);
		float v = x + g * delayed;
		push(v);
		return delayed - g * v;
	}

	void clear() {
		std::fill(buffer.begin(), buffer.end(), 0.f);
	}
};

/** A one-pole low-pass, as a share `a` of the way to the input each sample. */
struct OnePole {
	float y = 0.f;

	float process(float x, float a) {
		return y += a * (x - y);
	}
};

} // namespace xyz
