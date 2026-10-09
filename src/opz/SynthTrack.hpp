#pragma once
#include "../plugin.hpp"
#include "../ui.hpp"
#include "../dsp/Adsr.hpp"
#include "../dsp/Svf.hpp"
#include "Lfo.hpp"
#include "NoteStack.hpp"
#include <atomic>

/*
An OP-Z synth track, whatever its engine: four dials that step through four pages (sound, envelope, LFO, mix) plus
the track settings, note styles, portamento, polyphony, and a two-octave keyboard whose white keys double as quick
slots. The engine supplies only the sound, through two dials of its own. It is a struct with:

	static constexpr const char* SLUG;                     // module slug, and the name of its panel in res/
	static void configSound(Module* m, int p1, int p2);    // configures its two sound dials
	struct Voice {
		float process(float frequency, float sampleTime, float p1, float p2);  // one sample, about ±1
	};

Voices are summed before the filter: the filter has one cutoff for the whole track and is linear, so filtering
the mix sounds exactly like filtering each voice, for a fraction of the work.
*/
namespace opz {

enum Style { POLY, MONO, LEGATO, STYLES_LEN };
enum Target { TARGET_P1, TARGET_P2, TARGET_FILTER, TARGET_RESONANCE, TARGET_ATTACK, TARGET_PITCH, TARGET_PAN, TARGET_VOLUME, TARGETS_LEN };

/** Envelope dials run exponentially from 1 ms to 10 s. */
inline float envelopeTime(float dial) {
	return 0.001f * std::pow(10000.f, dial);
}

/** The filter dial sweeps a low-pass up from 20 Hz to 20 kHz across its left half, then a high-pass across its
right half. In the middle both are out of the way. */
inline float lowpassHz(float dial) {
	return 20.f * std::pow(1000.f, std::fmin(2.f * dial, 1.f));
}

inline float highpassHz(float dial) {
	return 20.f * std::pow(1000.f, std::fmax(2.f * dial - 1.f, 0.f));
}

/** Linear up to ±5V, then bends smoothly towards ±10V, so stacked voices and screaming resonance stay within Rack's
range without colouring normal levels. */
inline float softLimit(float v) {
	float a = std::fabs(v);
	return a <= 5.f ? v : std::copysign(5.f + 5.f * std::tanh((a - 5.f) / 5.f), v);
}

inline std::string formatHz(float hz) {
	return hz < 1000.f ? string::f("%.0f Hz", hz) : string::f("%.2f kHz", hz / 1000.f);
}

struct TimeQuantity : ParamQuantity {
	float getDisplayValue() override {
		return envelopeTime(getValue());
	}
	void setDisplayValue(float seconds) override {
		if (seconds > 0.f)
			setValue(std::log(seconds / 0.001f) / std::log(10000.f));
	}
	std::string getDisplayValueString() override {
		float s = getDisplayValue();
		return s < 1.f ? string::f("%.1f ms", 1000.f * s) : string::f("%.2f s", s);
	}
	/** Typed values are seconds, or milliseconds when they say "ms". */
	void setDisplayValueString(std::string text) override {
		float v = std::strtof(text.c_str(), nullptr);
		setDisplayValue(text.find("ms") != std::string::npos ? v / 1000.f : v);
	}
};

struct FilterQuantity : ParamQuantity {
	std::string getDisplayValueString() override {
		float v = getValue();
		if (v < 0.5f)
			return "Low-pass " + formatHz(lowpassHz(v));
		if (v > 0.5f)
			return "High-pass " + formatHz(highpassHz(v));
		return "Open";
	}
};

struct SpeedQuantity : ParamQuantity {
	std::string getDisplayValueString() override {
		float v = getValue();
		if (Lfo::synced(v))
			return Lfo::SYNCED_NAMES[Lfo::syncedIndex(v)];
		return string::f("%.2f Hz", 1.f / Lfo::period(v, 0.f));
	}
};


/** The keyboard on the panel: two octaves, C3 to B4 until the octave keys move it. */
constexpr int KEYS = 24;
/** Its white keys, which double as the quick slots in track mode. */
constexpr int SLOTS = 14;
constexpr int WHITE_KEYS[7] = {0, 2, 4, 5, 7, 9, 11};
inline int slotKey(int slot) {
	return 12 * (slot / 7) + WHITE_KEYS[slot % 7];
}
inline int keySlot(int key) {
	const int* k = std::find(std::begin(WHITE_KEYS), std::end(WHITE_KEYS), key % 12);
	return k == std::end(WHITE_KEYS) ? -1 : 7 * (key / 12) + int(k - WHITE_KEYS);
}

/** The pages the four dials step through, then the track settings, which take over the dials in track mode. */
enum Page { PAGE_SOUND, PAGE_ENVELOPE, PAGE_LFO, PAGE_MIX, PAGE_TRACK, PAGES_LEN };

/** A jack's LED: green follows the signal's peaks, red lights for a moment after it clips. */
struct Meter {
	float peak = 0.f, clip = 0.f;

	void process(float level, bool clipping, float dt) {
		peak = std::fmax(level, peak - 2.f * dt);
		clip = clipping ? 0.25f : std::fmax(clip - dt, 0.f);
	}
};


template <class Engine>
struct SynthTrack : Module {
	enum ParamId {
		P1_PARAM, P2_PARAM, FILTER_PARAM, RESONANCE_PARAM,
		ATTACK_PARAM, DECAY_PARAM, SUSTAIN_PARAM, RELEASE_PARAM,
		LFO_AMOUNT_PARAM, LFO_SPEED_PARAM, LFO_TARGET_PARAM, LFO_SHAPE_PARAM,
		FX1_PARAM, FX2_PARAM, PAN_PARAM, LEVEL_PARAM,
		STYLE_PARAM, GLIDE_PARAM,
		PAGE_PARAM, ENUMS(SELECT_PARAM, PAGES_LEN), OCTAVE_DOWN_PARAM, OCTAVE_UP_PARAM, ENUMS(KEY_PARAM, KEYS),
		PARAMS_LEN
	};
	enum InputId { VOCT_INPUT, GATE_INPUT, VELOCITY_INPUT, CLOCK_INPUT, RESET_INPUT, EXT_INPUT, INPUTS_LEN };
	enum OutputId { LEFT_OUTPUT, RIGHT_OUTPUT, FX1_OUTPUT, FX2_OUTPUT, OUTPUTS_LEN };
	enum LightId {
		ENUMS(DIAL_LIGHT, 4 * PAGES_LEN), ENUMS(PAGE_LIGHT, PAGES_LEN), ENUMS(SELECT_LIGHT, PAGES_LEN),
		ENUMS(KEY_LIGHT, KEYS), ENUMS(VOICE_LIGHT, PORT_MAX_CHANNELS), OCTAVE_DOWN_LIGHT, OCTAVE_UP_LIGHT,
		ENUMS(INPUT_LIGHT, 2 * INPUTS_LEN), ENUMS(OUTPUT_LIGHT, 2 * OUTPUTS_LEN),
		LIGHTS_LEN
	};

	/** What each dial does on each page, or -1 for nothing. On the hardware the track settings' first and third
	dials set note length and quantize, which belong to the sequencer. */
	static constexpr int DIALS[PAGES_LEN][4] = {
		{P1_PARAM, P2_PARAM, FILTER_PARAM, RESONANCE_PARAM},
		{ATTACK_PARAM, DECAY_PARAM, SUSTAIN_PARAM, RELEASE_PARAM},
		{LFO_AMOUNT_PARAM, LFO_SPEED_PARAM, LFO_TARGET_PARAM, LFO_SHAPE_PARAM},
		{FX1_PARAM, FX2_PARAM, PAN_PARAM, LEVEL_PARAM},
		{-1, STYLE_PARAM, -1, GLIDE_PARAM},
	};
	static constexpr int PPQN[] = {1, 2, 4, 8, 12, 16, 24, 48, 96};
	static constexpr float FLASH = 0.3f; // seconds the dial LEDs flash a new page's colour

	struct Voice {
		typename Engine::Voice sound;
		xyz::Adsr envelope;
		float pitch = 0.f; // V/oct, after portamento
		float velocity = 1.f;
		int note = -1;     // the gate channel or key holding it, in poly
		uint32_t started = 0;
	};

	/** The dials after the LFO, worked out at control rate. */
	struct Controls {
		float p1 = 0.f, p2 = 0.f;
		float pitch = 0.f;    // LFO offset, V/oct
		float glide = 1.f;    // portamento smoothing per sample; 1 jumps straight to the note
		float lfoCycles = 0.f;
		float left = 0.f, right = 0.f, gain = 0.f, fx1 = 0.f, fx2 = 0.f;
		xyz::Adsr::Shape envelope;
	};

	/** Shows each key's name, or in track mode what its slot does. */
	struct KeyQuantity : SwitchQuantity {
		std::string getLabel() override {
			static const char* const names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
			SynthTrack* track = static_cast<SynthTrack*>(module);
			int key = paramId - KEY_PARAM;
			if (track->trackMode && keySlot(key) >= 0)
				return string::f("Slot %d: click to load, hold to save", keySlot(key) + 1);
			return string::f("%s%d", names[key % 12], 3 + track->octave + key / 12);
		}
	};

	Voice voices[PORT_MAX_CHANNELS];
	uint32_t notesPlayed = 0;
	dsp::SchmittTrigger gates[PORT_MAX_CHANNELS];
	NoteStack held;
	int style = POLY;
	bool keyboard = false;         // the keys play the track, because GATE is unpatched
	bool keysDown[KEYS] = {};
	float keyPitch[KEYS] = {};     // where each key's note started, in case the octave moves while it's held
	Lfo lfo;
	float lfoValue = 0.f;
	xyz::Svf lowpass, highpass;
	Controls controls;
	dsp::ClockDivider controlDivider, lightDivider;
	dsp::SchmittTrigger clock, reset;
	float sinceClock = 0.f;
	float secondsPerBeat = 0.5f; // 120 BPM until a clock says otherwise
	int ppqn = 4;                // clock pulses per beat: 4 is one per sixteenth, one per step

	int page = PAGE_SOUND;
	bool trackMode = false;
	int octave = 0;
	float flash = 0.f;
	dsp::BooleanTrigger pageButton, selectButtons[PAGES_LEN], octaveDown, octaveUp;
	// The quick slots, which the widget loads and saves; the module only lights them.
	std::atomic<int> slotsFilled{0}, slot{-1};
	std::atomic<bool> slotSaved{false};
	float slotFlash = 0.f;
	Meter inputMeters[INPUTS_LEN], outputMeters[OUTPUTS_LEN];
	float outputPeaks[OUTPUTS_LEN] = {};
	bool limiting = false;

	SynthTrack() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		Engine::configSound(this, P1_PARAM, P2_PARAM);
		configParam<FilterQuantity>(FILTER_PARAM, 0.f, 1.f, 0.5f, "Filter")->description = "Low-pass to the left, high-pass to the right";
		configParam(RESONANCE_PARAM, 0.f, 1.f, 0.f, "Resonance", "%", 0.f, 100.f);
		configParam<TimeQuantity>(ATTACK_PARAM, 0.f, 1.f, 0.f, "Attack");
		configParam<TimeQuantity>(DECAY_PARAM, 0.f, 1.f, 0.5f, "Decay");
		configParam(SUSTAIN_PARAM, 0.f, 1.f, 0.7f, "Sustain", "%", 0.f, 100.f);
		configParam<TimeQuantity>(RELEASE_PARAM, 0.f, 1.f, 0.5f, "Release");
		configParam(LFO_AMOUNT_PARAM, -1.f, 1.f, 0.f, "LFO amount", "%", 0.f, 100.f)->description = "Off in the middle; to the left the LFO pulls the target the other way";
		configParam<SpeedQuantity>(LFO_SPEED_PARAM, 0.f, 1.f, 0.75f, "LFO speed")->description = "Synced to the clock to the left, free to the right";
		configSwitch(LFO_TARGET_PARAM, 0.f, TARGETS_LEN - 1, TARGET_FILTER, "LFO target",
			{getParamQuantity(P1_PARAM)->name, getParamQuantity(P2_PARAM)->name, "Filter", "Resonance", "Attack", "Pitch", "Pan", "Volume"});
		configSwitch(LFO_SHAPE_PARAM, 0.f, Lfo::SHAPES_LEN - 1, Lfo::SINE, "LFO shape",
			{"Sine", "Triangle", "Square", "Saw", "Random", "External (EXT input)",
			 "Bell, per note", "Triangle, per note", "Square, per note", "Saw, per note", "Random, per note", "Single saw, per note"});
		configParam(FX1_PARAM, 0.f, 1.f, 0.f, "FX 1 send", "%", 0.f, 100.f)->description = "To the FX 1 output, after the level";
		configParam(FX2_PARAM, 0.f, 1.f, 0.f, "FX 2 send", "%", 0.f, 100.f)->description = "To the FX 2 output, after the level";
		configParam(PAN_PARAM, -1.f, 1.f, 0.f, "Pan", "%", 0.f, 100.f);
		configParam(LEVEL_PARAM, 0.f, 1.f, 0.7f, "Level", "%", 0.f, 100.f);
		configSwitch(STYLE_PARAM, 0.f, STYLES_LEN - 1, POLY, "Note style", {"Poly", "Mono", "Legato"})->description =
			"Mono plays the newest held note and retriggers on every change; legato slides between held notes without retriggering";
		configParam(GLIDE_PARAM, 0.f, 1.f, 0.f, "Portamento", "%", 0.f, 100.f)->description = "Mono and legato only";

		configButton(PAGE_PARAM, "Page")->description = "Steps the dials through the sound, envelope, LFO and mix pages";
		const char* const pages[] = {"Sound page", "Envelope page", "LFO page", "Mix page", "Track mode"};
		for (int i = 0; i < PAGES_LEN; i++)
			configButton(SELECT_PARAM + i, pages[i]);
		getParamQuantity(SELECT_PARAM + PAGE_TRACK)->description = "On and off: the dials set the note style and portamento, and the white keys are quick slots";
		configButton(OCTAVE_DOWN_PARAM, "Keyboard octave down");
		configButton(OCTAVE_UP_PARAM, "Keyboard octave up");
		for (int i = 0; i < KEYS; i++)
			configButton<KeyQuantity>(KEY_PARAM + i)->description = "Plays while GATE is unpatched";
		for (int i = 0; i < PORT_MAX_CHANNELS; i++)
			configLight(VOICE_LIGHT + i, string::f("Voice %d", i + 1));

		configInput(VOCT_INPUT, "1V/octave pitch");
		configInput(GATE_INPUT, "Gate; the keys play while it's unpatched");
		configInput(VELOCITY_INPUT, "Velocity (0-10V)");
		configInput(CLOCK_INPUT, "Clock, for the synced LFO speeds");
		configInput(RESET_INPUT, "LFO reset");
		configInput(EXT_INPUT, "LFO external (±5V), for the External shape");
		configOutput(LEFT_OUTPUT, "Left, or mono while right is unpatched");
		configOutput(RIGHT_OUTPUT, "Right");
		configOutput(FX1_OUTPUT, "FX 1 send");
		configOutput(FX2_OUTPUT, "FX 2 send");

		controlDivider.setDivision(16);
		lightDivider.setDivision(64);
	}

	/** The page the dials are showing: one of the four, or the track settings. */
	int shownPage() const {
		return trackMode ? PAGE_TRACK : page;
	}

	void showPage(int p) {
		page = p;
		trackMode = false;
		flash = FLASH;
	}

	void setTrackMode(bool on) {
		trackMode = on;
		flash = FLASH;
	}

	/** Called by the widget when it loads or saves a quick slot. */
	void showSlot(int s, bool saved) {
		slot = s;
		if (saved)
			slotSaved = true;
	}

	float velocity(int channel) {
		return inputs[VELOCITY_INPUT].isConnected() ? clamp(inputs[VELOCITY_INPUT].getPolyVoltage(channel) / 10.f, 0.f, 1.f) : 1.f;
	}

	float noteVelocity(int note) {
		return keyboard ? 1.f : velocity(note);
	}

	float notePitch(int note) {
		return keyboard ? keyPitch[note] : inputs[VOCT_INPUT].getPolyVoltage(note);
	}

	void trigger(Voice& v, float velocity) {
		v.velocity = velocity;
		v.envelope.gate(true);
		v.started = ++notesPlayed;
		if (Lfo::triggered((int) params[LFO_SHAPE_PARAM].getValue()))
			lfo.restart();
	}

	/** A voice for a key: a silent one, else the quietest of those releasing, else the one playing longest. */
	int freeVoice() {
		int best = -1;
		for (int i = 0; i < PORT_MAX_CHANNELS; i++)
			if (!voices[i].envelope.active())
				return i;
		for (int i = 0; i < PORT_MAX_CHANNELS; i++)
			if (voices[i].note < 0 && (best < 0 || voices[i].envelope.level < voices[best].envelope.level))
				best = i;
		if (best >= 0)
			return best;
		for (int i = 0; i < PORT_MAX_CHANNELS; i++)
			if (best < 0 || voices[i].started < voices[best].started)
				best = i;
		return best;
	}

	/** Poly plays each note on a voice of its own: a gate channel on the voice with its number, a key on whichever is
	free. Mono and legato play the newest held note on voice 0. */
	void noteOn(int note) {
		if (style == POLY) {
			Voice& v = voices[keyboard ? freeVoice() : note];
			v.note = note;
			if (keyboard)
				v.pitch = keyPitch[note];
			trigger(v, noteVelocity(note));
		}
		else {
			bool slur = style == LEGATO && !held.empty();
			held.push(note);
			if (!slur)
				trigger(voices[0], noteVelocity(note));
		}
	}

	void noteOff(int note) {
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
			trigger(voices[0], noteVelocity(held.top()));
	}

	void releaseAll() {
		held.clear();
		for (Voice& v : voices) {
			v.envelope.gate(false);
			v.note = -1;
		}
	}

	void handleNotes() {
		int newStyle = (int) params[STYLE_PARAM].getValue();
		bool newKeyboard = !inputs[GATE_INPUT].isConnected();
		if (newStyle != style || newKeyboard != keyboard) {
			style = newStyle;
			keyboard = newKeyboard;
			releaseAll();
		}
		if (keyboard)
			return;
		// Channels beyond the gate input's count read as low, so dropping channels releases their notes.
		int channels = inputs[GATE_INPUT].getChannels();
		for (int c = 0; c < PORT_MAX_CHANNELS; c++) {
			switch (gates[c].processEvent(c < channels ? inputs[GATE_INPUT].getVoltage(c) : 0.f, 0.1f, 1.f)) {
				case dsp::SchmittTrigger::TRIGGERED: noteOn(c); break;
				case dsp::SchmittTrigger::UNTRIGGERED: noteOff(c); break;
				default: break;
			}
		}
	}

	/** The panel's buttons and keys, at control rate. In track mode the keys are quick slots, which the widget
	handles, so they only let go of their notes. */
	void handleButtons(float dt) {
		if (pageButton.process(params[PAGE_PARAM].getValue() > 0.f))
			showPage(trackMode ? page : (page + 1) % PAGE_TRACK);
		for (int i = 0; i < PAGES_LEN; i++) {
			if (selectButtons[i].process(params[SELECT_PARAM + i].getValue() > 0.f)) {
				if (i == PAGE_TRACK)
					setTrackMode(!trackMode);
				else
					showPage(i);
			}
		}
		if (octaveDown.process(params[OCTAVE_DOWN_PARAM].getValue() > 0.f))
			octave = std::max(octave - 1, -3);
		if (octaveUp.process(params[OCTAVE_UP_PARAM].getValue() > 0.f))
			octave = std::min(octave + 1, 3);
		flash = std::fmax(flash - dt, 0.f);

		for (int k = 0; k < KEYS; k++) {
			bool down = params[KEY_PARAM + k].getValue() > 0.f;
			if (down == keysDown[k])
				continue;
			keysDown[k] = down;
			if (!keyboard)
				continue;
			if (!down)
				noteOff(k);
			else if (!trackMode) {
				keyPitch[k] = octave - 1.f + k / 12.f;
				noteOn(k);
			}
		}
	}

	void trackTempo(float sampleTime) {
		sinceClock += sampleTime;
		if (clock.process(inputs[CLOCK_INPUT].getVoltage(), 0.1f, 1.f)) {
			// The first pulse after a long gap only starts the count again.
			if (sinceClock * ppqn < 4.f)
				secondsPerBeat = sinceClock * ppqn;
			sinceClock = 0.f;
		}
	}

	/** A dial's value, moved by the LFO when the LFO targets it. Its range is the LFO's full swing. */
	float modulated(int param, int target, float lfoOffset) {
		float v = params[param].getValue();
		if ((int) params[LFO_TARGET_PARAM].getValue() != target)
			return v;
		ParamQuantity* q = paramQuantities[param];
		v = clamp(v + lfoOffset * (q->maxValue - q->minValue), q->minValue, q->maxValue);
		return q->snapEnabled ? std::round(v) : v;
	}

	void updateControls(float sampleTime) {
		float amount = params[LFO_AMOUNT_PARAM].getValue();
		// Squaring the amount leaves room for subtle settings, like vibrato, near the middle.
		float offset = amount * std::fabs(amount) * lfoValue;
		Controls& c = controls;
		c.p1 = modulated(P1_PARAM, TARGET_P1, offset);
		c.p2 = modulated(P2_PARAM, TARGET_P2, offset);
		c.pitch = (int) params[LFO_TARGET_PARAM].getValue() == TARGET_PITCH ? 2.f * offset : 0.f;

		// The resonance belongs to whichever filter the dial is moving; the other one stays flat.
		float filter = modulated(FILTER_PARAM, TARGET_FILTER, offset);
		float damping = 2.f - 1.9f * modulated(RESONANCE_PARAM, TARGET_RESONANCE, offset); // Q from 0.5 to 10
		lowpass.tune(lowpassHz(filter) * sampleTime, filter < 0.5f ? damping : float(M_SQRT2));
		highpass.tune(highpassHz(filter) * sampleTime, filter > 0.5f ? damping : float(M_SQRT2));

		c.envelope.set(envelopeTime(modulated(ATTACK_PARAM, TARGET_ATTACK, offset)), envelopeTime(params[DECAY_PARAM].getValue()),
			params[SUSTAIN_PARAM].getValue(), envelopeTime(params[RELEASE_PARAM].getValue()), sampleTime);
		float glide = params[GLIDE_PARAM].getValue();
		c.glide = glide > 0.f ? xyz::Adsr::Shape::coefficient(glide * glide, sampleTime) : 1.f;
		c.lfoCycles = sampleTime / Lfo::period(params[LFO_SPEED_PARAM].getValue(), secondsPerBeat);

		float pan = modulated(PAN_PARAM, TARGET_PAN, offset);
		float level = modulated(LEVEL_PARAM, TARGET_VOLUME, offset);
		c.left = std::cos((pan + 1.f) * float(M_PI) / 4.f);
		c.right = std::sin((pan + 1.f) * float(M_PI) / 4.f);
		c.gain = 5.f * level * level;
		c.fx1 = params[FX1_PARAM].getValue() * params[FX1_PARAM].getValue();
		c.fx2 = params[FX2_PARAM].getValue() * params[FX2_PARAM].getValue();
	}

	void process(const ProcessArgs& args) override {
		trackTempo(args.sampleTime);
		if (reset.process(inputs[RESET_INPUT].getVoltage(), 0.1f, 1.f))
			lfo.restart();
		bool control = controlDivider.process();
		if (control)
			handleButtons(args.sampleTime * controlDivider.getDivision());
		handleNotes();
		lfoValue = lfo.process((int) params[LFO_SHAPE_PARAM].getValue(), controls.lfoCycles, inputs[EXT_INPUT].getVoltage() / 5.f);
		if (control)
			updateControls(args.sampleTime);

		if (style != POLY) {
			Voice& v = voices[0];
			if (!held.empty())
				v.pitch += (notePitch(held.top()) - v.pitch) * controls.glide;
		}
		// Every voice runs, so voices left releasing by a change of note style fade out normally.
		float mix = 0.f;
		for (int i = 0; i < PORT_MAX_CHANNELS; i++) {
			Voice& v = voices[i];
			if (!v.envelope.active())
				continue;
			if (style == POLY && !keyboard)
				v.pitch = inputs[VOCT_INPUT].getPolyVoltage(i);
			float frequency = dsp::FREQ_C4 * dsp::exp2_taylor5(v.pitch + controls.pitch);
			float level = v.envelope.process(controls.envelope) * v.velocity;
			mix += v.sound.process(frequency, args.sampleTime, controls.p1, controls.p2) * level;
		}

		float x = highpass.process(lowpass.process(mix).lp).hp * controls.gain;
		float y = softLimit(x);
		float out[OUTPUTS_LEN] = {y, 0.f, y * controls.fx1, y * controls.fx2};
		if (outputs[RIGHT_OUTPUT].isConnected()) {
			out[LEFT_OUTPUT] = y * controls.left;
			out[RIGHT_OUTPUT] = y * controls.right;
		}
		for (int i = 0; i < OUTPUTS_LEN; i++) {
			outputs[i].setVoltage(out[i]);
			outputPeaks[i] = std::fmax(outputPeaks[i], std::fabs(out[i]));
		}
		limiting |= std::fabs(x) > 5.f;

		if (lightDivider.process())
			updateLights(args.sampleTime * lightDivider.getDivision());
	}

	void updateLights(float dt) {
		// The dial LEDs flash the page's colour when it changes, then show their dial's value in it.
		int shown = shownPage();
		for (int d = 0; d < 4; d++) {
			int param = DIALS[shown][d];
			float b = param < 0 ? 0.f : flash > 0.f ? 1.f : 0.15f + 0.85f * paramQuantities[param]->getScaledValue();
			for (int p = 0; p < PAGES_LEN; p++)
				lights[DIAL_LIGHT + PAGES_LEN * d + p].setBrightness(p == shown ? b : 0.f);
		}
		for (int p = 0; p < PAGES_LEN; p++) {
			lights[PAGE_LIGHT + p].setBrightness(p == shown);
			lights[SELECT_LIGHT + p].setBrightness(p == shown);
		}
		lights[OCTAVE_DOWN_LIGHT].setBrightness(octave < 0 ? 0.2f - 0.2f * octave : 0.f);
		lights[OCTAVE_UP_LIGHT].setBrightness(octave > 0 ? 0.2f + 0.2f * octave : 0.f);

		// The keys light up under held notes, or in track mode show which slots are filled and which was used last.
		float keys[KEYS] = {};
		if (slotSaved.exchange(false))
			slotFlash = 0.6f;
		slotFlash = std::fmax(slotFlash - dt, 0.f);
		if (trackMode) {
			int filled = slotsFilled, current = slot;
			for (int s = 0; s < SLOTS; s++) {
				float b = (filled >> s) & 1 ? 0.3f : 0.f;
				if (s == current)
					b = slotFlash > 0.f && std::fmod(slotFlash, 0.2f) < 0.1f ? 0.f : 1.f;
				keys[slotKey(s)] = b;
			}
		}
		else if (keyboard) {
			for (int k = 0; k < KEYS; k++)
				keys[k] = keysDown[k];
		}
		else {
			for (int c = 0; c < inputs[GATE_INPUT].getChannels(); c++) {
				int k = (int) std::round((inputs[VOCT_INPUT].getPolyVoltage(c) - octave + 1.f) * 12.f);
				if (gates[c].isHigh() && k >= 0 && k < KEYS)
					keys[k] = 1.f;
			}
		}
		for (int k = 0; k < KEYS; k++)
			lights[KEY_LIGHT + k].setBrightness(keys[k]);
		for (int i = 0; i < PORT_MAX_CHANNELS; i++)
			lights[VOICE_LIGHT + i].setBrightnessSmooth(voices[i].envelope.level * voices[i].velocity, dt);

		for (int i = 0; i < INPUTS_LEN; i++) {
			float level = 0.f;
			for (int c = 0; c < inputs[i].getChannels(); c++)
				level = std::fmax(level, std::fabs(inputs[i].getVoltage(c)));
			inputMeters[i].process(level / 10.f, level > 10.f, dt);
		}
		for (int i = 0; i < OUTPUTS_LEN; i++) {
			outputMeters[i].process(outputPeaks[i] / 5.f, limiting && outputPeaks[i] > 0.f, dt);
			outputPeaks[i] = 0.f;
		}
		limiting = false;
		for (int i = 0; i < INPUTS_LEN; i++) {
			lights[INPUT_LIGHT + 2 * i].setBrightness(inputMeters[i].peak);
			lights[INPUT_LIGHT + 2 * i + 1].setBrightness(inputMeters[i].clip > 0.f);
		}
		for (int i = 0; i < OUTPUTS_LEN; i++) {
			lights[OUTPUT_LIGHT + 2 * i].setBrightness(outputMeters[i].peak);
			lights[OUTPUT_LIGHT + 2 * i + 1].setBrightness(outputMeters[i].clip > 0.f);
		}
	}

	void onReset(const ResetEvent& e) override {
		Module::onReset(e);
		ppqn = 4;
		page = PAGE_SOUND;
		trackMode = false;
		octave = 0;
	}

	/** Buttons are saved like any param, so one held down while saving would come back stuck. */
	void paramsFromJson(json_t* root) override {
		Module::paramsFromJson(root);
		for (int i = PAGE_PARAM; i < PARAMS_LEN; i++)
			params[i].setValue(0.f);
	}

	/** Track mode isn't saved: the quick slots are saved in it, and loading one shouldn't switch it on. */
	json_t* dataToJson() override {
		json_t* root = json_object();
		json_object_set_new(root, "ppqn", json_integer(ppqn));
		json_object_set_new(root, "page", json_integer(page));
		json_object_set_new(root, "octave", json_integer(octave));
		return root;
	}

	void dataFromJson(json_t* root) override {
		if (json_t* j = json_object_get(root, "ppqn"))
			ppqn = std::max((int) json_integer_value(j), 1);
		if (json_t* j = json_object_get(root, "page"))
			page = clamp((int) json_integer_value(j), 0, PAGE_TRACK - 1);
		if (json_t* j = json_object_get(root, "octave"))
			octave = clamp((int) json_integer_value(j), -3, 3);
	}
};


} // namespace opz
