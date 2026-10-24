#pragma once
#include "Plugs.hpp"
#include "Hub.hpp"
#include "KitCore.hpp"

/*
A track as the Sequencer plays it: a core for every plug of its kind, and one for its kind's breakout, all bound to
the track's 22 params on the Sequencer, the breakout's jacks and the track's lights. One core plays at a time. A
switch of plug is a swap of pointers on the audio thread, with nothing made or freed: the core switched away from
lets go of its notes and rings out until it falls quiet, under the new one.

The black keys in track mode pick a plug from the track's ten slots. A drum track has one plug, the kit player,
whose slots are kits instead (KitCore.hpp).

A breakout bound to the track takes it over, by what is patched: with its audio outputs patched, the plug plays and
its sound leaves there instead of the mix (TrackCore::sendUpstream sees to that); with its note outputs patched and
its audio outputs not, the breakout's core plays instead, sending the notes out and taking the sound back in at its
return.
*/
namespace opz {

/** "Octave" as "octave", leaving "X mod" and "CV 1" as they are, to follow "Track 5 ". */
inline std::string lowerFirst(std::string s) {
	if (s.size() > 1 && std::isupper((unsigned char) s[0]) && std::islower((unsigned char) s[1]))
		s[0] = std::tolower((unsigned char) s[0]);
	return s;
}

/** What a track's params are called: "Track 5 filter". */
inline std::string trackParamName(int lane, const std::string& name) {
	return string::f("Track %d ", trackNumber(lane)) + lowerFirst(name);
}

/** A sound dial on the Sequencer, 0 to 1 over its travel, shown as its track's plug shows it. The plug is read once
per call from an atomic, and its dials never change, so a switch of plug can't leave the UI thread half way between
two names. */
struct PlugDial : ParamQuantity {
	const std::atomic<const Plug*>* plug = nullptr;
	int dial = 0;
	int lane = 0;

	/** The plug's own dial, at this one's value. */
	ParamQuantity* shown() {
		const Plug* p = plug ? plug->load() : nullptr;
		if (!p)
			return nullptr;
		ParamQuantity* q = p->dial(dial);
		q->getParam()->setValue(p->range(dial).units(getValue()));
		return q;
	}

	std::string getLabel() override {
		ParamQuantity* q = shown();
		return trackParamName(lane, q ? q->getLabel() : name);
	}
	std::string getUnit() override {
		ParamQuantity* q = shown();
		return q ? q->getUnit() : unit;
	}
	std::string getDisplayValueString() override {
		ParamQuantity* q = shown();
		return q ? q->getDisplayValueString() : ParamQuantity::getDisplayValueString();
	}
	void setDisplayValueString(std::string s) override {
		ParamQuantity* q = shown();
		if (!q)
			return ParamQuantity::setDisplayValueString(s);
		q->setDisplayValueString(s);
		setScaledValue(q->getScaledValue());
	}
	std::string getDescription() override {
		ParamQuantity* q = shown();
		return q ? q->getDescription() : description;
	}
};

/** The LFO's target, whose first two are its track's sound dials, named as its plug names them. */
struct TargetQuantity : SwitchQuantity {
	const std::atomic<const Plug*>* plug = nullptr;

	std::string getDisplayValueString() override {
		int i = (int) getValue();
		const Plug* p = plug ? plug->load() : nullptr;
		if (p && i < 2)
			return p->dial(i)->getLabel();
		return SwitchQuantity::getDisplayValueString();
	}
};

/** A track's 22 params on module `m`, from param `base` on. */
inline void configTrack(Module* m, int base, int lane, const std::atomic<const Plug*>* plug) {
	using I = TrackIds;
	int kind = laneKind(lane);
	auto name = [&](const char* n) { return trackParamName(lane, n); };
	for (int d = 0; d < 2; d++) {
		PlugDial* q = m->configParam<PlugDial>(base + I::P1_PARAM + d, 0.f, 1.f, 0.f, d ? "Sound 2" : "Sound 1");
		q->plug = plug;
		q->dial = d;
		q->lane = lane;
	}
	m->configParam<FilterQuantity>(base + I::FILTER_PARAM, 0.f, 1.f, 0.5f, name("Filter"))->description = "Low-pass to the left, high-pass to the right";
	m->configParam(base + I::RESONANCE_PARAM, 0.f, 1.f, 0.f, name("Resonance"), "%", 0.f, 100.f);
	m->configParam<TimeQuantity>(base + I::ATTACK_PARAM, 0.f, 1.f, 0.f, name("Attack"));
	m->configParam<TimeQuantity>(base + I::DECAY_PARAM, 0.f, 1.f, 0.5f, name("Decay"));
	m->configParam(base + I::SUSTAIN_PARAM, 0.f, 1.f, kind == DRUM ? 1.f : 0.7f, name("Sustain"), "%", 0.f, 100.f);
	m->configParam<TimeQuantity>(base + I::RELEASE_PARAM, 0.f, 1.f, kind == DRUM ? 0.4f : 0.5f, name("Release"));
	m->configParam(base + I::LFO_AMOUNT_PARAM, -1.f, 1.f, 0.f, name("LFO amount"), "%", 0.f, 100.f)->description = "Off in the middle; to the left the LFO pulls the target the other way";
	m->configParam<SpeedQuantity>(base + I::LFO_SPEED_PARAM, 0.f, 1.f, 0.75f, name("LFO speed"))->description = "Synced to the clock to the left, free to the right";
	std::vector<std::string> targets = kind == DRUM ? std::vector<std::string>{"Sound 1", "Sound 2", "Filter", "Resonance", "Attack", "Pitch, 2 octaves", "Pan", "Volume"}
		: kind == FX ? std::vector<std::string>{"Sound 1", "Sound 2", "Filter", "Resonance", "Time", "Send", "Pan", "Volume"}
		: std::vector<std::string>{"Sound 1", "Sound 2", "Filter", "Resonance", "Attack", "Pitch", "Pan", "Volume"};
	TargetQuantity* target = m->configSwitch<TargetQuantity>(base + I::LFO_TARGET_PARAM, 0.f, TARGETS_LEN - 1, TARGET_FILTER, name("LFO target"), targets);
	target->plug = plug;
	if (kind == FX)
		target->description = "Time moves the delay or predelay up to an octave either way; send is the level into the effect";
	m->configSwitch(base + I::LFO_SHAPE_PARAM, 0.f, Lfo::SHAPES_LEN - 1, Lfo::SINE, name("LFO shape"),
		{"Sine", "Triangle", "Square", "Saw", "Random", "External (a breakout's EXT input)",
		 "Bell, per note", "Triangle, per note", "Square, per note", "Saw, per note", "Random, per note", "Single saw, per note"});
	m->configParam(base + I::FX1_PARAM, 0.f, 1.f, 0.f, name("FX 1 send"), "%", 0.f, 100.f)->description = "To FX 1, or a breakout's FX 1 output while it is patched, after the level";
	m->configParam(base + I::FX2_PARAM, 0.f, 1.f, 0.f, name("FX 2 send"), "%", 0.f, 100.f)->description = "To FX 2, or a breakout's FX 2 output while it is patched, after the level";
	m->configParam(base + I::PAN_PARAM, -1.f, 1.f, 0.f, name("Pan"), "%", 0.f, 100.f);
	m->configParam(base + I::LEVEL_PARAM, 0.f, 1.f, kind == FX ? 1.f : 0.7f, name("Level"), "%", 0.f, 100.f);
	if (kind == DRUM)
		m->configSwitch(base + I::STYLE_PARAM, 0.f, 3.f, 0.f, name("Note style"), {"Retrig", "Mono", "Gate", "Loop"})->description =
			"Retrig plays each sound to its end, and mono cuts the last one off; gate stops it with its note, and loop repeats it while the note is held";
	else if (kind == FX)
		m->configSwitch(base + I::STYLE_PARAM, 0.f, 0.f, 0.f, name("Note style"), {"None"})->description = "FX tracks have no note style";
	else
		m->configSwitch(base + I::STYLE_PARAM, 0.f, 2.f, 0.f, name("Note style"), {"Poly", "Mono", "Legato"})->description =
			"Mono plays the newest held note and retriggers on every change; legato slides between held notes without retriggering";
	m->configParam(base + I::GLIDE_PARAM, 0.f, 1.f, 0.f, name("Portamento"), "%", 0.f, 100.f)->description = kind == SYNTH ? "Mono and legato only" : "Synth tracks only";
	m->configSwitch(base + I::ARP_SPEED_PARAM, 0.f, ARP_SPEEDS, 4.f, name("Arp speed"), {"Off", "1/4", "1/8", "1/8 triplets", "1/16",
		"1/16 triplets", "1/32", "1/32 triplets", "1/64"})->description = "Arp track only. Off plays its notes as they are";
	m->configSwitch(base + I::ARP_PATTERN_PARAM, 0.f, ARP_PATTERNS - 1, ARP_UP, name("Arp pattern"), {"As played", "Up", "Down", "Up and down",
		"Down and up", "Random"})->description = "Arp track only";
	m->configSwitch(base + I::ARP_STYLE_PARAM, 0.f, ARP_STYLES - 1, 0.f, name("Arp style"), {"As set", "LHLLHLLH", "HLLHLLHL", "LLHLLHLL", "LMH",
		"LL, rest"})->description = "Arp track only. A rhythm of accents: H as played, M three quarters, L half, and rests";
	m->configParam<OctavesQuantity>(base + I::ARP_RANGE_PARAM, 0.f, 1.f, 0.f, name("Arp range"))->description = "Arp track only. Higher octaves of the notes";
}


struct HostedTrack : TrackIds {
	static constexpr int SLOTS = 10;

	int lane = 0, kind = SYNTH;
	std::vector<std::unique_ptr<TrackCore>> cores; // plugs(kind) in order, then the breakout's
	std::vector<const Plug*> plugOf;               // each core's plug
	TrackCore* active = nullptr;                   // the audio thread's: the core playing
	TrackCore* ringing = nullptr;                  // and the one it took over from, until it falls quiet
	float quiet = 0.f, rung = 0.f;                 // seconds the ringing core has been quiet, and has rung
	std::atomic<TrackCore*> shown{nullptr};        // the core playing, for the UI thread
	std::atomic<const Plug*> plug{nullptr};        // and its plug
	// A synth or FX track's plugs on the black keys, by their place in plugs(kind), or -1 for none. The UI thread
	// sets them, the audio thread reads them.
	std::atomic<int> slots[SLOTS];
	std::atomic<int> slot{0};                      // the black key picked
	int picked = -1;                               // the slot the kit player was last told of; the audio thread's
	bool bound = false;                            // a breakout had the track last frame; the audio thread's
	Input inputs[INPUTS_LEN];                      // the breakout's jacks, as it sent them
	Output outputs[OUTPUTS_LEN];
	Light lights[LIGHTS_LEN];
	Slot sound;                                    // what the cores played this frame, for the mix

	HostedTrack() {
		for (std::atomic<int>& s : slots)
			s = -1;
	}

	/** Makes its cores, bound to the params of module `m` from `base` on, which configTrack() has configured. */
	void init(int lane, Module* m, int base) {
		this->lane = lane;
		kind = laneKind(lane);
		std::vector<const Plug*> list = plugs(kind);
		list.push_back(breakoutPlug(kind));
		for (const Plug* p : list) {
			std::unique_ptr<TrackCore> c = p->make();
			c->bind(&m->params[base], &m->paramQuantities[base], inputs, outputs, lights);
			c->busTrack = lane;
			c->ranges[0] = p->range(0);
			c->ranges[1] = p->range(1);
			cores.push_back(std::move(c));
			plugOf.push_back(p);
		}
		activate(cores.front().get());
	}

	TrackCore* breakoutCore() const {
		return cores.back().get();
	}

	/** The plug in a slot, by its place in plugs(kind), or -1. A drum track's one plug plays in every slot. */
	int plugIn(int s) const {
		return kind == DRUM ? 0 : slots[clamp(s, 0, SLOTS - 1)].load();
	}

	KitCore* kits() const {
		return kind == DRUM ? static_cast<KitCore*>(cores.front().get()) : nullptr;
	}

	/** Whether a slot holds anything to play, for either thread. */
	bool filled(int s) const {
		return kind == DRUM ? kits()->holds(s) : plugIn(s) >= 0;
	}

	TrackCore* slotCore() const {
		int p = plugIn(slot);
		return cores[p >= 0 ? p : 0].get();
	}

	void activate(TrackCore* c) {
		active = c;
		shown = c;
		plug = plugOf[std::find_if(cores.begin(), cores.end(), [=](const std::unique_ptr<TrackCore>& u) { return u.get() == c; }) - cores.begin()];
	}

	/** Plays the slot picked straight away, with nothing ringing: for a patch loaded or reset, while the engine waits. */
	void settle() {
		if (ringing)
			ringing->releaseAll();
		ringing = nullptr;
		picked = -1;
		activate(slotCore());
	}

	void setSampleRate(float rate) {
		for (std::unique_ptr<TrackCore>& c : cores)
			c->setSampleRate(rate);
	}

	/** Hands the track to another core. The one it leaves rings out, unless one is ringing already, which stops. */
	void switchTo(TrackCore* c) {
		if (ringing && ringing != c)
			ringing->releaseAll();
		ringing = active;
		ringing->silenced = true;
		quiet = rung = 0.f;
		c->takeOver(*active);
		activate(c);
		if (ringing == c)
			ringing = nullptr;
	}

	/** Whether what a breakout has patched hands it the track: its note outputs, without its audio outputs. */
	static bool replaces(int kind, uint32_t patched) {
		auto on = [=](int o) { return bool(patched >> o & 1); };
		if (on(LEFT_OUTPUT) || on(RIGHT_OUTPUT))
			return false;
		if (kind == FX)
			return on(CV_SEND_L_OUTPUT) || on(CV_SEND_R_OUTPUT);
		return on(CV_GATE_OUTPUT) || on(kind == DRUM ? CV_TRIGGER_OUTPUT : CV_VOCT_OUTPUT);
	}

	/** Takes the breakout's jacks as it sent them: its inputs, and its outputs patched or not, so a core sees them as
	a module sees its own. */
	void takeJacks(const JacksIn* in) {
		bound = in;
		for (int i = 0; i < INPUTS_LEN; i++) {
			int n = in ? in->inputs[i].channels : 0;
			inputs[i].channels = n;
			if (n)
				std::copy(std::begin(in->inputs[i].voltages), std::end(in->inputs[i].voltages), inputs[i].voltages);
		}
		uint32_t patched = in ? in->patched : 0;
		for (int o = 0; o < OUTPUTS_LEN; o++)
			outputs[o].channels = !(patched >> o & 1) ? 0 : std::max<int>(outputs[o].channels, 1);
	}

	/** Its patched outputs, to the breakout. */
	void giveJacks(JacksOut& out) const {
		for (int o = 0; o < OUTPUTS_LEN; o++)
			if (int n = outputs[o].channels) {
				out.outputs[o].channels = n;
				std::copy(std::begin(outputs[o].voltages), std::end(outputs[o].voltages), out.outputs[o].voltages);
			}
		out.activity = lights[ACTIVE_LIGHT].value;
		out.selected = active->selected;
	}

	void run(TrackCore* c, const Module::ProcessArgs& args, const Downstream* bus) {
		sound = Slot();
		c->frame = args.frame;
		c->bus = bus;
		c->slot = &sound;
		c->process(args);
	}

	/** One sample: the jacks from the breakout, if one has the track, then the core playing, and the one ringing out
	under it. Its sound is in `sound` afterwards. */
	void play(const Module::ProcessArgs& args, const Downstream* bus, const JacksIn* in) {
		takeJacks(in);
		TrackCore* want = in && replaces(kind, in->patched) ? breakoutCore() : slotCore();
		if (want != active)
			switchTo(want);
		if (int s = slot; s != picked) {
			picked = s;
			slotCore()->pick(s);
		}
		float left = 0.f, right = 0.f, send[FX_TRACKS] = {};
		if (ringing) {
			run(ringing, args, bus);
			left = sound.left;
			right = sound.right;
			std::copy(std::begin(sound.send), std::end(sound.send), send);
			// Quiet for a moment, or for longer after an effect, whose echoes have gaps; or rung for 10 seconds.
			quiet = std::fabs(left) + std::fabs(right) > 1e-4f ? 0.f : quiet + args.sampleTime;
			rung += args.sampleTime;
			if (quiet > (kind == FX ? 3.f : 0.1f) || rung > 10.f) {
				ringing->releaseAll();
				ringing = nullptr;
			}
		}
		run(active, args, bus);
		sound.left += left;
		sound.right += right;
		for (int k = 0; k < FX_TRACKS; k++)
			sound.send[k] += send[k];
	}
};

} // namespace opz
