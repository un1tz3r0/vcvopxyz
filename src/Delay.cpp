/*
Delay: the OP-Z's basic digital delay on an FX track. The keys set its time in sixty-fourth notes of the tempo, from
1/64 on the lowest key to 3/8 on the highest. AMOUNT is how much of each echo comes round again, CUTOFF how bright
the echoes stay as they do: a low-pass inside the loop darkens each repeat further.
*/
#include "opz/FxTrack.hpp"

struct Delay : opz::TempoTime {
	static constexpr const char* SLUG = "Delay";
	static constexpr float LONGEST = 6.f; // seconds: the highest key at 40 BPM, an octave longer by the LFO

	static void configSound(Module* m, int p1, int p2) {
		m->configParam(p1, 0.f, 1.f, 0.4f, "Amount", "%", 0.f, 100.f)->description = "Feedback: how much of each echo repeats";
		m->configParam(p2, 0.f, 1.f, 0.7f, "Cutoff", "%", 0.f, 100.f)->description = "How bright the repeats stay";
	}

	struct Unit {
		xyz::DelayLine lines[2];
		xyz::OnePole damping[2];
		float delay = 0.f; // samples, gliding to the time set, so a new time bends the echoes rather than clicking

		void prepare(float sampleRate) {
			for (xyz::DelayLine& l : lines)
				l.resize((size_t) (LONGEST * sampleRate));
			delay = 0.f;
		}

		void process(const float in[2], float out[2], const opz::FxControls& c) {
			float target = clamp(c.time * c.sampleRate, 1.f, lines[0].longest());
			delay = delay < 1.f ? target : delay + (target - delay) * std::fmin(c.sampleTime * 8.f, 1.f);
			float feedback = 0.98f * c.p1;
			float a = 1.f - std::exp(-2.f * float(M_PI) * 200.f * std::pow(100.f, c.p2) * c.sampleTime);
			for (int s = 0; s < 2; s++) {
				float echo = lines[s].read(delay);
				out[s] = echo;
				lines[s].push(in[s] + feedback * damping[s].process(echo, a));
			}
		}
	};
};


Model* modelDelay = createModel<opz::FxTrack<Delay>, opz::FxTrackWidget<Delay>>(Delay::SLUG);
