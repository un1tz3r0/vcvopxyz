#pragma once
#include "../plugin.hpp"
#include "../dsp/Adsr.hpp"
#include "../dsp/Svf.hpp"
#include "Lfo.hpp"
#include "../TripleBuffer.hpp"
#include "Bus.hpp"
#include "Kit.hpp"
#include <atomic>

/*
An OP-Z track's sound, whatever plays it: four dials that step through four pages (sound, envelope, LFO, mix) plus
the track settings. On the arp track, the arp's dials take the LFO page's place, and the LFO is off. Its notes come
from its track's lane of the Sequencer (see Bus.hpp), or from a breakout's inputs. Everything after the voices is
shared: the LFO, the filter, the level and pan, the outputs, the lights and the snapshots a Screen reads. A synth
track (SynthTrack.hpp), a drum track (DrumTrack.hpp) and an FX track (FxTrack.hpp) differ only in their voices, their
two sound dials and their note styles.

A core holds no Module. The Sequencer keeps one for every plug a track can play (Host.hpp), and a core reads and
writes its track's params, the jacks of the breakout bound to it, and its lights through pointers, by the ids in
TrackIds, so its code reads as a module's would.
*/
namespace opz {

enum Target { TARGET_P1, TARGET_P2, TARGET_FILTER, TARGET_RESONANCE, TARGET_ATTACK, TARGET_PITCH, TARGET_PAN, TARGET_VOLUME, TARGETS_LEN };

/** An FX track's notes set its effect's time by key: two octaves of them from F3, the Sequencer's keys before − and +
move them. */
constexpr int FX_KEYS = 24, FX_LOWEST = 53;

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

struct OctavesQuantity : ParamQuantity {
	std::string getDisplayValueString() override {
		int n = Arp{0, 0, 0, getValue()}.octaves();
		return string::f("%d octave%s", n, n > 1 ? "s" : "");
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


/** A sound dial's travel in its plug's own units. The Sequencer keeps both sound dials as shares of their travel, 0
to 1, whichever plug plays, so steps, scenes and presets hold them the same way; a core turns a share into what its
voices take. */
struct SoundRange {
	float min = 0.f, max = 1.f;
	bool snap = false;

	float units(float share) const {
		float v = min + share * (max - min);
		return snap ? std::round(v) : v;
	}
};


/** The ids a track's params, jacks and lights go by, in its core and its module alike. */
struct TrackIds {
	enum ParamId {
		P1_PARAM, P2_PARAM, FILTER_PARAM, RESONANCE_PARAM,
		ATTACK_PARAM, DECAY_PARAM, SUSTAIN_PARAM, RELEASE_PARAM,
		LFO_AMOUNT_PARAM, LFO_SPEED_PARAM, LFO_TARGET_PARAM, LFO_SHAPE_PARAM,
		FX1_PARAM, FX2_PARAM, PAN_PARAM, LEVEL_PARAM,
		STYLE_PARAM, GLIDE_PARAM,
		ARP_SPEED_PARAM, ARP_PATTERN_PARAM, ARP_STYLE_PARAM, ARP_RANGE_PARAM,
		PARAMS_LEN
	};
	static_assert(PARAMS_LEN == SOUND_PARAMS && ARP_SPEED_PARAM == ARP_PARAMS, "a step locks any of a track's params");
	// The jacks are a breakout's, which hands its track to other modules. Ids are append-only, since patches save them.
	enum InputId { VOCT_INPUT, GATE_INPUT, VELOCITY_INPUT, CLOCK_INPUT, RESET_INPUT, EXT_INPUT, RETURN_L_INPUT, RETURN_R_INPUT, INPUTS_LEN };
	enum OutputId {
		LEFT_OUTPUT, RIGHT_OUTPUT, FX1_OUTPUT, FX2_OUTPUT,
		CV_VOCT_OUTPUT, CV_GATE_OUTPUT, CV_ENV_OUTPUT, CV_LFO_OUTPUT, CV_P1_OUTPUT, CV_P2_OUTPUT,
		CV_FILTER_OUTPUT, CV_RESONANCE_OUTPUT, CV_FX1_OUTPUT, CV_FX2_OUTPUT,
		CV_TRIGGER_OUTPUT, CV_SOUND_OUTPUT, CV_VELOCITY_OUTPUT,
		CV_SEND_L_OUTPUT, CV_SEND_R_OUTPUT, CV_TIME_OUTPUT,
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
	static constexpr const char* PAGE_NAMES[PAGES_LEN] = {"SOUND", "ENVELOPE", "LFO", "MIX", "TRACK"};
};


/** What every track has in common, whatever its engine, and all a Screen needs to know. */
struct TrackCore : TrackIds {
	using ProcessArgs = Module::ProcessArgs;

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

	// The params, jacks and lights it plays by, by the ids in TrackIds, from bind(): its track's params on the
	// Sequencer, its breakout's jacks, and lights for the meters.
	Param* params = nullptr;
	ParamQuantity** paramQuantities = nullptr;
	Input* inputs = nullptr;
	Output* outputs = nullptr;
	Light* lights = nullptr;

	/** Whether it plays the arp track, whose LFO page is the arp's. */
	bool arp() const {
		return busTrack == ARP_TRACK;
	}

	const char* pageName(int p) const {
		return p == PAGE_LFO && arp() ? "ARP" : PAGE_NAMES[p];
	}

	bool drum = false;           // plays one of tracks 1 to 4 rather than 5 to 8
	bool effect = false;         // plays FX 1 or 2
	int timeKey = -1;            // an FX module's time, as the key its notes set it with
	float secondsPerBeat = 0.5f; // 120 BPM until a clock says otherwise
	int ppqn = 4;                // clock pulses per beat: 4 is one per sixteenth, one per step
	int page = PAGE_SOUND;
	bool trackMode = false;
	int busTrack = -1;           // the Sequencer's track it plays, set once
	SoundRange ranges[2];        // its sound dials' travel, from its plug
	// What the Sequencer hands it each frame: the lanes and the master's keys, and where its sound goes.
	int64_t frame = 0;
	const Downstream* bus = nullptr;
	Slot* slot = nullptr;
	bool silenced = false;       // ringing out after a switch of plug: it takes no notes, and an effect no send
	bool following = false;      // a breakout's core beside the plug, for its CVs: its sound and lights go nowhere
	bool selected = false;       // the Sequencer's keys and dials are on its track

	int style = 0;               // the note style playing, which a change of the dial lets every note go for
	int channels = 0;            // the note source's channels
	int notesStarted = 0;        // ever, for the Sequencer's track keys to flash at
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
	xyz::Svf returnLowpasses[2], returnHighpasses[2];
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

	TrackCore() {
		controlDivider.setDivision(16);
		lightDivider.setDivision(64);
	}

	virtual ~TrackCore() = default;

	void bind(Param* p, ParamQuantity** q, Input* in, Output* out, Light* l) {
		params = p;
		paramQuantities = q;
		inputs = in;
		outputs = out;
		lights = l;
	}

	/** Takes over its track from the core that played it: the page and the master's keys, the clock and the LFO, so
	a switch of plug carries on where the last one was. Its dials catch up on its first sample. A core that `followed`
	the same notes beside it keeps its gates. */
	void takeOver(const TrackCore& from, bool followed = false) {
		page = from.page;
		trackMode = from.trackMode;
		masterTrackMode = from.masterTrackMode;
		pageRequests = from.pageRequests;
		selected = from.selected;
		ppqn = from.ppqn;
		secondsPerBeat = from.secondsPerBeat;
		lfo = from.lfo;
		lfoValue = from.lfoValue;
		std::copy(std::begin(from.dialValues), std::end(from.dialValues), dialValues);
		// Its gates start low, so the notes held as it comes in start on it, where a fresh trigger would wait for them
		// to fall first.
		if (!followed)
			for (dsp::SchmittTrigger& g : gates)
				g.s = dsp::SchmittTrigger::LOW;
		silenced = false;
		controlDivider.clock = controlDivider.division - 1;
	}

	/** What dial `d` turns on a page, or -1 for nothing. */
	virtual int dial(int page, int d) const {
		return page == PAGE_LFO && arp() ? ARP_SPEED_PARAM + d : DIALS[page][d];
	}

	/** What a note plays, for the Sequencer's keys, where that isn't just its pitch; for the UI thread. */
	virtual std::string soundName(int pitch) {
		return "";
	}

	/** A track that plays a kit, for the Screen to show and edit, on the UI thread: the kit playing, or null; a
	slice's new settings; and the slice played last. */
	virtual const Kit* kit() {
		return nullptr;
	}
	virtual void editSlice(int i, const Kit::Slice& to) {}
	/** A track with slots of its own, a kit player's, plays slot `s` now; for the audio thread. */
	virtual void pick(int s) {}
	virtual int lastSlice() {
		return 0;
	}
	/** Gets ready for a sample rate, on the UI thread, so that a switch of plug allocates nothing on the audio thread. */
	virtual void setSampleRate(float rate) {}

	virtual void noteOn(int channel) = 0;
	virtual void noteOff(int channel) = 0;
	virtual void releaseAll() = 0;
	/** A voice's envelope, and its velocity, for the lights and a Screen; null past the last voice. */
	virtual const xyz::Adsr* envelope(int voice, float* velocity) const = 0;

	/** A dial's name as the panel's legend prints it; the sound dials are named after their plug's. */
	std::string dialName(int param) {
		static const char* const names[] = {nullptr, nullptr, "FILTER", "RESO", "ATTACK", "DECAY", "SUSTAIN", "RELEASE",
			"AMOUNT", "SPEED", "TARGET", "SHAPE", "FX 1", "FX 2", "PAN", "LEVEL", "STYLE", "GLIDE", "SPEED", "PATTERN", "STYLE", "RANGE"};
		if (names[param])
			return names[param];
		// Its label after "Track 5 ", which the Sequencer's params all start with.
		std::string label = paramQuantities[param]->getLabel();
		if (label.rfind("Track ", 0) == 0 && label.find(' ', 6) != std::string::npos)
			label = label.substr(label.find(' ', 6) + 1);
		return string::uppercase(label);
	}

	bool hasDials(int p) const {
		for (int d = 0; d < 4; d++)
			if (dial(p, d) >= 0)
				return true;
		return false;
	}

	/** The next page along that has any dials: an FX module has only two. */
	int nextPage() const {
		for (int i = 1; i < PAGE_TRACK; i++)
			if (hasDials((page + i) % PAGE_TRACK))
				return (page + i) % PAGE_TRACK;
		return page;
	}

	/** How busy it looks on its NOTE light, 0 to 1: its loudest voice. */
	virtual float activity() {
		float loudest = 0.f, velocity;
		for (int i = 0; const xyz::Adsr* e = envelope(i, &velocity); i++)
			loudest = std::fmax(loudest, e->level * velocity);
		return loudest;
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
		lane = silenced ? nullptr : from;
		// Channels beyond the gate's count read as low, so dropping channels releases their notes.
		channels = silenced ? 0 : lane ? lane->channels : inputs[GATE_INPUT].getChannels();
		for (int c = 0; c < PORT_MAX_CHANNELS; c++) {
			float gate = c >= channels ? 0.f : lane ? 10.f * lane->gate[c] : inputs[GATE_INPUT].getVoltage(c);
			switch (gates[c].processEvent(gate, 0.1f, 1.f)) {
				case dsp::SchmittTrigger::TRIGGERED: noteOn(c); notesStarted++; break;
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
			showPage(bus->page == NEXT_PAGE ? nextPage() : hasDials(bus->page) ? bus->page : page);
		masterTrackMode = bus->trackMode;
		pageRequests = bus->pageRequests;
	}

	/** Its sound and FX sends this frame, for the Sequencer's mix, while its breakout's outputs are unpatched. */
	void sendUpstream(float left, float right, float mono) {
		if (!slot)
			return;
		Slot& s = *slot;
		s.frame = frame;
		bool mixed = !outputs[LEFT_OUTPUT].isConnected() && !outputs[RIGHT_OUTPUT].isConnected();
		s.left = mixed ? left : 0.f;
		s.right = mixed ? right : 0.f;
		for (int k = 0; k < FX_TRACKS; k++)
			s.send[k] = effect || outputs[FX1_OUTPUT + k].isConnected() ? 0.f : mono * (k ? controls.fx2 : controls.fx1);
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
		float amount = arp() ? 0.f : value(LFO_AMOUNT_PARAM);
		// Squaring the amount leaves room for subtle settings, like vibrato, near the middle.
		Controls& c = controls;
		c.lfo = amount * std::fabs(amount) * lfoValue;
		float offset = lfoInside ? c.lfo : 0.f;
		c.p1 = ranges[0].units(modulated(P1_PARAM, TARGET_P1, offset, sweep[SWEEP_SYNTH]));
		c.p2 = ranges[1].units(modulated(P2_PARAM, TARGET_P2, offset));
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

	/** Everything up to the voices, once a sample, once the Sequencer has handed over the bus: the tempo, the keys,
	the notes, the LFO and the dials. */
	void prepare(const ProcessArgs& args) {
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
		sendUpstream(panned[0], panned[1], mono);

		// A core ringing out after a switch, or following, leaves the lights to the one playing.
		if (lightDivider.process() && !silenced && !following)
			updateLights(sampleTime * lightDivider.getDivision());
	}

	/** A breakout's voice, back from other modules at RET, through whatever it doesn't send out: enveloped unless ENV
	is patched, filtered unless FILTER or RESO is, and at full level to an FX output whose amount is patched.
	`levels` are the voices' envelopes: a polyphonic return takes them channel by channel, a mono one the loudest. */
	void emitReturn(const float* levels, float loudest, float sampleTime) {
		bool enveloped = !outputs[CV_ENV_OUTPUT].isConnected();
		bool filtered = !outputs[CV_FILTER_OUTPUT].isConnected() && !outputs[CV_RESONANCE_OUTPUT].isConnected();
		float sides[2];
		for (int s = 0; s < 2; s++) {
			// RIGHT is normalled to LEFT, as on a stereo pair.
			Input& in = inputs[s && inputs[RETURN_R_INPUT].isConnected() ? RETURN_R_INPUT : RETURN_L_INPUT];
			int n = in.getChannels();
			float x = 0.f;
			for (int c = 0; c < n; c++)
				x += in.getVoltage(c) / 5.f * (!enveloped ? 1.f : n > 1 ? levels[c] : loudest);
			if (filtered) {
				returnLowpasses[s].tuneLike(lowpass);
				returnHighpasses[s].tuneLike(highpass);
				x = returnHighpasses[s].process(returnLowpasses[s].process(x).lp).hp;
			}
			sides[s] = x * controls.gain;
		}
		emit(sides[0], sides[1], outputs[CV_FX1_OUTPUT].isConnected() ? 1.f : controls.fx1,
			outputs[CV_FX2_OUTPUT].isConnected() ? 1.f : controls.fx2, sampleTime);
	}

	/** The dials every breakout sends out, 0 to 10V after the LFO. */
	void emitDialCVs() {
		outputs[CV_LFO_OUTPUT].setVoltage(5.f * controls.lfo);
		const std::pair<int, float> cvs[] = {{CV_FILTER_OUTPUT, controls.filter}, {CV_RESONANCE_OUTPUT, controls.resonance},
			{CV_FX1_OUTPUT, controls.fx1}, {CV_FX2_OUTPUT, controls.fx2}};
		for (auto [output, value] : cvs)
			outputs[output].setVoltage(10.f * value);
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
		lights[SELECTED_LIGHT].setBrightness(selected);
		lights[ACTIVE_LIGHT].setBrightnessSmooth(activity(), dt);

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

	/** One sample: the voices after prepare(), then emit() or emitReturn(). */
	virtual void process(const ProcessArgs& args) = 0;

	/** Its settings beside the params, back to how they start. */
	virtual void resetSettings() {
		ppqn = 4;
		page = PAGE_SOUND;
		trackMode = false;
	}

	/** Its settings beside the params, into its track's JSON. Track mode isn't saved, so a patch always opens on the
	dials' pages. */
	virtual void dataToJson(json_t* root) {
		json_object_set_new(root, "ppqn", json_integer(ppqn));
		json_object_set_new(root, "page", json_integer(page));
	}

	virtual void dataFromJson(json_t* root) {
		if (json_t* j = json_object_get(root, "ppqn"))
			ppqn = std::max((int) json_integer_value(j), 1);
		if (json_t* j = json_object_get(root, "page"))
			page = clamp((int) json_integer_value(j), 0, PAGE_TRACK - 1);
	}
};

} // namespace opz
