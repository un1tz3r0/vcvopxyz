#pragma once
#include "Hub.hpp"
#include "../ui.hpp"

/*
A breakout: a module that hands one of the Sequencer's tracks to other modules through its jacks. It holds no sound of
its own. Its jacks travel over the hub (Hub.hpp) to the track's core inside the Sequencer, which plays them as a
module plays its own; what the breakout has patched decides what the track does (Host.hpp).

Its selector picks the track: Auto takes the first track of its kind that no breakout to its left has, as engines
once took theirs by their order along the row; a fixed track is its own, unless a breakout to its left has it already,
and then it has none and shows a dash.
*/
namespace opz {

struct BreakoutModule : Module, TrackIds {
	enum { TRACK_PARAM, BREAKOUT_PARAMS };

	int kind;
	Hub* hub = nullptr;                 // the audio thread's
	std::atomic<int> track{-1};         // the track it has, or -1
	uint32_t layoutSeen = ~0u;
	float selected = -1.f;              // the selector as last seen, to find a track again when it moves
	Meter inputMeters[INPUTS_LEN], outputMeters[OUTPUTS_LEN];
	dsp::ClockDivider lightDivider;

	static int firstTrack(int kind) {
		return kind == FX ? FX1 : kind == DRUM ? 0 : DRUM_TRACKS;
	}
	static int tracks(int kind) {
		return kind == FX ? FX_TRACKS : kind == DRUM ? DRUM_TRACKS : SYNTH_TRACKS;
	}

	explicit BreakoutModule(int kind) : kind(kind) {
		config(BREAKOUT_PARAMS, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		std::vector<std::string> labels = {"Auto, by its place along the row"};
		static const char* const names[LANES] = {"kick", "snare", "perc", "sample", "bass", "lead", "arp", "chord", "master", "FX 1", "FX 2"};
		for (int t = firstTrack(kind); t < firstTrack(kind) + tracks(kind); t++)
			labels.push_back(string::f("Track %d, %s", trackNumber(t), names[t]));
		configSwitch(TRACK_PARAM, 0.f, tracks(kind), 0.f, "Track", labels)->description = "Which of the Sequencer's tracks it hands to other modules";
		configLight(SELECTED_LIGHT, "Selected on the Sequencer");
		configLight(ACTIVE_LIGHT, "Notes playing");
		configInput(VOCT_INPUT, "1V/octave pitch, in place of the track's notes while GATE is patched");
		configInput(GATE_INPUT, "Gate, in place of the track's notes");
		configInput(VELOCITY_INPUT, "Velocity (0-10V)");
		configInput(CLOCK_INPUT, "Clock, for the track's synced LFO speeds");
		configInput(RESET_INPUT, "LFO reset");
		configInput(EXT_INPUT, "LFO external (±5V), for the External shape");
		configInput(RETURN_L_INPUT, "Return left, or mono");
		configInput(RETURN_R_INPUT, "Return right");
		configOutput(LEFT_OUTPUT, "Left, or mono while right is unpatched, in place of the mix");
		configOutput(RIGHT_OUTPUT, "Right");
		configOutput(FX1_OUTPUT, "FX 1 send, in place of FX 1");
		configOutput(FX2_OUTPUT, "FX 2 send, in place of FX 2");
		const char* const cvs[] = {"1V/octave pitch", "Gate", "Envelope (0-10V)", "LFO (±5V)", "CV 1 (0-10V)", "CV 2 (0-10V)",
			"Filter (0-10V)", "Resonance (0-10V)", "FX 1 send (0-10V)", "FX 2 send (0-10V)",
			"Trigger", "Sound (1/12V a sound, 0V for the first)", "Velocity (0-10V)",
			"Send left (±5V)", "Send right (±5V)", "Time (1V a second)"};
		for (int i = CV_VOCT_OUTPUT; i < OUTPUTS_LEN; i++)
			configOutput(i, cvs[i - CV_VOCT_OUTPUT]);
		lightDivider.setDivision(64);
	}

	/** The track it would take, given those breakouts to its left have taken: Auto the first free one of its kind, a
	fixed one its own if free; -1 for none. */
	int wants(uint32_t taken) {
		int s = (int) std::round(params[TRACK_PARAM].getValue());
		if (s > 0) {
			int t = firstTrack(kind) + s - 1;
			return taken >> t & 1 ? -1 : t;
		}
		for (int t = firstTrack(kind); t < firstTrack(kind) + tracks(kind); t++)
			if (!(taken >> t & 1))
				return t;
		return -1;
	}

	/** Finds its hub, the first Sequencer to its left across this plugin's modules, and its track, after the
	breakouts of its kind between them. */
	void findPlace() {
		std::vector<BreakoutModule*> between;
		Hub* found = nullptr;
		for (Module* m = leftExpander.module; onBus(m) && !(found = dynamic_cast<Hub*>(m)); m = m->leftExpander.module)
			if (BreakoutModule* b = dynamic_cast<BreakoutModule*>(m); b && b->kind == kind)
				between.push_back(b);
		uint32_t taken = 0;
		for (auto b = between.rbegin(); b != between.rend(); ++b)
			if (int t = (*b)->wants(taken); t >= 0)
				taken |= 1u << t;
		hub = found;
		track = found ? wants(taken) : -1;
	}

	void process(const ProcessArgs& args) override {
		float s = params[TRACK_PARAM].getValue();
		if (s != selected) {
			selected = s;
			busMoved();
		}
		uint32_t layout = busLayout.load(std::memory_order_relaxed);
		if (layout != layoutSeen) {
			layoutSeen = layout;
			findPlace();
		}
		int t = track;
		const JacksOut* from = nullptr;
		if (hub && t >= 0) {
			JacksIn& to = hub->jacksIn(t, args.frame);
			to.frame = args.frame;
			to.patched = 0;
			for (int o = 0; o < OUTPUTS_LEN; o++)
				to.patched |= uint32_t(outputs[o].isConnected()) << o;
			for (int i = 0; i < INPUTS_LEN; i++) {
				int n = to.inputs[i].channels = inputs[i].getChannels();
				if (n)
					std::copy(std::begin(inputs[i].voltages), std::end(inputs[i].voltages), to.inputs[i].voltages);
			}
			from = &hub->lastJacksOut(t, args.frame);
		}
		for (int o = 0; o < OUTPUTS_LEN; o++) {
			if (!outputs[o].isConnected())
				continue;
			int n = from ? (int) from->outputs[o].channels : 0;
			outputs[o].setChannels(std::max(n, 1));
			for (int c = 0; c < std::max(n, 1); c++)
				outputs[o].setVoltage(n ? from->outputs[o].voltages[c] : 0.f, c);
		}
		if (lightDivider.process())
			updateLights(args.sampleTime * lightDivider.getDivision(), from);
	}

	void updateLights(float dt, const JacksOut* from) {
		lights[SELECTED_LIGHT].setBrightness(from && from->selected);
		lights[ACTIVE_LIGHT].setBrightness(from ? from->activity : 0.f);
		for (int i = 0; i < INPUTS_LEN; i++) {
			float level = TrackCore::peak(inputs[i]);
			inputMeters[i].process(level / 10.f, level > 10.f, dt);
			lights[INPUT_LIGHT + 2 * i].setBrightness(inputMeters[i].peak);
			lights[INPUT_LIGHT + 2 * i + 1].setBrightness(inputMeters[i].clip > 0.f);
		}
		for (int o = 0; o < OUTPUTS_LEN; o++) {
			// The audio outputs run about ±5V, into the track's soft limit; the CVs 10V.
			float level = TrackCore::peak(outputs[o]), full = o < AUDIO_OUTPUTS ? 5.f : 10.f;
			outputMeters[o].process(level / full, o < AUDIO_OUTPUTS && level > 5.f, dt);
			lights[OUTPUT_LIGHT + 2 * o].setBrightness(outputMeters[o].peak);
			lights[OUTPUT_LIGHT + 2 * o + 1].setBrightness(outputMeters[o].clip > 0.f);
		}
	}

	void onExpanderChange(const ExpanderChangeEvent& e) override {
		busMoved();
	}

	/** Before 2.1.0 a breakout's first param was a sound dial, now the selector: an older patch's goes back to Auto. */
	void fromJson(json_t* root) override {
		Module::fromJson(root);
		const char* version = json_string_value(json_object_get(root, "version"));
		if (!version || string::Version(version) < string::Version("2.1.0"))
			params[TRACK_PARAM].setValue(0.f);
	}
};


// By lane: the master track has no engine, so no status screen shows its symbol.
inline const char* const TRACK_SYMBOLS[LANES] = {"kick", "snare", "perc", "sample", "bass", "lead", "arp", "chord", "master", "fx1", "fx2"};
inline const char* const TRACK_LABELS[LANES] = {"KICK", "SNARE", "PERC", "SAMPLE", "BASS", "LEAD", "ARP", "CHORD", "MASTER", "FX 1", "FX 2"};

/** A breakout's status screen: the track it has, as a big digit and that track's icon, then a scope of the track's
output, a dot per voice and a level bar. With no track it shows a dash. */
struct TrackStatus : widget::Widget {
	BreakoutModule* module = nullptr;
	TrackCore* core = nullptr;    // the core playing its track, found again each frame
	std::shared_ptr<window::Svg> symbols[LANES];

	TrackStatus() {
		for (int t = 0; t < LANES; t++)
			symbols[t] = Svg::load(asset::plugin(pluginInstance, string::f("res/components/symbols/%s.svg", TRACK_SYMBOLS[t])));
	}

	void step() override {
		Hub* hub = module ? findHub(module) : nullptr;
		int track = module ? module->track.load() : -1;
		core = hub && track >= 0 ? hub->trackCore(track) : nullptr;
		// The UI thread is the buffer's one consumer, shared with a Screen showing the track.
		if (core)
			core->snapshots.update();
		widget::Widget::step();
	}

	/** The track's number, as wide as its digits: two reach left and up to the icon, to keep them apart. */
	static math::Rect digits(int track) {
		bool two = track >= 0 && trackNumber(track) > 9;
		return math::Rect(mm2px(math::Vec(two ? 1.2f : 2.f, 2.2f)), mm2px(math::Vec(two ? 13.6f : 6.f, 11.f)));
	}

	void draw(const DrawArgs& args) override {
		int track = core ? core->busTrack : -1;
		SevenSegment::draw(args.vg, digits(track), track >= 0 && trackNumber(track) > 9 ? "88" : "8", false, nvgTransRGBAf(NOTE_COLOR, 0.08f));
		if (track < 0)
			return;
		NVGcolor ink = nvgRGB(0xc9, 0xc8, 0xc4);
		nvgFontFaceId(args.vg, APP->window->uiFont->handle);
		nvgFontSize(args.vg, mm2px(2.2f));
		nvgTextAlign(args.vg, NVG_ALIGN_RIGHT | NVG_ALIGN_BASELINE);
		nvgFillColor(args.vg, ink);
		nvgText(args.vg, box.size.x - mm2px(1.6f), mm2px(13.2f), TRACK_LABELS[track], nullptr);
	}

	void drawLayer(const DrawArgs& args, int layer) override {
		if (layer != 1)
			return Widget::drawLayer(args, layer);
		int track = core ? core->busTrack : -1;
		SevenSegment::draw(args.vg, digits(track), track >= 0 ? std::to_string(trackNumber(track)) : "-", true, NOTE_COLOR);
		if (!core)
			return Widget::drawLayer(args, layer);
		const TrackCore::Snapshot& snapshot = core->snapshots.read();

		// The track's icon, lit white while the Sequencer has it selected.
		if (symbols[track]->handle) {
			math::Vec size = symbols[track]->getSize();
			float side = mm2px(11.f), scale = side / size.x;
			nvgSave(args.vg);
			nvgTranslate(args.vg, box.size.x - mm2px(5.4f) - side / 2, mm2px(6.6f) - side / 2);
			nvgScale(args.vg, scale, scale);
			nvgTint(args.vg, core->selected ? nvgRGB(0xff, 0xff, 0xff) : nvgRGB(0x8a, 0x89, 0x86));
			window::svgDraw(args.vg, symbols[track]->handle);
			nvgRestore(args.vg);
		}

		// The scope: the output's last 1024 samples, ±5V full height.
		float x0 = mm2px(1.6f), w = box.size.x - 2 * x0, mid = mm2px(19.6f), h = mm2px(4.f);
		nvgBeginPath(args.vg);
		for (int i = 0; i < TrackCore::Snapshot::SCOPE; i += 4) {
			float x = x0 + w * i / (TrackCore::Snapshot::SCOPE - 1), y = mid - h * clamp(snapshot.scope[i] / 5.f, -1.f, 1.f);
			i ? nvgLineTo(args.vg, x, y) : nvgMoveTo(args.vg, x, y);
		}
		nvgStrokeColor(args.vg, nvgRGB(0x3b, 0xe0, 0x7a));
		nvgStrokeWidth(args.vg, 1.f);
		nvgStroke(args.vg);

		// A dot per voice, as bright as its envelope.
		float pitch = w / PORT_MAX_CHANNELS;
		for (int i = 0; i < PORT_MAX_CHANNELS; i++) {
			nvgBeginPath(args.vg);
			nvgCircle(args.vg, x0 + pitch * (i + 0.5f), mm2px(25.6f), mm2px(0.5f));
			nvgFillColor(args.vg, snapshot.levels[i] > 0.f ? nvgTransRGBAf(NOTE_COLOR, 0.2f + 0.8f * snapshot.levels[i]) : nvgRGB(0x2c, 0x2d, 0x30));
			nvgFill(args.vg);
		}

		// The level, in twelve segments: green, then yellow, then red into the soft limit.
		float level = std::fmax(snapshot.outputs[TrackIds::LEFT_OUTPUT], snapshot.outputs[TrackIds::RIGHT_OUTPUT]);
		float segment = w / 12;
		for (int i = 0; i < 12; i++) {
			bool lit = level > (i + 0.5f) / 12;
			NVGcolor c = i < 8 ? nvgRGB(0x3b, 0xe0, 0x7a) : i < 11 ? nvgRGB(0xff, 0xc8, 0x2e) : nvgRGB(0xff, 0x4d, 0x3d);
			nvgBeginPath(args.vg);
			nvgRect(args.vg, x0 + segment * i + 0.5f, mm2px(27.9f), segment - 1.f, mm2px(1.2f));
			nvgFillColor(args.vg, lit ? c : nvgTransRGBAf(c, 0.12f));
			nvgFill(args.vg);
		}
		Widget::drawLayer(args, layer);
	}
};


/** A breakout's panel: the status screen, SEL and NOTE, and the jacks. The track's dials are the Sequencer's. */
struct BreakoutWidget : ModuleWidget {
	BreakoutWidget(BreakoutModule* module, const std::string& slug) {
		setModule(module);
		std::string panel = asset::plugin(pluginInstance, string::f("res/%s.svg", slug.c_str()));
		setPanel(createPanel(panel, asset::plugin(pluginInstance, string::f("res/%s-dark.svg", slug.c_str()))));
		addScrews(this);

		PanelLayout layout(panel);
		TrackStatus* status = createWidget<TrackStatus>(math::Vec());
		status->box = layout.box("STATUS_WIDGET");
		status->module = module;
		addChild(status);
		addChild(createLightCentered<SmallLight<WhiteLight>>(layout.center("SELECTED_LIGHT"), module, TrackIds::SELECTED_LIGHT));
		addChild(createLightCentered<SmallLight<NoteLight>>(layout.center("ACTIVE_LIGHT"), module, TrackIds::ACTIVE_LIGHT));

		// Only the jacks the panel has.
		static const char* const inputs[] = {"VOCT", "GATE", "VELOCITY", "CLOCK", "RESET", "EXT", "RETURN_L", "RETURN_R"};
		static const char* const outputs[] = {"LEFT", "RIGHT", "FX1", "FX2",
			"CV_VOCT", "CV_GATE", "CV_ENV", "CV_LFO", "CV_P1", "CV_P2", "CV_FILTER", "CV_RESONANCE", "CV_FX1", "CV_FX2",
			"CV_TRIGGER", "CV_SOUND", "CV_VELOCITY", "CV_SEND_L", "CV_SEND_R", "CV_TIME"};
		for (int i = 0; i < TrackIds::INPUTS_LEN; i++) {
			std::string name = inputs[i];
			if (!layout.has(name + "_INPUT"))
				continue;
			addInput(createInputCentered<ThemedPJ301MPort>(layout.center(name + "_INPUT"), module, i));
			addChild(createLightCentered<DotLight<GreenRedLight>>(layout.center(name + "_LIGHT"), module, TrackIds::INPUT_LIGHT + 2 * i));
		}
		for (int i = 0; i < TrackIds::OUTPUTS_LEN; i++) {
			std::string name = outputs[i];
			if (!layout.has(name + "_OUTPUT"))
				continue;
			addOutput(createOutputCentered<ThemedPJ301MPort>(layout.center(name + "_OUTPUT"), module, i));
			addChild(createLightCentered<DotLight<GreenRedLight>>(layout.center(name + "_LIGHT"), module, TrackIds::OUTPUT_LIGHT + 2 * i));
		}
	}

	void appendContextMenu(Menu* menu) override {
		BreakoutModule* m = getModule<BreakoutModule>();
		SwitchQuantity* q = static_cast<SwitchQuantity*>(m->getParamQuantity(BreakoutModule::TRACK_PARAM));
		menu->addChild(new MenuSeparator);
		menu->addChild(createIndexSubmenuItem("Track", q->labels,
			[=]() { return (size_t) std::round(q->getValue()); },
			[=](size_t i) { q->setValue(i); }));
	}
};

} // namespace opz
