#pragma once
#include "../plugin.hpp"
#include "../ui.hpp"
#include "../dsp/Adsr.hpp"
#include "../dsp/Svf.hpp"
#include "Lfo.hpp"
#include "NoteStack.hpp"

/*
An OP-Z synth track, whatever its engine: four pages of four dials (sound, envelope, LFO, mix), note styles,
portamento and polyphony. The engine supplies only the sound, through two dials of its own. It is a struct with:

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


template <class Engine>
struct SynthTrack : Module {
	enum ParamId {
		P1_PARAM, P2_PARAM, FILTER_PARAM, RESONANCE_PARAM,
		ATTACK_PARAM, DECAY_PARAM, SUSTAIN_PARAM, RELEASE_PARAM,
		LFO_AMOUNT_PARAM, LFO_SPEED_PARAM, LFO_TARGET_PARAM, LFO_SHAPE_PARAM,
		STYLE_PARAM, GLIDE_PARAM, PAN_PARAM, LEVEL_PARAM,
		PARAMS_LEN
	};
	enum InputId { VOCT_INPUT, GATE_INPUT, VELOCITY_INPUT, CLOCK_INPUT, RESET_INPUT, EXT_INPUT, INPUTS_LEN };
	enum OutputId { LEFT_OUTPUT, RIGHT_OUTPUT, OUTPUTS_LEN };
	enum LightId { LIGHTS_LEN };

	static constexpr int PPQN[] = {1, 2, 4, 8, 12, 16, 24, 48, 96};

	struct Voice {
		typename Engine::Voice sound;
		xyz::Adsr envelope;
		float pitch = 0.f; // V/oct, after portamento
		float velocity = 1.f;
	};

	/** The dials after the LFO, worked out at control rate. */
	struct Controls {
		float p1 = 0.f, p2 = 0.f;
		float pitch = 0.f;    // LFO offset, V/oct
		float glide = 1.f;    // portamento smoothing per sample; 1 jumps straight to the note
		float lfoCycles = 0.f;
		float left = 0.f, right = 0.f, gain = 0.f;
		xyz::Adsr::Shape envelope;
	};

	Voice voices[PORT_MAX_CHANNELS];
	dsp::SchmittTrigger gates[PORT_MAX_CHANNELS];
	NoteStack held;
	int style = POLY;
	Lfo lfo;
	float lfoValue = 0.f;
	xyz::Svf lowpass, highpass;
	Controls controls;
	dsp::ClockDivider controlDivider;
	dsp::SchmittTrigger clock, reset;
	float sinceClock = 0.f;
	float secondsPerBeat = 0.5f; // 120 BPM until a clock says otherwise
	int ppqn = 4;                // clock pulses per beat: 4 is one per sixteenth, one per step

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
		configSwitch(STYLE_PARAM, 0.f, STYLES_LEN - 1, POLY, "Note style", {"Poly", "Mono", "Legato"})->description =
			"Mono plays the newest held note and retriggers on every change; legato slides between held notes without retriggering";
		configParam(GLIDE_PARAM, 0.f, 1.f, 0.f, "Portamento", "%", 0.f, 100.f)->description = "Mono and legato only";
		configParam(PAN_PARAM, -1.f, 1.f, 0.f, "Pan", "%", 0.f, 100.f);
		configParam(LEVEL_PARAM, 0.f, 1.f, 0.7f, "Level", "%", 0.f, 100.f);

		configInput(VOCT_INPUT, "1V/octave pitch");
		configInput(GATE_INPUT, "Gate");
		configInput(VELOCITY_INPUT, "Velocity (0-10V)");
		configInput(CLOCK_INPUT, "Clock, for the synced LFO speeds");
		configInput(RESET_INPUT, "LFO reset");
		configInput(EXT_INPUT, "LFO external (±5V), for the External shape");
		configOutput(LEFT_OUTPUT, "Left, or mono while right is unpatched");
		configOutput(RIGHT_OUTPUT, "Right");

		controlDivider.setDivision(16);
	}

	float velocity(int channel) {
		return inputs[VELOCITY_INPUT].isConnected() ? clamp(inputs[VELOCITY_INPUT].getPolyVoltage(channel) / 10.f, 0.f, 1.f) : 1.f;
	}

	void trigger(Voice& v, int channel) {
		v.velocity = velocity(channel);
		v.envelope.gate(true);
		if (Lfo::triggered((int) params[LFO_SHAPE_PARAM].getValue()))
			lfo.restart();
	}

	/** Poly plays each channel on its own voice. Mono and legato play the newest held note on voice 0. */
	void noteOn(int channel) {
		if (style == POLY) {
			trigger(voices[channel], channel);
		}
		else {
			bool slur = style == LEGATO && !held.empty();
			held.push(channel);
			if (!slur)
				trigger(voices[0], channel);
		}
	}

	void noteOff(int channel) {
		if (style == POLY) {
			voices[channel].envelope.gate(false);
			return;
		}
		bool playing = !held.empty() && held.top() == channel;
		held.remove(channel);
		if (held.empty())
			voices[0].envelope.gate(false);
		else if (playing && style == MONO)
			trigger(voices[0], held.top());
	}

	void handleNotes() {
		int newStyle = (int) params[STYLE_PARAM].getValue();
		if (newStyle != style) {
			style = newStyle;
			held.clear();
			for (Voice& v : voices)
				v.envelope.gate(false);
		}
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
	}

	void process(const ProcessArgs& args) override {
		trackTempo(args.sampleTime);
		if (reset.process(inputs[RESET_INPUT].getVoltage(), 0.1f, 1.f))
			lfo.restart();
		handleNotes();
		lfoValue = lfo.process((int) params[LFO_SHAPE_PARAM].getValue(), controls.lfoCycles, inputs[EXT_INPUT].getVoltage() / 5.f);
		if (controlDivider.process())
			updateControls(args.sampleTime);

		if (style != POLY) {
			Voice& v = voices[0];
			if (!held.empty())
				v.pitch += (inputs[VOCT_INPUT].getPolyVoltage(held.top()) - v.pitch) * controls.glide;
		}
		// Every voice runs, so voices left releasing by a change of note style fade out normally.
		float mix = 0.f;
		for (int i = 0; i < PORT_MAX_CHANNELS; i++) {
			Voice& v = voices[i];
			if (!v.envelope.active())
				continue;
			if (style == POLY)
				v.pitch = inputs[VOCT_INPUT].getPolyVoltage(i);
			float frequency = dsp::FREQ_C4 * dsp::exp2_taylor5(v.pitch + controls.pitch);
			float level = v.envelope.process(controls.envelope) * v.velocity;
			mix += v.sound.process(frequency, args.sampleTime, controls.p1, controls.p2) * level;
		}

		float y = softLimit(highpass.process(lowpass.process(mix).lp).hp * controls.gain);
		if (outputs[RIGHT_OUTPUT].isConnected()) {
			outputs[LEFT_OUTPUT].setVoltage(y * controls.left);
			outputs[RIGHT_OUTPUT].setVoltage(y * controls.right);
		}
		else {
			outputs[LEFT_OUTPUT].setVoltage(y);
		}
	}

	void onReset(const ResetEvent& e) override {
		Module::onReset(e);
		ppqn = 4;
	}

	json_t* dataToJson() override {
		json_t* root = json_object();
		json_object_set_new(root, "ppqn", json_integer(ppqn));
		return root;
	}

	void dataFromJson(json_t* root) override {
		if (json_t* j = json_object_get(root, "ppqn"))
			ppqn = std::max((int) json_integer_value(j), 1);
	}
};


/** Every synth track's panel has the same layout, so one widget serves them all. */
template <class Engine>
struct SynthTrackWidget : ModuleWidget {
	using Track = SynthTrack<Engine>;

	SynthTrackWidget(Track* module) {
		setModule(module);
		std::string panel = asset::plugin(pluginInstance, string::f("res/%s.svg", Engine::SLUG));
		setPanel(createPanel(panel, asset::plugin(pluginInstance, string::f("res/%s-dark.svg", Engine::SLUG))));
		addScrews(this);

		PanelLayout layout(panel);
#define AT(ID) layout.center(#ID), module, Track::ID
		// One colour per column, like the hardware's four dials; one row per page.
		addParam(createParamCentered<PastelKnob<Pastel::Green>>(AT(P1_PARAM)));
		addParam(createParamCentered<PastelKnob<Pastel::Cyan>>(AT(P2_PARAM)));
		addParam(createParamCentered<PastelKnob<Pastel::Yellow>>(AT(FILTER_PARAM)));
		addParam(createParamCentered<PastelKnob<Pastel::Salmon>>(AT(RESONANCE_PARAM)));
		addParam(createParamCentered<PastelKnob<Pastel::Green>>(AT(ATTACK_PARAM)));
		addParam(createParamCentered<PastelKnob<Pastel::Cyan>>(AT(DECAY_PARAM)));
		addParam(createParamCentered<PastelKnob<Pastel::Yellow>>(AT(SUSTAIN_PARAM)));
		addParam(createParamCentered<PastelKnob<Pastel::Salmon>>(AT(RELEASE_PARAM)));
		addParam(createParamCentered<PastelKnob<Pastel::Green>>(AT(LFO_AMOUNT_PARAM)));
		addParam(createParamCentered<PastelKnob<Pastel::Cyan>>(AT(LFO_SPEED_PARAM)));
		addParam(createParamCentered<PastelKnob<Pastel::Yellow>>(AT(LFO_TARGET_PARAM)));
		addParam(createParamCentered<PastelKnob<Pastel::Salmon>>(AT(LFO_SHAPE_PARAM)));
		addParam(createParamCentered<PastelKnob<Pastel::Green>>(AT(STYLE_PARAM)));
		addParam(createParamCentered<PastelKnob<Pastel::Cyan>>(AT(GLIDE_PARAM)));
		addParam(createParamCentered<PastelKnob<Pastel::Yellow>>(AT(PAN_PARAM)));
		addParam(createParamCentered<PastelKnob<Pastel::Salmon>>(AT(LEVEL_PARAM)));

		addInput(createInputCentered<PJ301MPort>(AT(VOCT_INPUT)));
		addInput(createInputCentered<PJ301MPort>(AT(GATE_INPUT)));
		addInput(createInputCentered<PJ301MPort>(AT(VELOCITY_INPUT)));
		addInput(createInputCentered<PJ301MPort>(AT(CLOCK_INPUT)));
		addInput(createInputCentered<PJ301MPort>(AT(RESET_INPUT)));
		addInput(createInputCentered<PJ301MPort>(AT(EXT_INPUT)));
		addOutput(createOutputCentered<PJ301MPort>(AT(LEFT_OUTPUT)));
		addOutput(createOutputCentered<PJ301MPort>(AT(RIGHT_OUTPUT)));
#undef AT
	}

	void appendContextMenu(Menu* menu) override {
		Track* module = getModule<Track>();
		std::vector<std::string> labels;
		for (int n : Track::PPQN)
			labels.push_back(string::f("%d PPQN", n));
		menu->addChild(new MenuSeparator);
		menu->addChild(createIndexSubmenuItem("Clock pulses per beat", labels,
			[=]() { return std::find(std::begin(Track::PPQN), std::end(Track::PPQN), module->ppqn) - std::begin(Track::PPQN); },
			[=](size_t i) { module->ppqn = Track::PPQN[i]; }));
	}
};

} // namespace opz
