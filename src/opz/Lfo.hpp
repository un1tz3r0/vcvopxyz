#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace opz {

/** The synth track's LFO. The free-running shapes are bipolar and never restart. The triggered ones are unipolar,
0 to 1, and restart with every note, so they double as a second envelope. */
struct Lfo {
	enum Shape {
		SINE, TRIANGLE, SQUARE, SAW, RANDOM, EXTERNAL,
		BELL, TRIGGERED_TRIANGLE, TRIGGERED_SQUARE, TRIGGERED_SAW, TRIGGERED_RANDOM, SINGLE_SAW,
		SHAPES_LEN
	};

	/** The speed dial: tempo-synced periods, in whole notes, step across its left half. */
	static constexpr float SYNCED[] = {1 / 64.f, 1 / 32.f, 1 / 16.f, 1 / 8.f, 1 / 4.f, 1 / 2.f, 1.f, 2.f};
	static constexpr const char* SYNCED_NAMES[] = {"1/64", "1/32", "1/16", "1/8", "1/4", "1/2", "1/1", "2/1"};
	static constexpr int SYNCED_LEN = 8;
	/** Its right half sweeps freely between these periods, in seconds. */
	static constexpr float SLOWEST = 8.f, FASTEST = 0.05f;

	static bool triggered(int shape) {
		return shape >= BELL;
	}

	static bool synced(float speed) {
		return speed < 0.5f;
	}

	static int syncedIndex(float speed) {
		return std::clamp(int(speed * 2.f * SYNCED_LEN), 0, SYNCED_LEN - 1);
	}

	/** Seconds per cycle at this setting of the speed dial (0 to 1), at this tempo. */
	static float period(float speed, float secondsPerBeat) {
		if (synced(speed))
			return SYNCED[syncedIndex(speed)] * 4.f * secondsPerBeat;
		return SLOWEST * std::pow(FASTEST / SLOWEST, std::fmin(2.f * speed - 1.f, 1.f));
	}

	float phase = 0.f;
	float held = 0.f;   // the random shapes' current value, 0 to 1
	bool done = false;  // the single saw has run its course
	uint32_t seed = 0x9e3779b9u;

	void restart() {
		phase = 0.f;
		done = false;
		held = random();
	}

	/** Advances by `cycles` (frequency × sample time) and returns the shape's value. `external` is what the
	EXTERNAL shape follows, already scaled to [-1, 1]. */
	float process(int shape, float cycles, float external) {
		phase += cycles;
		if (phase >= 1.f) {
			phase -= std::floor(phase);
			held = random();
			done = true;
		}
		return value(shape, phase, held, done, external);
	}

	/** A shape's value at a phase, given the random shapes' current value and whether the single saw is done. */
	static float value(int shape, float p, float held, bool done, float external) {
		switch (shape) {
			case SINE: return std::sin(2.f * 3.14159265f * p);
			case TRIANGLE: return 1.f - 4.f * std::fabs(p + 0.25f - std::floor(p + 0.25f) - 0.5f);
			case SQUARE: return p < 0.5f ? 1.f : -1.f;
			case SAW: return 1.f - 2.f * p;
			case RANDOM: return 2.f * held - 1.f;
			case EXTERNAL: return std::clamp(external, -1.f, 1.f);
			case BELL: return 0.5f - 0.5f * std::cos(2.f * 3.14159265f * p);
			case TRIGGERED_TRIANGLE: return 1.f - std::fabs(2.f * p - 1.f);
			case TRIGGERED_SQUARE: return p < 0.5f ? 1.f : 0.f;
			case TRIGGERED_SAW: return 1.f - p;
			case TRIGGERED_RANDOM: return held;
			default: return done ? 0.f : 1.f - p;
		}
	}

	float random() {
		seed ^= seed << 13;
		seed ^= seed >> 17;
		seed ^= seed << 5;
		return (seed >> 8) * (1.f / 16777216.f);
	}
};

} // namespace opz
