#pragma once
#include <cmath>

namespace xyz {

/** ADSR envelope with exponential segments, like an analog one. Each segment starts from wherever the level is,
so a retrigger mid-release swells back up instead of clicking to zero. */
struct Adsr {
	enum Stage { IDLE, ATTACK, DECAY, RELEASE };

	/** Per-sample smoothing coefficients and the sustain level, shared by every voice of a track. */
	struct Shape {
		float attack = 1.f, decay = 1.f, sustain = 1.f, release = 1.f;

		/** Times in seconds. Attack is the rise from 0 to full, decay and release the fall to within 1% of their
		target. */
		void set(float attackTime, float decayTime, float sustainLevel, float releaseTime, float sampleTime) {
			// The attack aims past full level, so it reaches 1 in finite time with an analog-style curve.
			attack = coefficient(attackTime / std::log(OVERSHOOT / (OVERSHOOT - 1.f)), sampleTime);
			decay = coefficient(decayTime / std::log(100.f), sampleTime);
			release = coefficient(releaseTime / std::log(100.f), sampleTime);
			sustain = sustainLevel;
		}

		static float coefficient(float tau, float sampleTime) {
			return 1.f - std::exp(-sampleTime / std::fmax(tau, 1e-5f));
		}
	};

	static constexpr float OVERSHOOT = 1.2f;

	Stage stage = IDLE;
	float level = 0.f;

	void gate(bool on) {
		if (on)
			stage = ATTACK;
		else if (stage != IDLE)
			stage = RELEASE;
	}

	bool active() const {
		return stage != IDLE;
	}

	float process(const Shape& s) {
		switch (stage) {
			case ATTACK:
				level += (OVERSHOOT - level) * s.attack;
				if (level >= 1.f) {
					level = 1.f;
					stage = DECAY;
				}
				break;
			case DECAY:
				level += (s.sustain - level) * s.decay;
				break;
			case RELEASE:
				level -= level * s.release;
				if (level < 1e-4f) {
					level = 0.f;
					stage = IDLE;
				}
				break;
			case IDLE:
				break;
		}
		return level;
	}
};

} // namespace xyz
