/*
Bow: the OP-Z's string synthesis, a bowed string as a waveguide (after Smith's and Cook's): two delay lines either
side of the bow, a damped bridge, and the bow's stick and slip as a friction curve. The bow's speed follows the
amp envelope, so a note swells in as the string catches and rings on after it lifts. TENSION is how hard the bow
presses and how bright the string is, from airy to biting; CHORUS adds a second string, detuned against the first.
*/
#include "opz/SynthTrackWidget.hpp"
#include <vector>

struct Bow {
	static constexpr const char* SLUG = "Bow";
	static constexpr int LINE = 8192;               // samples a delay line holds; the lowest note at 192 kHz needs 7500
	static constexpr float BOW_POINT = 0.127236f;   // the bow's place along the string, from the bridge

	static void configSound(Module* m, int p1, int p2) {
		m->configParam(p1, 0.f, 1.f, 0.5f, "Tension", "%", 0.f, 100.f)->description = "Bow pressure and the string's brightness";
		m->configParam(p2, 0.f, 1.f, 0.3f, "Chorus", "%", 0.f, 100.f)->description = "A second string, detuned against the first";
	}

	struct String {
		std::vector<float> neck = std::vector<float>(LINE), bridge = std::vector<float>(LINE);
		int at = 0;              // where the next sample goes in both lines
		float damped = 0.f;      // the bridge's one-pole low-pass

		float tap(const std::vector<float>& line, float delay) const {
			delay = std::fmax(delay, 1.f);
			int whole = int(delay);
			float f = delay - whole;
			return line[(at - whole) & (LINE - 1)] * (1.f - f) + line[(at - whole - 1) & (LINE - 1)] * f;
		}

		/** One sample of a string `period` samples long, bowed at `speed`, with the friction curve's `slope` and the
		bridge's `pole`, which delays the wave `lag` samples at the note. */
		float process(float period, float speed, float slope, float pole, float lag) {
			float length = clamp(period - lag, 2.f, LINE - 2.f);
			float fromBridge = tap(bridge, length * BOW_POINT), fromNeck = tap(neck, length * (1.f - BOW_POINT));
			damped = 0.95f * (1.f - pole) * fromBridge + pole * damped;
			float bridgeWave = -damped, neckWave = -fromNeck;
			float slip = speed - (bridgeWave + neckWave);
			float t = std::fabs(slip * slope) + 0.75f;
			t *= t;
			float push = slip * std::fmin(1.f / (t * t), 1.f);
			neck[at] = bridgeWave + push;
			bridge[at] = neckWave + push;
			if (!std::isfinite(damped)) {
				damped = 0.f;
				std::fill(neck.begin(), neck.end(), 0.f);
				std::fill(bridge.begin(), bridge.end(), 0.f);
			}
			at = (at + 1) & (LINE - 1);
			return fromBridge;
		}
	};

	struct Voice {
		String strings[2];

		float process(float frequency, float sampleTime, float tension, float chorus, float envelope) {
			// The bridge's low-pass corner rises with tension, and never sits below the note's eighth harmonic, or high
			// notes lose their fundamental and the string jumps an octave.
			float corner = std::fmax(1150.f + 6200.f * tension, 8.f * frequency);
			float pole = std::exp(-2.f * float(M_PI) * corner * sampleTime);
			float slope = 1.2f - 0.5f * tension, speed = 0.25f * envelope;  // steeper than about 1.5 and the string falls into its upper modes
			float period = 1.f / (std::fmax(frequency, 1.f) * sampleTime);
			// The bridge's low-pass delays the note by its phase over its frequency.
			float w = 2.f * float(M_PI) / period, lag = std::atan2(pole * std::sin(w), 1.f - pole * std::cos(w)) / w;
			if (chorus <= 0.f)
				return 1.4f * strings[0].process(period, speed, slope, pole, lag);
			// The two strings sit either side of the note, so the pair stays in tune as they spread.
			float spread = std::exp2((2.f + 7.f * chorus) / 1200.f);
			float x = strings[0].process(period * spread, speed, slope, pole, lag) + chorus * strings[1].process(period / spread, speed, slope, pole, lag);
			return 1.4f * x / (1.f + 0.5f * chorus);
		}
	};
};


Model* modelBow = createModel<opz::SynthTrack<Bow>, opz::SynthTrackWidget<Bow>>(Bow::SLUG);
