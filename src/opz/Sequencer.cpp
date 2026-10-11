/*
Sequencer: an OP-Z synth track's sequencer. Sixteen steps of up to four notes each, filled from its own keyboard or
from poly V/OCT, GATE and VEL inputs, and played out as poly V/OCT, GATE and VEL for an engine beside it. It keeps the
tempo until a master module takes that over: its own, or a patched clock's.

Its keys are laid out as on the OP-Z: TRACK, REC and −, PLAY and +, STOP and SHIFT, then the 16 steps along the top,
the numbered black keys and the white keys from F to E. Rack can't hold one button down while clicking another, so
a long press or a Ctrl+click picks a step up, and it stays held (blinking) until it's clicked again. TRACK, REC and
SHIFT latch.
*/
#include "SynthTrack.hpp"
#include "Sequence.hpp"

namespace opz {

/** The black keys, in semitones above F, double as the value keys: 1 to 9 then 0 from the left. */
constexpr int BLACK_KEYS[5] = {1, 3, 5, 8, 10};
inline int keyValue(int key) {
	const int* k = std::find(std::begin(BLACK_KEYS), std::end(BLACK_KEYS), key % 12);
	return k == std::end(BLACK_KEYS) ? -1 : 5 * (key / 12) + int(k - BLACK_KEYS);
}
inline int valueKey(int value) {
	return 12 * (value / 5) + BLACK_KEYS[value % 5];
}

inline std::string noteName(int pitch) {
	static const char* const names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
	return string::f("%s%d", names[pitch % 12], pitch / 12 - 1);
}

/** The keys' LED colours, besides the notes' orange. */
namespace led {
const NVGcolor WHITE = nvgRGBf(1.f, 1.f, 1.f), RED = nvgRGBf(1.f, 0.1f, 0.06f), GREEN = nvgRGBf(0.15f, 1.f, 0.3f), BLUE = nvgRGBf(0.2f, 0.4f, 1.f);
}

/** The note length dial's settings, in ticks of a ×1 step: 1/64 to a bar, dotted between, then drone. */
constexpr int NOTE_LENGTHS[] = {6, 9, 12, 18, 24, 36, 48, 72, 96, 144, 192, 288, 384, DRONE};
constexpr int LONGEST = STEPS * TICKS; // the longest note a step's menu sets, short of a drone


struct Sequencer : Module {
	enum ParamId {
		TEMPO_PARAM, SWING_PARAM, LENGTH_PARAM, QUANTIZE_PARAM, STEP_COUNT_PARAM, STEP_LENGTH_PARAM,
		// The keys, each with an RGB light: three light ids from TRACK_LIGHT, in the same order.
		TRACK_PARAM, REC_PARAM, PLAY_PARAM, STOP_PARAM, OCTAVE_DOWN_PARAM, OCTAVE_UP_PARAM, SHIFT_PARAM,
		ENUMS(STEP_PARAM, STEPS), ENUMS(KEY_PARAM, KEYS),
		PARAMS_LEN
	};
	enum InputId { CLOCK_INPUT, RESET_INPUT, ADVANCE_INPUT, VOCT_INPUT, GATE_INPUT, VELOCITY_INPUT, INPUTS_LEN };
	enum OutputId { CLOCK_OUTPUT, RESET_OUTPUT, VOCT_OUTPUT, GATE_OUTPUT, VELOCITY_OUTPUT, OUTPUTS_LEN };
	enum LightId {
		ENUMS(TRACK_LIGHT, 3), ENUMS(REC_LIGHT, 3), ENUMS(PLAY_LIGHT, 3), ENUMS(STOP_LIGHT, 3), ENUMS(OCTAVE_DOWN_LIGHT, 3),
		ENUMS(OCTAVE_UP_LIGHT, 3), ENUMS(SHIFT_LIGHT, 3), ENUMS(STEP_LIGHT, 3 * STEPS), ENUMS(KEY_LIGHT, 3 * KEYS),
		TEMPO_LIGHT, SWING_LIGHT, LENGTH_LIGHT, QUANTIZE_LIGHT, ENUMS(CHANNEL_LIGHT, CHANNELS), INTERNAL_LIGHT, EXTERNAL_LIGHT,
		ENUMS(INPUT_LIGHT, 2 * INPUTS_LEN), ENUMS(OUTPUT_LIGHT, 2 * OUTPUTS_LEN),
		LIGHTS_LEN
	};
	static constexpr float LONG_PRESS = 0.4f, CLEAR_HOLD = 1.f; // seconds
	static constexpr int SOURCES = KEYS + CHANNELS;             // live notes: the keys, then the GATE input's channels
	static constexpr int ALL_KEYS = PARAMS_LEN - TRACK_PARAM;

	static int keyLight(int param) {
		return TRACK_LIGHT + 3 * (param - TRACK_PARAM);
	}

	/** A note played live, from a key or a GATE channel. */
	struct Live {
		bool on = false;
		uint32_t id = 0;        // the player's id for it
		int pitch = 60;
		int recorded = -1;      // the step it was recorded into while playing, which gets its length on release
		double start = 0.0, slot = 1.0;
	};

	struct StepQuantity : SwitchQuantity {
		std::string getLabel() override {
			Sequencer* m = static_cast<Sequencer*>(module);
			int i = paramId - STEP_PARAM;
			if (m->trackMode)
				return string::f("Step count %d", i + 1);
			std::string label = string::f("Step %d", i + 1);
			const Step& s = m->pattern.steps[i];
			for (int n = 0; n < s.count; n++)
				label += (n ? " " : ": ") + noteName(s.notes[n].pitch);
			return label;
		}
	};

	struct KeyQuantity : SwitchQuantity {
		std::string getLabel() override {
			static const char* const values[] = {"×1", "×2", "×3", "×4", "×5", "×6", "×7", "×8", "×16", "one per ADV trigger"};
			Sequencer* m = static_cast<Sequencer*>(module);
			int key = paramId - KEY_PARAM;
			if (m->trackMode && keyValue(key) >= 0)
				return string::f("Step length %s", values[keyValue(key)]);
			return noteName(m->keyPitch(key));
		}
	};

	Pattern pattern;
	Pattern recorded;            // the pattern as the undo history last saw it; the UI thread's
	Settings settings;
	Player player;
	Step memory;                 // what a click on an empty step pastes: the last notes played, or the step last held
	Live live[SOURCES];
	bool playing = false;        // the transport, as PLAY shows it
	bool armed = false;          // playing, but waiting for the clock's first pulse to start
	bool recording = false, trackMode = false, shifted = false;
	bool erasing = false;        // removing the held notes from the steps they pass, while recording
	bool chord = false;          // notes are down for the step being step-recorded
	int held = -1, cursor = 0, octave = 0, ppqn = 4;
	bool longPressHolds = true, ctrlClickHolds = true;
	std::atomic<float> shownTempo{120.f}; // -1 while an external clock is still being measured

	KeyGesture gestures[ALL_KEYS]; // the keys', from TRACK_PARAM on
	float clearing = -1.f;       // how long STOP has been held in track mode, or -1
	dsp::SchmittTrigger gates[CHANNELS], clockIn, resetIn, advanceIn;
	dsp::PulseGenerator clockOut, resetOut;
	bool external = false;
	long pulses = 0, lastTick = -1;
	float sincePulse = 0.f, pulsePeriod = 0.f; // seconds; a period of 0 is unknown
	dsp::ClockDivider controlDivider, lightDivider;
	Meter inputMeters[INPUTS_LEN], outputMeters[OUTPUTS_LEN];
	float blink = 0.f;

	Sequencer() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		ParamQuantity* q = configParam(TEMPO_PARAM, 40.f, 200.f, 120.f, "Tempo", " BPM");
		q->snapEnabled = true;
		q->randomizeEnabled = false;
		q->description = "A patched CLOCK overrides it";
		configParam(SWING_PARAM, 0.5f, 0.75f, 0.5f, "Swing", "%", 0.f, 100.f)->description = "Delays every second sixteenth; 50% is straight";
		configSwitch(LENGTH_PARAM, 0.f, std::size(NOTE_LENGTHS) - 1, 4.f, "Note length",
			{"1/64", "1/64.", "1/32", "1/32.", "1/16", "1/16.", "1/8", "1/8.", "1/4", "1/4.", "1/2", "1/2.", "1 bar", "Drone"})
			->description = "For notes left at the default length";
		configParam(QUANTIZE_PARAM, 0.f, 1.f, 0.f, "Quantize", "%", 0.f, 100.f)->description = "Pulls micro timing towards the grid as it plays";
		configParam(STEP_COUNT_PARAM, 1.f, STEPS, STEPS, "Step count")->snapEnabled = true;
		configSwitch(STEP_LENGTH_PARAM, 0.f, 9.f, 0.f, "Step length", {"×1", "×2", "×3", "×4", "×5", "×6", "×7", "×8", "×16", "Per ADV trigger"});
		getParamQuantity(STEP_COUNT_PARAM)->randomizeEnabled = false;
		getParamQuantity(STEP_LENGTH_PARAM)->randomizeEnabled = false;
		configKey(this, TRACK_PARAM, "Track mode")->description = "On and off: the steps set the step count, the black keys the step length, − and + shift the pattern, and holding STOP clears it";
		configKey(this, REC_PARAM, "Record")->description = "On and off: records the keys and inputs live while playing, or a step at a time while stopped";
		configKey(this, PLAY_PARAM, "Play")->description = "Starts from the first step, or the clock's next pulse";
		configKey(this, STOP_PARAM, "Stop")->description = "Stopped already: lets go of every note";
		configKey(this, OCTAVE_DOWN_PARAM, "Octave down")->description = "Nudges a held step a tick earlier; with notes held while recording, erases them";
		configKey(this, OCTAVE_UP_PARAM, "Octave up")->description = "Nudges a held step a tick later";
		configKey(this, SHIFT_PARAM, "Shift")->description = "On and off. Shifted keys will place step components";
		for (int i = 0; i < STEPS; i++)
			configKey<StepQuantity>(this, STEP_PARAM + i)->description = "Click: paste the last notes played, or clear. Long-press or " RACK_MOD_CTRL_NAME "+click: hold";
		for (int i = 0; i < KEYS; i++)
			configKey<KeyQuantity>(this, KEY_PARAM + i);
		for (int i = 0; i < CHANNELS; i++)
			configLight(CHANNEL_LIGHT + i, string::f("Channel %d", i + 1));

		configInput(CLOCK_INPUT, "Clock; overrides the tempo");
		configInput(RESET_INPUT, "Reset");
		configInput(ADVANCE_INPUT, "Advance, one step per trigger at step length 0");
		configInput(VOCT_INPUT, "1V/octave pitch, to play and record");
		configInput(GATE_INPUT, "Gate, to play and record");
		configInput(VELOCITY_INPUT, "Velocity (0-10V)");
		configOutput(CLOCK_OUTPUT, "Clock, while playing");
		configOutput(RESET_OUTPUT, "Reset, as it starts");
		configOutput(VOCT_OUTPUT, "1V/octave pitch");
		configOutput(GATE_OUTPUT, "Gate");
		configOutput(VELOCITY_OUTPUT, "Velocity (0-10V)");
		memory.add(Note());
		controlDivider.setDivision(16);
		lightDivider.setDivision(64);
	}

	/** The keys run from F3 to E5, an octave either side of middle C, before − and + move them. */
	int keyPitch(int key) const {
		return clamp(53 + 12 * octave + key, 0, 127);
	}

	bool anyHeld() const {
		return std::any_of(std::begin(live), std::end(live), [](const Live& n) { return n.on; });
	}

	int stepCount() const {
		return clamp(settings.stepCount, 1, STEPS);
	}

	void restart() {
		player.start();
		lastTick = -1;
		pulses = 0;
		resetOut.trigger(1e-3f);
	}

	/** Starts now, or with an external clock, on its next pulse. */
	void play() {
		playing = true;
		armed = external;
		if (armed)
			player.stop();
		else
			restart();
	}

	void stop() {
		if (trackMode)
			player.silence();
		else if (playing) {
			playing = armed = erasing = false;
			player.stop();
		}
		else
			player.panic();
	}

	void toggleHold(int i) {
		held = held == i ? -1 : i;
		const Step& s = pattern.steps[i];
		if (held >= 0 && !s.empty()) {
			memory = s;
			if (!playing)
				player.preview(s, settings);
		}
	}

	/** A click on a step that doesn't pick it up. */
	void clickStep(int i) {
		Step& s = pattern.steps[i];
		if (trackMode)
			params[STEP_COUNT_PARAM].setValue(i + 1);
		else if (held == i)
			held = -1;
		else if (held >= 0) {
			// The held step's notes last until the end of this one, or go back to the default if they already do.
			Step& h = pattern.steps[held];
			int loop = held < stepCount() ? stepCount() : STEPS, span = i - held;
			if (span < 0)
				span += loop;
			int ticks = (span + 1) * TICKS;
			bool same = std::all_of(h.notes, h.notes + h.count, [=](const Note& n) { return n.length == ticks; });
			for (int n = 0; n < h.count; n++)
				h.notes[n].length = same ? 0 : ticks;
		}
		else if (recording && !playing)
			cursor = i;
		else if (s.empty())
			s = memory;
		else
			s.clear();
	}

	void shift(int direction) {
		if (trackMode)
			pattern.rotate(direction, stepCount());
		else if (held >= 0)
			pattern.steps[held].nudge(direction);
		else if (direction < 0 && recording && playing && anyHeld())
			erasing = true;
		else
			octave = clamp(octave + direction, -3, 3);
	}

	/** A live note starts: it plays, and goes into the held step, the record, or the notes a click pastes. */
	void noteDown(int source, int pitch, float volts, int velocity) {
		bool first = !anyHeld();
		Live& n = live[source];
		n = Live();
		n.on = true;
		n.pitch = pitch = clamp(pitch, 0, 127);
		n.id = player.noteOn(volts, velocity / 127.f, INFINITY, true, settings);
		Note note;
		note.pitch = pitch;
		note.velocity = clamp(velocity, 1, 127);
		if (held >= 0) {
			Step& s = pattern.steps[held];
			if (s.indexOf(pitch) >= 0)
				s.remove(pitch);
			else
				s.add(note);
			if (!s.empty())
				memory = s;
			return;
		}
		if (first)
			memory.clear();
		memory.add(note);
		if (!recording || erasing)
			return;
		if (playing) {
			int step, ticks;
			if (player.locate(&step, &ticks)) {
				note.offset = ticks;
				if (pattern.steps[step].add(note)) {
					n.recorded = step;
					n.start = player.now;
					n.slot = player.stepLength();
				}
			}
		}
		else {
			if (!chord)
				pattern.steps[cursor].clear();
			chord = true;
			pattern.steps[cursor].add(note);
		}
	}

	void noteUp(int source) {
		Live& n = live[source];
		if (!n.on)
			return;
		n.on = false;
		player.release(n.id);
		if (n.recorded >= 0) {
			Step& s = pattern.steps[n.recorded];
			int i = s.indexOf(n.pitch);
			if (i >= 0)
				s.notes[i].length = (int16_t) std::clamp(std::lround((player.now - n.start) / n.slot * TICKS), 1L, long(DRONE - 1));
		}
		if (chord && !anyHeld()) {
			chord = false;
			cursor = (cursor + 1) % stepCount();
		}
	}

	void keyDown(int key) {
		if (!trackMode)
			noteDown(key, keyPitch(key), (keyPitch(key) - 60) / 12.f, 100);
		else if (keyValue(key) >= 0)
			params[STEP_LENGTH_PARAM].setValue(keyValue(key));
	}

	void handleControls(float dt) {
		settings.stepCount = (int) params[STEP_COUNT_PARAM].getValue();
		settings.stepLength = stepLengthOfKey((int) params[STEP_LENGTH_PARAM].getValue());
		settings.noteLength = NOTE_LENGTHS[(int) params[LENGTH_PARAM].getValue()];
		settings.quantize = params[QUANTIZE_PARAM].getValue();
		settings.swing = params[SWING_PARAM].getValue();

		for (int p = TRACK_PARAM; p < PARAMS_LEN; p++) {
			KeyGesture& g = gestures[p - TRACK_PARAM];
			if (KeyGesture::Event e = g.process(params[p].getValue(), dt, LONG_PRESS))
				onKey(p, e, g);
		}
		if (clearing >= 0.f) {
			const KeyGesture& stop = gestures[STOP_PARAM - TRACK_PARAM];
			if (!stop.down() || !trackMode)
				clearing = -1.f;
			else if ((clearing = stop.held) >= CLEAR_HOLD) {
				pattern.clear();
				clearing = -1.f;
			}
		}

		if (erasing) {
			int step, ticks;
			if (!recording || !playing || !anyHeld())
				erasing = false;
			else if (player.locate(&step, &ticks))
				for (const Live& n : live)
					if (n.on)
						pattern.steps[step].remove(n.pitch);
		}
	}

	/** A step acts when it's let go, so the module can tell a click from a long press, which picks it up as soon as it's
	long enough; a Ctrl+click picks it up straight away. The other keys act as they go down. */
	void onKey(int p, KeyGesture::Event e, KeyGesture& g) {
		if (p >= KEY_PARAM) {
			if (e == KeyGesture::PRESS)
				keyDown(p - KEY_PARAM);
			else if (e == KeyGesture::CLICK || e == KeyGesture::RELEASE)
				noteUp(p - KEY_PARAM);
		}
		else if (p >= STEP_PARAM) {
			int i = p - STEP_PARAM;
			bool hold = !trackMode && !g.handled && (e == KeyGesture::PRESS ? g.press == KEY_CTRL && ctrlClickHolds : e == KeyGesture::LONG_PRESS && longPressHolds);
			if (hold) {
				toggleHold(i);
				g.handled = true;
			}
			else if (e == KeyGesture::CLICK)
				clickStep(i);
		}
		else if (e == KeyGesture::PRESS) {
			switch (p) {
				case TRACK_PARAM: trackMode = !trackMode; break;
				case REC_PARAM: recording = !recording; cursor = 0; break;
				case PLAY_PARAM: play(); break;
				case STOP_PARAM: stop(); clearing = trackMode ? 0.f : -1.f; break;
				case OCTAVE_DOWN_PARAM: shift(-1); break;
				case OCTAVE_UP_PARAM: shift(1); break;
				case SHIFT_PARAM: shifted = !shifted; break;
			}
		}
	}

	/** The GATE input's channels play like keys, and while held their notes follow V/OCT. */
	void handleInputs() {
		int channels = inputs[GATE_INPUT].getChannels();
		for (int c = 0; c < CHANNELS; c++) {
			Live& n = live[KEYS + c];
			float volts = inputs[VOCT_INPUT].getPolyVoltage(c);
			switch (gates[c].processEvent(c < channels ? inputs[GATE_INPUT].getVoltage(c) : 0.f, 0.1f, 1.f)) {
				case dsp::SchmittTrigger::TRIGGERED: {
					int velocity = inputs[VELOCITY_INPUT].isConnected() ? (int) std::round(inputs[VELOCITY_INPUT].getPolyVoltage(c) * 12.7f) : 100;
					noteDown(KEYS + c, 60 + (int) std::round(volts * 12.f), volts, velocity);
					break;
				}
				case dsp::SchmittTrigger::UNTRIGGERED:
					noteUp(KEYS + c);
					break;
				default:
					if (n.on)
						if (Player::Channel* channel = player.find(n.id))
							channel->pitch = volts;
			}
		}
	}

	/** Runs the player at the tempo, or follows the clock: between pulses at the rate they last came, but never past
	where the next one is due, and on a pulse straight to it. */
	void runClock(float dt) {
		bool patched = inputs[CLOCK_INPUT].isConnected();
		if (patched != external) {
			external = patched;
			pulsePeriod = 0.f;
			if (playing)
				play();
		}
		if (resetIn.process(inputs[RESET_INPUT].getVoltage(), 0.1f, 1.f) && playing)
			play();
		if (advanceIn.process(inputs[ADVANCE_INPUT].getVoltage(), 0.1f, 1.f) && settings.stepLength == 0)
			player.trigger(pattern, settings);

		double perPulse = 4.0 / ppqn;
		double rate = external && pulsePeriod > 0.f ? perPulse / pulsePeriod : params[TEMPO_PARAM].getValue() / 15.0;
		double d = rate * dt;
		if (external) {
			sincePulse += dt;
			if (clockIn.process(inputs[CLOCK_INPUT].getVoltage(), 0.1f, 1.f)) {
				// The first pulse after a long gap only starts the count again.
				pulsePeriod = sincePulse * ppqn < 4.f ? sincePulse : 0.f;
				sincePulse = 0.f;
				if (armed) {
					armed = false;
					restart();
				}
				else if (player.running)
					pulses++;
			}
			if (player.running) {
				double due = pulses * 4.0 / ppqn;
				d = player.now < due ? due - player.now : pulsePeriod > 0.f ? std::min(d, due + perPulse - 1e-9 - player.now) : 0.0;
				d = std::max(d, 0.0);
			}
		}
		player.advance(d, pattern, settings);
		if (player.running) {
			long tick = (long) std::floor(player.now / perPulse + 1e-9);
			if (tick != lastTick) {
				lastTick = tick;
				clockOut.trigger(1e-3f);
			}
		}
	}

	void writeOutputs(float dt) {
		int n = clamp(settings.channels, 1, CHANNELS);
		for (int c = 0; c < n; c++) {
			Player::Channel& channel = player.channels[c];
			outputs[VOCT_OUTPUT].setVoltage(channel.pitch, c);
			outputs[GATE_OUTPUT].setVoltage(channel.gate && !channel.retrigger ? 10.f : 0.f, c);
			outputs[VELOCITY_OUTPUT].setVoltage(10.f * channel.velocity, c);
			channel.retrigger = false;
		}
		for (int o : {VOCT_OUTPUT, GATE_OUTPUT, VELOCITY_OUTPUT})
			outputs[o].setChannels(n);
		outputs[CLOCK_OUTPUT].setVoltage(clockOut.process(dt) ? 10.f : 0.f);
		outputs[RESET_OUTPUT].setVoltage(resetOut.process(dt) ? 10.f : 0.f);
	}

	void process(const ProcessArgs& args) override {
		if (controlDivider.process())
			handleControls(args.sampleTime * controlDivider.getDivision());
		handleInputs();
		runClock(args.sampleTime);
		writeOutputs(args.sampleTime);
		if (lightDivider.process())
			updateLights(args.sampleTime * lightDivider.getDivision());
	}

	/** How many steps the held step's longest note reaches over, itself included. */
	int heldSpan(int loop) const {
		const Step& s = pattern.steps[held];
		int span = 0;
		for (int n = 0; n < s.count; n++) {
			int ticks = s.notes[n].length ? s.notes[n].length : settings.noteLength;
			span = std::max(span, ticks == DRONE ? loop : (ticks + TICKS - 1) / TICKS);
		}
		return std::min(span, loop);
	}

	void setKeyLight(int param, NVGcolor c, float brightness) {
		int l = keyLight(param);
		lights[l].setBrightness(c.r * brightness);
		lights[l + 1].setBrightness(c.g * brightness);
		lights[l + 2].setBrightness(c.b * brightness);
	}

	void updateLights(float dt) {
		blink = std::fmod(blink + dt, 0.5f);
		bool on = blink < 0.25f;
		int count = stepCount(), head = player.running ? player.playing() : -1;
		int loop = held < count ? count : STEPS, span = held >= 0 ? heldSpan(loop) : 0;

		// Steps: orange for notes, white for the playhead (red while recording), blue for how long the held step's
		// notes last. In track mode, white up to the step count; holding STOP there fills them red until the pattern
		// clears.
		for (int i = 0; i < STEPS; i++) {
			NVGcolor c = led::WHITE;
			float b = 0.f;
			if (clearing >= 0.f) {
				c = led::RED;
				b = i < clearing / CLEAR_HOLD * STEPS;
			}
			else if (trackMode)
				b = i == count - 1 ? 1.f : i < count ? 0.15f : 0.f;
			else {
				if (!pattern.steps[i].empty()) {
					c = NOTE_COLOR;
					b = i < count ? 0.6f : 0.2f;
				}
				int d = i - held;
				if (d < 0)
					d += loop;
				if (held >= 0 && i < loop && d > 0 && d < span) {
					c = led::BLUE;
					b = 0.8f;
				}
				if (i == held)
					b = on ? std::max(b, 0.4f) : 0.f;
				if (recording && !playing && i == cursor) {
					c = led::RED;
					b = on;
				}
				if (i == head) {
					c = recording ? led::RED : led::WHITE;
					b = 1.f;
				}
			}
			setKeyLight(STEP_PARAM + i, c, b);
		}

		// Keys: the notes sounding, brighter for the ones held or on the held step. In track mode, the step length, in
		// blue like the held step's length on the steps.
		float level[KEYS] = {};
		auto light = [&](int pitch, float b) {
			int k = pitch - keyPitch(0);
			if (k >= 0 && k < KEYS)
				level[k] = std::max(level[k], b);
		};
		if (trackMode) {
			for (int v = 0; v < 10; v++)
				level[valueKey(v)] = v == (int) params[STEP_LENGTH_PARAM].getValue() ? 1.f : 0.15f;
		}
		else {
			for (const Player::Channel& c : player.channels)
				if (c.gate)
					light(60 + (int) std::round(c.pitch * 12.f), 0.4f);
			for (const Live& n : live)
				if (n.on)
					light(n.pitch, 1.f);
			if (held >= 0)
				for (int n = 0; n < pattern.steps[held].count; n++)
					light(pattern.steps[held].notes[n].pitch, 1.f);
		}
		for (int k = 0; k < KEYS; k++)
			setKeyLight(KEY_PARAM + k, trackMode ? led::BLUE : NOTE_COLOR, level[k]);
		for (int c = 0; c < CHANNELS; c++) {
			const Player::Channel& channel = player.channels[c];
			lights[CHANNEL_LIGHT + c].setBrightness(channel.gate && c < settings.channels ? 0.2f + 0.8f * channel.velocity : 0.f);
		}

		setKeyLight(TRACK_PARAM, led::WHITE, trackMode);
		setKeyLight(REC_PARAM, led::RED, recording);
		setKeyLight(PLAY_PARAM, led::GREEN, playing && (!armed || on));
		setKeyLight(STOP_PARAM, led::RED, clearing >= 0.f);
		setKeyLight(OCTAVE_DOWN_PARAM, erasing ? led::RED : led::WHITE, erasing ? on : octave < 0 ? 0.2f - 0.2f * octave : 0.f);
		setKeyLight(OCTAVE_UP_PARAM, led::WHITE, octave > 0 ? 0.2f + 0.2f * octave : 0.f);
		setKeyLight(SHIFT_PARAM, led::WHITE, shifted);

		// The dials' LEDs: TEMPO flashes on the beat, the others glow as far as they're turned.
		lights[TEMPO_LIGHT].setBrightness(player.running && std::fmod(player.now, 4.0) < 1.0 ? 1.f : 0.15f);
		lights[SWING_LIGHT].setBrightness(0.15f + 0.85f * (settings.swing - 0.5f) / 0.25f);
		lights[LENGTH_LIGHT].setBrightness(0.15f + 0.85f * params[LENGTH_PARAM].getValue() / (std::size(NOTE_LENGTHS) - 1));
		lights[QUANTIZE_LIGHT].setBrightness(0.15f + 0.85f * settings.quantize);
		lights[INTERNAL_LIGHT].setBrightness(!external);
		lights[EXTERNAL_LIGHT].setBrightness(external);
		shownTempo = !external ? params[TEMPO_PARAM].getValue() : pulsePeriod > 0.f ? 60.f / (pulsePeriod * ppqn) : -1.f;

		for (int i = 0; i < INPUTS_LEN; i++)
			meter(inputMeters[i], inputs[i], INPUT_LIGHT + 2 * i, dt);
		for (int i = 0; i < OUTPUTS_LEN; i++)
			meter(outputMeters[i], outputs[i], OUTPUT_LIGHT + 2 * i, dt);
	}

	void meter(Meter& m, engine::Port& port, int light, float dt) {
		float level = 0.f;
		for (int c = 0; c < port.getChannels(); c++)
			level = std::fmax(level, std::fabs(port.getVoltage(c)));
		m.process(level / 10.f, level > 10.f, dt);
		lights[light].setBrightness(m.peak);
		lights[light + 1].setBrightness(m.clip > 0.f);
	}

	void onReset(const ResetEvent& e) override {
		Module::onReset(e);
		pattern.clear();
		memory.clear();
		memory.add(Note());
		player.panic();
		playing = armed = recording = trackMode = shifted = erasing = chord = false;
		held = -1;
		cursor = octave = 0;
		ppqn = 4;
		settings.channels = 4;
		recorded = pattern;
	}

	/** Buttons are saved like any param, so one held down while saving would come back stuck. */
	void paramsFromJson(json_t* root) override {
		Module::paramsFromJson(root);
		for (int i = TRACK_PARAM; i < PARAMS_LEN; i++)
			params[i].setValue(0.f);
	}

	/** The pattern is a list of steps, each a list of notes: [pitch, velocity, micro timing, length]. */
	json_t* dataToJson() override {
		json_t* root = json_object();
		json_t* steps = json_array();
		for (const Step& s : pattern.steps) {
			json_t* notes = json_array();
			for (int i = 0; i < s.count; i++) {
				const Note& n = s.notes[i];
				json_array_append_new(notes, json_pack("[iiii]", n.pitch, n.velocity, n.offset, n.length));
			}
			json_array_append_new(steps, notes);
		}
		json_object_set_new(root, "pattern", steps);
		json_object_set_new(root, "octave", json_integer(octave));
		json_object_set_new(root, "ppqn", json_integer(ppqn));
		json_object_set_new(root, "channels", json_integer(settings.channels));
		json_object_set_new(root, "longPressHolds", json_boolean(longPressHolds));
		json_object_set_new(root, "ctrlClickHolds", json_boolean(ctrlClickHolds));
		return root;
	}

	void dataFromJson(json_t* root) override {
		if (json_t* steps = json_object_get(root, "pattern")) {
			pattern.clear();
			size_t i, j;
			json_t *notes, *n;
			json_array_foreach(steps, i, notes) {
				if (i >= STEPS)
					break;
				json_array_foreach(notes, j, n) {
					int pitch, velocity, offset, length;
					if (json_unpack(n, "[iiii]", &pitch, &velocity, &offset, &length) == 0)
						pattern.steps[i].add({int8_t(clamp(pitch, 0, 127)), uint8_t(clamp(velocity, 1, 127)),
							int8_t(clamp(offset, MIN_OFFSET, MAX_OFFSET)), int16_t(clamp(length, 0, DRONE))});
				}
			}
		}
		recorded = pattern;
		if (json_t* j = json_object_get(root, "octave"))
			octave = clamp((int) json_integer_value(j), -3, 3);
		if (json_t* j = json_object_get(root, "ppqn"))
			ppqn = std::max((int) json_integer_value(j), 1);
		if (json_t* j = json_object_get(root, "channels"))
			settings.channels = clamp((int) json_integer_value(j), 1, CHANNELS);
		if (json_t* j = json_object_get(root, "longPressHolds"))
			longPressHolds = json_boolean_value(j);
		if (json_t* j = json_object_get(root, "ctrlClickHolds"))
			ctrlClickHolds = json_boolean_value(j);
	}
};


/** The pattern changes in the audio thread, from the keys and inputs, so the widget notices each change and saves it
here for undo, rather than being told. */
struct PatternChange : history::ModuleAction {
	Pattern before, after;

	void set(const Pattern& p) {
		if (Sequencer* m = dynamic_cast<Sequencer*>(APP->engine->getModule(moduleId)))
			m->pattern = m->recorded = p;
	}
	void undo() override {
		set(before);
	}
	void redo() override {
		set(after);
	}
};


/** One setting of every note on a step, for its context menu: velocity, micro timing or length. */
struct NoteField : Quantity {
	enum Field { VELOCITY, OFFSET, LENGTH };
	Step* step;
	Field field;

	NoteField(Step* step, Field field) : step(step), field(field) {}

	float getMinValue() override {
		return field == VELOCITY ? 1.f : field == OFFSET ? MIN_OFFSET : 0.f;
	}
	float getMaxValue() override {
		return field == VELOCITY ? 127.f : field == OFFSET ? MAX_OFFSET : LONGEST + 1;
	}
	float getDefaultValue() override {
		return field == VELOCITY ? 100.f : 0.f;
	}
	float getValue() override {
		const Note& n = step->notes[0];
		return field == VELOCITY ? n.velocity : field == OFFSET ? n.offset : n.length == DRONE ? LONGEST + 1 : n.length;
	}
	void setValue(float value) override {
		int v = (int) std::round(clamp(value, getMinValue(), getMaxValue()));
		for (int i = 0; i < step->count; i++) {
			Note& n = step->notes[i];
			if (field == VELOCITY)
				n.velocity = v;
			else if (field == OFFSET)
				n.offset = v;
			else
				n.length = v > LONGEST ? DRONE : v;
		}
	}
	std::string getLabel() override {
		return field == VELOCITY ? "Velocity" : field == OFFSET ? "Micro timing" : "Length";
	}
	std::string getDisplayValueString() override {
		int v = (int) getValue();
		if (field == VELOCITY)
			return string::f("%d", v);
		if (field == OFFSET)
			return string::f("%+d ticks", v);
		return v == 0 ? "Default" : v > LONGEST ? "Drone" : string::f("%.3g steps", v / float(TICKS));
	}
};

struct NoteSlider : ui::Slider {
	explicit NoteSlider(Quantity* q) {
		quantity = q;
		box.size.x = 220.f;
	}
	~NoteSlider() {
		delete quantity;
	}
};

/** A step key, whose context menu edits its notes. */
struct StepKey : RoundKey {
	Sequencer* sequencer() {
		return module ? static_cast<Sequencer*>(module) : nullptr;
	}

	void appendContextMenu(Menu* menu) override {
		Sequencer* m = sequencer();
		if (!m)
			return;
		Step* step = &m->pattern.steps[paramId - Sequencer::STEP_PARAM];
		menu->addChild(new MenuSeparator);
		if (step->empty()) {
			menu->addChild(createMenuLabel("Empty step"));
			return;
		}
		for (NoteField::Field f : {NoteField::VELOCITY, NoteField::OFFSET, NoteField::LENGTH})
			menu->addChild(new NoteSlider(new NoteField(step, f)));
		menu->addChild(createMenuItem("Clear", "", [=]() { step->clear(); }));
	}
};


/** Three seven-segment digits showing the tempo, or dashes while an external clock is still being measured. */
struct TempoDisplay : widget::Widget {
	Sequencer* module = nullptr;
	static constexpr uint8_t DIGITS[10] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f}, DASH = 0x40;

	/** A segment from p to q, pointed at both ends and pulled back from them a little. */
	static void segment(NVGcontext* vg, math::Vec p, math::Vec q, float r) {
		math::Vec u = q.minus(p).normalize(), n(-u.y, u.x);
		float gap = 0.25f * r;
		nvgMoveTo(vg, VEC_ARGS(p.plus(u.mult(gap))));
		nvgLineTo(vg, VEC_ARGS(p.plus(u.mult(gap + r)).plus(n.mult(r))));
		nvgLineTo(vg, VEC_ARGS(q.minus(u.mult(gap + r)).plus(n.mult(r))));
		nvgLineTo(vg, VEC_ARGS(q.minus(u.mult(gap))));
		nvgLineTo(vg, VEC_ARGS(q.minus(u.mult(gap + r)).minus(n.mult(r))));
		nvgLineTo(vg, VEC_ARGS(p.plus(u.mult(gap + r)).minus(n.mult(r))));
		nvgClosePath(vg);
	}

	/** The segments of every digit that are lit (or, for the ghosts behind them, unlit). */
	void drawDigits(NVGcontext* vg, bool lit, NVGcolor color) {
		float tempo = module ? module->shownTempo.load() : 120.f;
		int value = std::min((int) std::round(tempo), 999);
		float pad = mm2px(1.8f), h = box.size.y - 2 * pad, w = 0.5f * h, r = 0.06f * h;
		float gap = (box.size.x - 2 * pad - 3 * w) / 2;
		nvgSave(vg);
		nvgTranslate(vg, 0, box.size.y / 2);
		nvgSkewX(vg, -0.1f);
		nvgTranslate(vg, 0, -box.size.y / 2);
		nvgBeginPath(vg);
		for (int d = 0; d < 3; d++) {
			int place = 2 - d, digit = value / (place == 2 ? 100 : place == 1 ? 10 : 1) % 10;
			uint8_t mask = tempo < 0.f ? DASH : value < std::pow(10, place) && place > 0 ? 0 : DIGITS[digit];
			float x = pad + d * (w + gap), y = pad;
			const math::Vec corners[6] = {{x, y}, {x + w, y}, {x + w, y + h / 2}, {x + w, y + h}, {x, y + h}, {x, y + h / 2}};
			const int ends[7][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 4}, {5, 4}, {0, 5}, {5, 2}};
			for (int s = 0; s < 7; s++)
				if (bool(mask & (1 << s)) == lit)
					segment(vg, corners[ends[s][0]], corners[ends[s][1]], r);
		}
		nvgFillColor(vg, color);
		nvgFill(vg);
		nvgRestore(vg);
	}

	void draw(const DrawArgs& args) override {
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, mm2px(1.f));
		nvgFillColor(args.vg, nvgRGB(0x14, 0x15, 0x17));
		nvgFill(args.vg);
		drawDigits(args.vg, false, nvgTransRGBAf(NOTE_COLOR, 0.08f));
	}

	void drawLayer(const DrawArgs& args, int layer) override {
		if (layer == 1)
			drawDigits(args.vg, true, NOTE_COLOR);
		Widget::drawLayer(args, layer);
	}
};


struct SequencerWidget : ModuleWidget {
	SequencerWidget(Sequencer* module) {
		setModule(module);
		std::string panel = asset::plugin(pluginInstance, "res/Sequencer.svg");
		setPanel(createPanel(panel, asset::plugin(pluginInstance, "res/Sequencer-dark.svg")));
		addScrews(this);

		PanelLayout layout(panel);
#define AT(ID) layout.center(#ID), module, Sequencer::ID
		TempoDisplay* display = createWidget<TempoDisplay>(math::Vec());
		display->box = layout.box("TEMPO_WIDGET");
		display->module = module;
		addChild(display);
		addChild(createLightCentered<SmallSimpleLight<NoteLight>>(AT(INTERNAL_LIGHT)));
		addChild(createLightCentered<SmallSimpleLight<NoteLight>>(AT(EXTERNAL_LIGHT)));
		for (int i = 0; i < CHANNELS; i++)
			addChild(createLightCentered<DotLight<NoteLight>>(layout.center(string::f("CHANNEL%d_LIGHT", i)), module, Sequencer::CHANNEL_LIGHT + i));

		addParam(createParamCentered<SmallDial>(AT(TEMPO_PARAM))->color(0));
		addParam(createParamCentered<SmallDial>(AT(SWING_PARAM))->color(1));
		addParam(createParamCentered<SmallDial>(AT(LENGTH_PARAM))->color(2));
		addParam(createParamCentered<SmallDial>(AT(QUANTIZE_PARAM))->color(3));
		addChild(createLightCentered<DotLight<ColorLight<0>>>(AT(TEMPO_LIGHT)));
		addChild(createLightCentered<DotLight<ColorLight<1>>>(AT(SWING_LIGHT)));
		addChild(createLightCentered<DotLight<ColorLight<2>>>(AT(LENGTH_LIGHT)));
		addChild(createLightCentered<DotLight<ColorLight<3>>>(AT(QUANTIZE_LIGHT)));

		// Every key, with the symbol its LED lights: an icon for the function keys, a number for the black keys.
		static const char* const functions[][2] = {{"TRACK", "track"}, {"REC", "rec"}, {"PLAY", "play"}, {"STOP", "stop"},
			{"OCTAVE_DOWN", "minus"}, {"OCTAVE_UP", "plus"}, {"SHIFT", "shift"}};
		for (int p = Sequencer::TRACK_PARAM; p < Sequencer::PARAMS_LEN; p++) {
			int light = Sequencer::keyLight(p);
			std::string name, symbol = "dot";
			RoundKey* key;
			if (p >= Sequencer::KEY_PARAM) {
				int k = p - Sequencer::KEY_PARAM;
				name = string::f("KEY%d", k);
				if (keyValue(k) >= 0)
					symbol = std::to_string((keyValue(k) + 1) % 10);
				key = createLightParam<RoundKey>(math::Vec(), module, p, light);
			}
			else if (p >= Sequencer::STEP_PARAM) {
				name = string::f("STEP%d", p - Sequencer::STEP_PARAM);
				key = createLightParam<StepKey>(math::Vec(), module, p, light);
			}
			else {
				name = functions[p - Sequencer::TRACK_PARAM][0];
				symbol = functions[p - Sequencer::TRACK_PARAM][1];
				key = createLightParam<RoundKey>(math::Vec(), module, p, light);
			}
			key->setRing(layout.box(name + "_PARAM"), symbol);
			addParam(key);
		}

		static const char* const inputs[] = {"CLOCK", "RESET", "ADVANCE", "VOCT", "GATE", "VELOCITY"};
		static const char* const outputs[] = {"CLOCK", "RESET", "VOCT", "GATE", "VELOCITY"};
		for (int i = 0; i < Sequencer::INPUTS_LEN; i++) {
			addInput(createInputCentered<ThemedPJ301MPort>(layout.center(string::f("%s_INPUT", inputs[i])), module, i));
			addChild(createLightCentered<DotLight<GreenRedLight>>(layout.center(string::f("%s_INPUT_LIGHT", inputs[i])), module, Sequencer::INPUT_LIGHT + 2 * i));
		}
		for (int i = 0; i < Sequencer::OUTPUTS_LEN; i++) {
			addOutput(createOutputCentered<ThemedPJ301MPort>(layout.center(string::f("%s_OUTPUT", outputs[i])), module, i));
			addChild(createLightCentered<DotLight<GreenRedLight>>(layout.center(string::f("%s_OUTPUT_LIGHT", outputs[i])), module, Sequencer::OUTPUT_LIGHT + 2 * i));
		}
#undef AT
	}

	void step() override {
		Sequencer* m = getModule<Sequencer>();
		if (m && !(m->pattern == m->recorded)) {
			PatternChange* h = new PatternChange;
			h->name = "edit pattern";
			h->moduleId = m->id;
			h->before = m->recorded;
			h->after = m->recorded = m->pattern;
			APP->history->push(h);
		}
		ModuleWidget::step();
	}

	void appendContextMenu(Menu* menu) override {
		Sequencer* module = getModule<Sequencer>();
		std::vector<std::string> ppqn, channels;
		for (int n : Track::PPQN)
			ppqn.push_back(string::f("%d PPQN", n));
		for (int n = 1; n <= CHANNELS; n++)
			channels.push_back(string::f("%d", n));
		menu->addChild(new MenuSeparator);
		menu->addChild(createIndexSubmenuItem("Clock pulses per beat", ppqn,
			[=]() { return std::find(std::begin(Track::PPQN), std::end(Track::PPQN), module->ppqn) - std::begin(Track::PPQN); },
			[=](size_t i) { module->ppqn = Track::PPQN[i]; }));
		menu->addChild(createIndexSubmenuItem("Polyphony channels", channels,
			[=]() { return module->settings.channels - 1; },
			[=](size_t i) { module->settings.channels = i + 1; }));
		menu->addChild(createBoolPtrMenuItem("Long-press toggles hold", "", &module->longPressHolds));
		menu->addChild(createBoolPtrMenuItem(RACK_MOD_CTRL_NAME "-click toggles hold", "", &module->ctrlClickHolds));
	}
};

} // namespace opz

Model* modelSequencer = createModel<opz::Sequencer, opz::SequencerWidget>("Sequencer");
