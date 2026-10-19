#pragma once
#include "Track.hpp"
#include "NoteStack.hpp"

/*
An OP-Z synth track (Track.hpp), whatever its engine: note styles, portamento and polyphony. The engine supplies
only the sound, through two dials of its own. It is a struct with:

	static constexpr const char* SLUG;                     // module slug, and the name of its panel in res/
	static void configSound(Module* m, int p1, int p2);    // configures its two sound dials
	struct Voice {
		// One sample, about ±1. `envelope` is the voice's amp envelope, 0 to 1, for an engine with a filter envelope.
		float process(float frequency, float sampleTime, float p1, float p2, float envelope);
	};

Voices are summed before the filter: the filter has one cutoff for the whole track and is linear, so filtering
the mix sounds exactly like filtering each voice, for a fraction of the work.
*/
namespace opz {

enum Style { POLY, MONO, LEGATO, STYLES_LEN };

/** An engine's optional SOUND_SINCE: the version its two dials took their present meaning, so older patches reset
them rather than load a value meant for something else. */
template <class Engine, class = void>
struct SoundSince {
	static constexpr const char* value = nullptr;
};
template <class Engine>
struct SoundSince<Engine, std::void_t<decltype(Engine::SOUND_SINCE)>> {
	static constexpr const char* value = Engine::SOUND_SINCE;
};

template <class Engine>
struct SynthTrack : Track {

	struct Voice {
		typename Engine::Voice sound;
		xyz::Adsr envelope;
		float pitch = 0.f; // V/oct, after portamento
		float velocity = 1.f;
		int note = -1;     // the gate channel or key holding it, in poly
		uint32_t started = 0;
	};

	Voice voices[PORT_MAX_CHANNELS];
	NoteStack held;

	SynthTrack() {
		Engine::configSound(this, P1_PARAM, P2_PARAM);
		configTrack({"Poly", "Mono", "Legato"},
			"Mono plays the newest held note and retriggers on every change; legato slides between held notes without retriggering");
	}

	void fromJson(json_t* root) override {
		Track::fromJson(root);
		const char* version = json_string_value(json_object_get(root, "version"));
		if (SoundSince<Engine>::value && (!version || string::Version(version) < string::Version(SoundSince<Engine>::value))) {
			paramQuantities[P1_PARAM]->reset();
			paramQuantities[P2_PARAM]->reset();
		}
	}

	const xyz::Adsr* envelope(int voice, float* velocity) const override {
		if (voice >= PORT_MAX_CHANNELS)
			return nullptr;
		*velocity = voices[voice].velocity;
		return &voices[voice].envelope;
	}

	void trigger(Voice& v, float velocity) {
		v.velocity = velocity;
		v.envelope.gate(true);
		noteStarted();
		v.started = notesPlayed;
	}

	/** Poly plays each gate channel on the voice with its number. Mono and legato play the newest held note on voice 0. */
	void noteOn(int note) override {
		if (style == POLY) {
			voices[note].note = note;
			trigger(voices[note], velocity(note));
		}
		else {
			bool slur = style == LEGATO && !held.empty();
			held.push(note);
			if (!slur)
				trigger(voices[0], velocity(note));
		}
	}

	void noteOff(int note) override {
		if (style == POLY) {
			for (Voice& v : voices) {
				if (v.note == note) {
					v.envelope.gate(false);
					v.note = -1;
				}
			}
			return;
		}
		bool playing = !held.empty() && held.top() == note;
		held.remove(note);
		if (held.empty())
			voices[0].envelope.gate(false);
		else if (playing && style == MONO)
			trigger(voices[0], velocity(held.top()));
	}

	void releaseAll() override {
		held.clear();
		for (Voice& v : voices) {
			v.envelope.gate(false);
			v.note = -1;
		}
	}

	/** Track::prepare, then mono and legato's portamento. */
	void prepare(const ProcessArgs& args) {
		Track::prepare(args);
		if (style != POLY) {
			Voice& v = voices[0];
			if (!held.empty())
				v.pitch += (notePitch(held.top()) - v.pitch) * controls.glide;
		}
	}

	/** Steps a voice's envelope; its level, times its velocity. */
	float advance(Voice& v) {
		return v.envelope.process(controls.envelope) * v.velocity;
	}

	void process(const ProcessArgs& args) override {
		prepare(args);
		// Every voice runs, so voices left releasing by a change of note style fade out normally.
		float mix = 0.f;
		for (int i = 0; i < PORT_MAX_CHANNELS; i++) {
			Voice& v = voices[i];
			if (!v.envelope.active())
				continue;
			if (style == POLY)
				v.pitch = notePitch(i);
			float frequency = dsp::FREQ_C4 * dsp::exp2_taylor5(v.pitch + controls.pitch);
			float level = advance(v);
			mix += v.sound.process(frequency, args.sampleTime, controls.p1, controls.p2, v.envelope.level) * level;
		}
		float x = filter(mix);
		emit(x, x, controls.fx1, controls.fx2, args.sampleTime);
	}
};

} // namespace opz
