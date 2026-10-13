/*
Cluster: the OP-Z's clustered oscillators, seven saws per note spread around its pitch. GRAVITY pulls them
together, from a wide smear of almost two semitones each way to a single saw. TONE softens them, with a low-pass
that follows the note, from a near sine to fully bright.
*/
#include "opz/SynthTrackWidget.hpp"
#include "dsp/Blep.hpp"

struct Cluster {
	static constexpr const char* SLUG = "Cluster";
	static constexpr int SAWS = 7;
	/** Each saw's offset from the note, as a fraction of its frequency at no gravity: uneven, so they never line up. */
	static constexpr float SPREAD[SAWS] = {-0.11002313f, -0.06288439f, -0.01952356f, 0.f, 0.01991221f, 0.06216538f, 0.10745242f};
	static constexpr float LEVEL[SAWS] = {0.6f, 0.75f, 0.9f, 1.f, 0.9f, 0.75f, 0.6f};

	static void configSound(Module* m, int p1, int p2) {
		m->configParam(p1, 0.f, 1.f, 0.7f, "Tone", "%", 0.f, 100.f)->description = "From soft to bright";
		m->configParam(p2, 0.f, 1.f, 0.5f, "Gravity", "%", 0.f, 100.f)->description = "Pulls the detuned saws together";
	}

	struct Voice {
		float phases[SAWS];
		float low1 = 0.f, low2 = 0.f;

		Voice() {
			// Free-running saws don't start in step.
			for (float& p : phases)
				p = random::uniform();
		}

		float process(float frequency, float sampleTime, float tone, float gravity, float) {
			float spread = (1.f - gravity) * (1.f - gravity);
			float x = 0.f;
			for (int i = 0; i < SAWS; i++) {
				float increment = std::fmin(frequency * (1.f + SPREAD[i] * spread) * sampleTime, 0.5f);
				phases[i] += increment;
				phases[i] -= std::floor(phases[i]);
				x += LEVEL[i] * xyz::blepSaw(phases[i], increment);
			}
			// Two one-pole low-passes, from twice the note's frequency up to 256 times.
			float a = std::fmin(2.f * float(M_PI) * frequency * std::exp2(1.f + 7.f * tone) * sampleTime, 1.f);
			low1 += a * (x - low1);
			low2 += a * (low1 - low2);
			return 0.25f * low2;
		}
	};
};


Model* modelCluster = createModel<opz::SynthTrack<Cluster>, opz::SynthTrackWidget<Cluster>>(Cluster::SLUG);
