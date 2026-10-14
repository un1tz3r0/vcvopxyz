#pragma once
#include "../plugin.hpp"
#include "../ui.hpp"
#include "../dsp/Adsr.hpp"
#include "../dsp/Svf.hpp"
#include "Lfo.hpp"
#include "../TripleBuffer.hpp"
#include "Bus.hpp"
#include <atomic>

/*
An OP-Z track's engine, whatever it plays: four dials that step through four pages (sound, envelope, LFO, mix) plus
the track settings. Its notes come from the inputs, or with GATE unpatched from its track's lane of a Sequencer to
its left, over the bus (see Bus.hpp). Everything after the voices is shared: the LFO, the filter, the level and pan,
the outputs, the lights and the snapshots a Screen beside it reads. A synth track (SynthTrack.hpp) and a drum track
(Drum.cpp) differ only in their voices, their two sound dials and their note styles.
*/
namespace opz {

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


/** What every track has in common, whatever its engine, and all a Screen beside it needs to know. */
struct Track : Module, BusNode {
	enum ParamId {
		P1_PARAM, P2_PARAM, FILTER_PARAM, RESONANCE_PARAM,
		ATTACK_PARAM, DECAY_PARAM, SUSTAIN_PARAM, RELEASE_PARAM,
		LFO_AMOUNT_PARAM, LFO_SPEED_PARAM, LFO_TARGET_PARAM, LFO_SHAPE_PARAM,
		FX1_PARAM, FX2_PARAM, PAN_PARAM, LEVEL_PARAM,
		STYLE_PARAM, GLIDE_PARAM,
		PARAMS_LEN
	};
	static_assert(PARAMS_LEN == SOUND_PARAMS, "a step locks any of a track's params");
	// The return and the CV outputs belong to Breakout, which hands the voice to other modules; the other engines'
	// panels leave them out.
	enum InputId { VOCT_INPUT, GATE_INPUT, VELOCITY_INPUT, CLOCK_INPUT, RESET_INPUT, EXT_INPUT, RETURN_L_INPUT, RETURN_R_INPUT, INPUTS_LEN };
	enum OutputId {
		LEFT_OUTPUT, RIGHT_OUTPUT, FX1_OUTPUT, FX2_OUTPUT,
		CV_VOCT_OUTPUT, CV_GATE_OUTPUT, CV_ENV_OUTPUT, CV_LFO_OUTPUT, CV_P1_OUTPUT, CV_P2_OUTPUT,
		CV_FILTER_OUTPUT, CV_RESONANCE_OUTPUT, CV_FX1_OUTPUT, CV_FX2_OUTPUT,
		OUTPUTS_LEN
	};
	static constexpr int AUDIO_OUTPUTS = CV_VOCT_OUTPUT;
	enum LightId {
		SELECTED_LIGHT, ACTIVE_LIGHT,
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

	/** The audio thread's moving parts, published for a Screen about 60 times a second. */
	struct Snapshot {
		static constexpr int SCOPE = 1024;
		float scope[SCOPE];                  // the output's last samples, oldest first
		float levels[PORT_MAX_CHANNELS];     // each voice's envelope
		int stages[PORT_MAX_CHANNELS];
		float lfoPhase, lfoValue;
		float outputs[AUDIO_OUTPUTS];        // peak levels since the last snapshot
	};
	TripleBuffer<Snapshot> snapshots;

	/** The dials after the LFO, worked out at control rate. */
	struct Controls {
		float p1 = 0.f, p2 = 0.f, filter = 0.5f, resonance = 0.f;
		float lfo = 0.f;      // the LFO's offset, after its amount
		float pitch = 0.f;    // LFO offset, V/oct
		float glide = 1.f;    // portamento smoothing per sample; 1 jumps straight to the note
		float lfoCycles = 0.f;
		float left = 0.f, right = 0.f, gain = 0.f, fx1 = 0.f, fx2 = 0.f;
		xyz::Adsr::Shape envelope;
	};

	static constexpr const char* PAGE_NAMES[PAGES_LEN] = {"SOUND", "ENVELOPE", "LFO", "MIX", "TRACK"};

	bool drum = false;           // takes one of tracks 1 to 4 rather than 5 to 8
	float secondsPerBeat = 0.5f; // 120 BPM until a clock says otherwise
	int ppqn = 4;                // clock pulses per beat: 4 is one per sixteenth, one per step
	int page = PAGE_SOUND;
	bool trackMode = false;
	int busTrack = -1;           // the Sequencer's track this engine plays, or -1 off the bus
	bool selected = false;       // the Sequencer's keys and dials are on this engine's track

	int style = 0;               // the note style playing, which a change of the dial lets every note go for
	int channels = 0;            // the note source's channels
	dsp::SchmittTrigger gates[PORT_MAX_CHANNELS];
	uint32_t notesPlayed = 0;
	bool lfoInside = true;       // false while the LFO goes out to other modules instead
	float sweep[SWEEP_TARGETS] = {}; // the sequencer's sweep component, from the lane
	// The lane this engine plays (null while GATE is patched), and the master's requests last seen.
	const Downstream::Lane* lane = nullptr;
	bool masterTrackMode = false;
	uint32_t pageRequests = 0;
	float dialValues[4] = {-1.f, -1.f, -1.f, -1.f};
	Lfo lfo;
	float lfoValue = 0.f;
	xyz::Svf lowpass, highpass;
	Controls controls;
	dsp::ClockDivider controlDivider, lightDivider;
	dsp::SchmittTrigger clock, reset;
	float sinceClock = 0.f;
	Meter inputMeters[INPUTS_LEN], outputMeters[OUTPUTS_LEN];
	float outputPeaks[AUDIO_OUTPUTS] = {};
	bool limiting = false;
	float scope[Snapshot::SCOPE] = {};
	int scopeEnd = 0;
	float sinceSnapshot = 0.f;

	Track() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		attach(this);
		controlDivider.setDivision(16);
		lightDivider.setDivision(64);
	}

	/** Every param but the two sound dials, which the engine configures first, and the jacks. The LFO's targets
	default to the sound dials' names. */
	void configTrack(const std::vector<std::string>& styles, const std::string& styleHelp, std::vector<std::string> targets = {}) {
		configParam<FilterQuantity>(FILTER_PARAM, 0.f, 1.f, 0.5f, "Filter")->description = "Low-pass to the left, high-pass to the right";
		configParam(RESONANCE_PARAM, 0.f, 1.f, 0.f, "Resonance", "%", 0.f, 100.f);
		configParam<TimeQuantity>(ATTACK_PARAM, 0.f, 1.f, 0.f, "Attack");
		configParam<TimeQuantity>(DECAY_PARAM, 0.f, 1.f, 0.5f, "Decay");
		configParam(SUSTAIN_PARAM, 0.f, 1.f, 0.7f, "Sustain", "%", 0.f, 100.f);
		configParam<TimeQuantity>(RELEASE_PARAM, 0.f, 1.f, 0.5f, "Release");
		configParam(LFO_AMOUNT_PARAM, -1.f, 1.f, 0.f, "LFO amount", "%", 0.f, 100.f)->description = "Off in the middle; to the left the LFO pulls the target the other way";
		configParam<SpeedQuantity>(LFO_SPEED_PARAM, 0.f, 1.f, 0.75f, "LFO speed")->description = "Synced to the clock to the left, free to the right";
		if (targets.empty())
			targets = {getParamQuantity(P1_PARAM)->name, getParamQuantity(P2_PARAM)->name, "Filter", "Resonance", "Attack", "Pitch", "Pan", "Volume"};
		configSwitch(LFO_TARGET_PARAM, 0.f, TARGETS_LEN - 1, TARGET_FILTER, "LFO target", targets);
		configSwitch(LFO_SHAPE_PARAM, 0.f, Lfo::SHAPES_LEN - 1, Lfo::SINE, "LFO shape",
			{"Sine", "Triangle", "Square", "Saw", "Random", "External (EXT input)",
			 "Bell, per note", "Triangle, per note", "Square, per note", "Saw, per note", "Random, per note", "Single saw, per note"});
		configParam(FX1_PARAM, 0.f, 1.f, 0.f, "FX 1 send", "%", 0.f, 100.f)->description = "To the FX 1 output, after the level";
		configParam(FX2_PARAM, 0.f, 1.f, 0.f, "FX 2 send", "%", 0.f, 100.f)->description = "To the FX 2 output, after the level";
		configParam(PAN_PARAM, -1.f, 1.f, 0.f, "Pan", "%", 0.f, 100.f);
		configParam(LEVEL_PARAM, 0.f, 1.f, 0.7f, "Level", "%", 0.f, 100.f);
		configSwitch(STYLE_PARAM, 0.f, styles.size() - 1, 0.f, "Note style", styles)->description = styleHelp;
		configParam(GLIDE_PARAM, 0.f, 1.f, 0.f, "Portamento", "%", 0.f, 100.f)->description = "Mono and legato only";

		configLight(SELECTED_LIGHT, "Selected on the Sequencer");
		configLight(ACTIVE_LIGHT, "Notes playing");

		configInput(VOCT_INPUT, "1V/octave pitch");
		configInput(GATE_INPUT, "Gate");
		configInput(VELOCITY_INPUT, "Velocity (0-10V)");
		configInput(CLOCK_INPUT, "Clock, for the synced LFO speeds");
		configInput(RESET_INPUT, "LFO reset");
		configInput(EXT_INPUT, "LFO external (±5V), for the External shape");
		configOutput(LEFT_OUTPUT, "Left, or mono while right is unpatched");
		configOutput(RIGHT_OUTPUT, "Right");
		configOutput(FX1_OUTPUT, "FX 1 send");
		configOutput(FX2_OUTPUT, "FX 2 send");
		configInput(RETURN_L_INPUT, "Return left, or mono");
		configInput(RETURN_R_INPUT, "Return right");
		const char* const cvs[] = {"1V/octave pitch", "Gate", "Envelope (0-10V)", "LFO (±5V)", "", "",
			"Filter (0-10V)", "Resonance (0-10V)", "FX 1 send (0-10V)", "FX 2 send (0-10V)"};
		for (int i = CV_VOCT_OUTPUT; i < OUTPUTS_LEN; i++)
			configOutput(i, cvs[i - CV_VOCT_OUTPUT]);
		configOutput(CV_P1_OUTPUT, getParamQuantity(P1_PARAM)->name + " (0-10V)");
		configOutput(CV_P2_OUTPUT, getParamQuantity(P2_PARAM)->name + " (0-10V)");
	}

	/** Gives a param a new default, and sets it there. */
	void configDefault(int param, float value) {
		paramQuantities[param]->defaultValue = value;
		params[param].setValue(value);
	}

	/** What dial `d` turns on a page, or -1 for nothing. */
	virtual int dial(int page, int d) const {
		return DIALS[page][d];
	}

	/** What a note plays, for the Sequencer's keys, where that isn't just its pitch; for the UI thread. */
	virtual std::string soundName(int pitch) {
		return "";
	}

	virtual void noteOn(int channel) = 0;
	virtual void noteOff(int channel) = 0;
	virtual void releaseAll() = 0;
	/** A voice's envelope, and its velocity, for the lights and a Screen; null past the last voice. */
	virtual const xyz::Adsr* envelope(int voice, float* velocity) const = 0;

	/** A dial's name as the panel's legend prints it; the engine's two are named after its params. */
	std::string dialName(int param) {
		static const char* const names[] = {nullptr, nullptr, "FILTER", "RESO", "ATTACK", "DECAY", "SUSTAIN", "RELEASE",
			"AMOUNT", "SPEED", "TARGET", "SHAPE", "FX 1", "FX 2", "PAN", "LEVEL", "STYLE", "GLIDE"};
		return names[param] ? names[param] : string::uppercase(paramQuantities[param]->name);
	}

	/** The page the dials are showing: one of the four, or the track settings. */
	int shownPage() const {
		return trackMode ? PAGE_TRACK : page;
	}

	void showPage(int p) {
		page = p;
		trackMode = false;
	}

	void setTrackMode(bool on) {
		trackMode = on;
	}

	float velocity(int channel) {
		if (lane)
			return lane->velocity[channel];
		return inputs[VELOCITY_INPUT].isConnected() ? clamp(inputs[VELOCITY_INPUT].getPolyVoltage(channel) / 10.f, 0.f, 1.f) : 1.f;
	}

	float notePitch(int note) {
		return lane ? lane->pitch[note] : inputs[VOCT_INPUT].getPolyVoltage(note);
	}

	/** Restarts the per-note LFO shapes, for a note that starts its sound. */
	void noteStarted() {
		notesPlayed++;
		if (Lfo::triggered((int) value(LFO_SHAPE_PARAM)))
			lfo.restart();
	}

	void handleNotes(const Downstream::Lane* from) {
		int newStyle = (int) value(STYLE_PARAM);
		if (newStyle != style || bool(from) != bool(lane)) {
			style = newStyle;
			releaseAll();
		}
		lane = from;
		// Channels beyond the gate's count read as low, so dropping channels releases their notes.
		channels = lane ? lane->channels : inputs[GATE_INPUT].getChannels();
		for (int c = 0; c < PORT_MAX_CHANNELS; c++) {
			float gate = c >= channels ? 0.f : lane ? 10.f * lane->gate[c] : inputs[GATE_INPUT].getVoltage(c);
			switch (gates[c].processEvent(gate, 0.1f, 1.f)) {
				case dsp::SchmittTrigger::TRIGGERED: noteOn(c); break;
				case dsp::SchmittTrigger::UNTRIGGERED: noteOff(c); break;
				default: break;
			}
		}
	}

	/** The selected track's engine follows the master's track mode and page keys. */
	void handleMaster(const Downstream* bus) {
		bool was = selected;
		selected = bus && busTrack == bus->selected;
		if (was && !selected && trackMode)
			setTrackMode(false);
		if (!bus)
			return;
		if (selected && (!was || bus->trackMode != masterTrackMode))
			setTrackMode(bus->trackMode);
		if (selected && bus->pageRequests != pageRequests)
			showPage(bus->page == NEXT_PAGE ? (page + 1) % PAGE_TRACK : bus->page);
		masterTrackMode = bus->trackMode;
		pageRequests = bus->pageRequests;
	}

	/** Takes this engine's track from the bus, the next drum or synth track along, and passes the bus on with this
	engine counted. */
	const Downstream* joinBus() {
		const Downstream* bus = fromLeft(this);
		busTrack = -1;
		if (bus) {
			int taken = drum ? bus->drums : bus->synths;
			if (taken < (drum ? DRUM_TRACKS : SYNTH_TRACKS))
				busTrack = (drum ? 0 : DRUM_TRACKS) + taken;
			if (Downstream* out = toRight(this)) {
				*out = *bus;
				(drum ? out->drums : out->synths)++;
			}
		}
		return bus;
	}

	/** Sends the mix of the engines to the right on to the left, with this one's sound added while its outputs are
	unpatched, and its page and dials. */
	void sendUpstream(float left, float right) {
		Upstream* out = toLeft(this);
		if (!out)
			return;
		const Upstream* u = fromRight(this);
		*out = u ? *u : Upstream();
		if (busTrack < 0)
			return;
		out->page[busTrack] = trackMode ? -1 : page;
		if (!outputs[LEFT_OUTPUT].isConnected() && !outputs[RIGHT_OUTPUT].isConnected()) {
			out->left += left;
			out->right += right;
		}
		if (selected)
			std::copy(std::begin(dialValues), std::end(dialValues), out->dials);
	}

	void trackTempo(float sampleTime, const Downstream* bus) {
		if (bus && !inputs[CLOCK_INPUT].isConnected()) {
			secondsPerBeat = bus->secondsPerBeat;
			return;
		}
		sinceClock += sampleTime;
		if (clock.process(inputs[CLOCK_INPUT].getVoltage(), 0.1f, 1.f)) {
			// The first pulse after a long gap only starts the count again.
			if (sinceClock * ppqn < 4.f)
				secondsPerBeat = sinceClock * ppqn;
			sinceClock = 0.f;
		}
	}

	/** A dial's value: where the step playing locks it, or where it's turned. */
	float value(int param) {
		if (!lane || !(lane->locks >> param & 1))
			return params[param].getValue();
		const ParamQuantity* q = paramQuantities[param];
		float v = math::rescale(lane->lockValues[param], 0.f, 1.f, q->minValue, q->maxValue);
		return q->snapEnabled ? std::round(v) : v;
	}

	/** A dial's value, moved by the LFO when the LFO targets it, and by a sweep. Both are shares of its travel. */
	float modulated(int param, int target, float lfoOffset, float sweep = 0.f) {
		float v = value(param);
		float offset = sweep + ((int) value(LFO_TARGET_PARAM) == target ? lfoOffset : 0.f);
		if (offset == 0.f)
			return v;
		ParamQuantity* q = paramQuantities[param];
		v = clamp(v + offset * (q->maxValue - q->minValue), q->minValue, q->maxValue);
		return q->snapEnabled ? std::round(v) : v;
	}

	void updateControls(float sampleTime) {
		float amount = value(LFO_AMOUNT_PARAM);
		// Squaring the amount leaves room for subtle settings, like vibrato, near the middle.
		Controls& c = controls;
		c.lfo = amount * std::fabs(amount) * lfoValue;
		float offset = lfoInside ? c.lfo : 0.f;
		c.p1 = modulated(P1_PARAM, TARGET_P1, offset, sweep[SWEEP_SYNTH]);
		c.p2 = modulated(P2_PARAM, TARGET_P2, offset);
		c.pitch = (int) value(LFO_TARGET_PARAM) == TARGET_PITCH ? 2.f * offset : 0.f;

		// The resonance belongs to whichever filter the dial is moving; the other one stays flat.
		c.filter = modulated(FILTER_PARAM, TARGET_FILTER, offset, sweep[SWEEP_FILTER]);
		c.resonance = modulated(RESONANCE_PARAM, TARGET_RESONANCE, offset);
		float damping = 2.f - 1.9f * c.resonance; // Q from 0.5 to 10
		lowpass.tune(lowpassHz(c.filter) * sampleTime, c.filter < 0.5f ? damping : float(M_SQRT2));
		highpass.tune(highpassHz(c.filter) * sampleTime, c.filter > 0.5f ? damping : float(M_SQRT2));

		c.envelope.set(envelopeTime(modulated(ATTACK_PARAM, TARGET_ATTACK, offset)), envelopeTime(value(DECAY_PARAM)),
			value(SUSTAIN_PARAM), envelopeTime(value(RELEASE_PARAM)), sampleTime);
		float glide = value(GLIDE_PARAM);
		c.glide = glide > 0.f ? xyz::Adsr::Shape::coefficient(glide * glide, sampleTime) : 1.f;
		c.lfoCycles = sampleTime / Lfo::period(value(LFO_SPEED_PARAM), secondsPerBeat);

		float pan = modulated(PAN_PARAM, TARGET_PAN, offset, sweep[SWEEP_PAN]);
		float level = modulated(LEVEL_PARAM, TARGET_VOLUME, offset);
		c.left = std::cos((pan + 1.f) * float(M_PI) / 4.f);
		c.right = std::sin((pan + 1.f) * float(M_PI) / 4.f);
		c.gain = 5.f * level * level * (lane ? lane->level : 1.f);
		c.fx1 = value(FX1_PARAM) * value(FX1_PARAM);
		c.fx2 = value(FX2_PARAM) * value(FX2_PARAM);
	}

	/** Everything up to the voices, once a sample: the bus, the tempo, the keys, the notes, the LFO and the dials. */
	void prepare(const ProcessArgs& args) {
		const Downstream* bus = joinBus();
		trackTempo(args.sampleTime, bus);
		if (reset.process(inputs[RESET_INPUT].getVoltage(), 0.1f, 1.f))
			lfo.restart();
		bool control = controlDivider.process();
		if (control)
			handleMaster(bus);
		bool fromBus = busTrack >= 0 && !inputs[GATE_INPUT].isConnected() && bus->lanes[busTrack].channels > 0;
		handleNotes(fromBus ? &bus->lanes[busTrack] : nullptr);
		for (int i = 0; i < SWEEP_TARGETS; i++)
			sweep[i] = lane ? lane->sweep[i] : 0.f;
		lfoValue = lfo.process((int) value(LFO_SHAPE_PARAM), controls.lfoCycles, inputs[EXT_INPUT].getVoltage() / 5.f);
		if (control)
			updateControls(args.sampleTime);
	}

	/** The voices' mix through the filter, at the track's level. */
	float filter(float mix) {
		return highpass.process(lowpass.process(mix).lp).hp * controls.gain;
	}

	/** The track's sound after the level, before the soft limit and the pan, to the outputs, the bus and the meters. */
	void emit(float left, float right, float fx1, float fx2, float sampleTime) {
		limiting |= std::fmax(std::fabs(left), std::fabs(right)) > 5.f;
		left = softLimit(left);
		right = softLimit(right);
		float mono = 0.5f * (left + right);
		float panned[2] = {left * controls.left, right * controls.right};
		float out[AUDIO_OUTPUTS] = {mono, 0.f, mono * fx1, mono * fx2};
		if (outputs[RIGHT_OUTPUT].isConnected()) {
			out[LEFT_OUTPUT] = panned[0];
			out[RIGHT_OUTPUT] = panned[1];
		}
		for (int i = 0; i < AUDIO_OUTPUTS; i++) {
			outputs[i].setVoltage(out[i]);
			outputPeaks[i] = std::fmax(outputPeaks[i], std::fabs(out[i]));
		}
		scope[scopeEnd] = mono;
		scopeEnd = (scopeEnd + 1) % Snapshot::SCOPE;
		sendUpstream(panned[0], panned[1]);

		if (lightDivider.process())
			updateLights(sampleTime * lightDivider.getDivision());
	}

	static float peak(Port& port) {
		float level = 0.f;
		for (int c = 0; c < port.getChannels(); c++)
			level = std::fmax(level, std::fabs(port.getVoltage(c)));
		return level;
	}

	void updateLights(float dt) {
		// The dials' values, for the Sequencer's dial LEDs.
		for (int d = 0; d < 4; d++) {
			int param = dial(shownPage(), d);
			dialValues[d] = param < 0 ? -1.f : paramQuantities[param]->getScaledValue();
		}
		float loudest = 0.f, velocity;
		for (int i = 0; const xyz::Adsr* e = envelope(i, &velocity); i++)
			loudest = std::fmax(loudest, e->level * velocity);
		lights[SELECTED_LIGHT].setBrightness(selected);
		lights[ACTIVE_LIGHT].setBrightnessSmooth(loudest, dt);

		for (int i = 0; i < INPUTS_LEN; i++) {
			float level = peak(inputs[i]);
			inputMeters[i].process(level / 10.f, level > 10.f, dt);
		}
		for (int i = 0; i < AUDIO_OUTPUTS; i++) {
			outputMeters[i].process(outputPeaks[i] / 5.f, limiting && outputPeaks[i] > 0.f, dt);
			outputPeaks[i] = 0.f;
		}
		for (int i = AUDIO_OUTPUTS; i < OUTPUTS_LEN; i++)
			outputMeters[i].process(peak(outputs[i]) / 10.f, false, dt);
		limiting = false;
		for (int i = 0; i < INPUTS_LEN; i++) {
			lights[INPUT_LIGHT + 2 * i].setBrightness(inputMeters[i].peak);
			lights[INPUT_LIGHT + 2 * i + 1].setBrightness(inputMeters[i].clip > 0.f);
		}
		for (int i = 0; i < OUTPUTS_LEN; i++) {
			lights[OUTPUT_LIGHT + 2 * i].setBrightness(outputMeters[i].peak);
			lights[OUTPUT_LIGHT + 2 * i + 1].setBrightness(outputMeters[i].clip > 0.f);
		}

		sinceSnapshot += dt;
		if (sinceSnapshot >= 1.f / 60.f) {
			sinceSnapshot = 0.f;
			publishSnapshot();
		}
	}

	void publishSnapshot() {
		Snapshot& s = snapshots.write();
		for (int i = 0; i < Snapshot::SCOPE; i++)
			s.scope[i] = scope[(scopeEnd + i) % Snapshot::SCOPE];
		float velocity;
		for (int i = 0; i < PORT_MAX_CHANNELS; i++) {
			const xyz::Adsr* e = envelope(i, &velocity);
			s.levels[i] = e ? e->level : 0.f;
			s.stages[i] = e ? e->stage : xyz::Adsr::IDLE;
		}
		s.lfoPhase = lfo.phase;
		s.lfoValue = lfoValue;
		for (int i = 0; i < AUDIO_OUTPUTS; i++)
			s.outputs[i] = outputMeters[i].peak;
		snapshots.publish();
	}

	void onReset(const ResetEvent& e) override {
		Module::onReset(e);
		ppqn = 4;
		page = PAGE_SOUND;
		trackMode = false;
	}

	/** Track mode isn't saved, so a patch always opens on the dials' pages. */
	json_t* dataToJson() override {
		json_t* root = json_object();
		json_object_set_new(root, "ppqn", json_integer(ppqn));
		json_object_set_new(root, "page", json_integer(page));
		return root;
	}

	void dataFromJson(json_t* root) override {
		if (json_t* j = json_object_get(root, "ppqn"))
			ppqn = std::max((int) json_integer_value(j), 1);
		if (json_t* j = json_object_get(root, "page"))
			page = clamp((int) json_integer_value(j), 0, PAGE_TRACK - 1);
	}
};

} // namespace opz
