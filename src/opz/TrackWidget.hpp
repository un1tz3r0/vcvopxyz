#pragma once
#include "Track.hpp"

namespace opz {

// By lane: the master track has no engine, so no status screen shows its symbol.
inline const char* const TRACK_SYMBOLS[LANES] = {"kick", "snare", "perc", "sample", "bass", "lead", "arp", "chord", "master", "fx1", "fx2"};
inline const char* const TRACK_LABELS[LANES] = {"KICK", "SNARE", "PERC", "SAMPLE", "BASS", "LEAD", "ARP", "CHORD", "MASTER", "FX 1", "FX 2"};

/** A track's status screen: the Sequencer track it plays, as a big digit and that track's icon, then a scope of
its output, a dot per voice and a level bar. Off the bus it shows a dash and plays from its inputs. */
struct TrackStatus : widget::Widget {
	TrackCore* core = nullptr;
	std::shared_ptr<window::Svg> symbols[LANES];

	TrackStatus() {
		for (int t = 0; t < LANES; t++)
			symbols[t] = Svg::load(asset::plugin(pluginInstance, string::f("res/components/symbols/%s.svg", TRACK_SYMBOLS[t])));
	}

	void step() override {
		// The UI thread is the buffer's one consumer, shared with a Screen beside the track.
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
		if (!core)
			return Widget::drawLayer(args, layer);
		int track = core->busTrack;
		bool on = track >= 0;
		const TrackCore::Snapshot& snapshot = core->snapshots.read();
		SevenSegment::draw(args.vg, digits(track), on ? std::to_string(trackNumber(track)) : "-", true, NOTE_COLOR);

		// The track's icon, lit white while the Sequencer has it selected.
		if (on && symbols[track]->handle) {
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
		float level = std::fmax(snapshot.outputs[Track::LEFT_OUTPUT], snapshot.outputs[Track::RIGHT_OUTPUT]);
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


/** Every track's panel: the status screen, SEL and NOTE, and whichever jacks the panel has. The track's dials are
the Sequencer's, or a Screen's beside it. */
struct TrackWidget : ModuleWidget {
	TrackWidget(Track* module, const std::string& slug) {
		setModule(module);
		std::string panel = asset::plugin(pluginInstance, string::f("res/%s.svg", slug.c_str()));
		setPanel(createPanel(panel, asset::plugin(pluginInstance, string::f("res/%s-dark.svg", slug.c_str()))));
		addScrews(this);

		PanelLayout layout(panel);
		TrackStatus* status = createWidget<TrackStatus>(math::Vec());
		status->box = layout.box("STATUS_WIDGET");
		status->core = module ? &module->getCore() : nullptr;
		addChild(status);
		addChild(createLightCentered<SmallLight<WhiteLight>>(layout.center("SELECTED_LIGHT"), module, Track::SELECTED_LIGHT));
		addChild(createLightCentered<SmallLight<NoteLight>>(layout.center("ACTIVE_LIGHT"), module, Track::ACTIVE_LIGHT));

		// Only the jacks the panel has: the breakouts' returns and CV outputs are on their panels alone.
		static const char* const inputs[] = {"VOCT", "GATE", "VELOCITY", "CLOCK", "RESET", "EXT", "RETURN_L", "RETURN_R"};
		static const char* const outputs[] = {"LEFT", "RIGHT", "FX1", "FX2",
			"CV_VOCT", "CV_GATE", "CV_ENV", "CV_LFO", "CV_P1", "CV_P2", "CV_FILTER", "CV_RESONANCE", "CV_FX1", "CV_FX2",
			"CV_TRIGGER", "CV_SOUND", "CV_VELOCITY", "CV_SEND_L", "CV_SEND_R", "CV_TIME"};
		for (int i = 0; i < Track::INPUTS_LEN; i++) {
			std::string name = inputs[i];
			if (!layout.has(name + "_INPUT"))
				continue;
			addInput(createInputCentered<ThemedPJ301MPort>(layout.center(name + "_INPUT"), module, i));
			addChild(createLightCentered<DotLight<GreenRedLight>>(layout.center(name + "_LIGHT"), module, Track::INPUT_LIGHT + 2 * i));
		}
		for (int i = 0; i < Track::OUTPUTS_LEN; i++) {
			std::string name = outputs[i];
			if (!layout.has(name + "_OUTPUT"))
				continue;
			addOutput(createOutputCentered<ThemedPJ301MPort>(layout.center(name + "_OUTPUT"), module, i));
			addChild(createLightCentered<DotLight<GreenRedLight>>(layout.center(name + "_LIGHT"), module, Track::OUTPUT_LIGHT + 2 * i));
		}
	}

	void appendContextMenu(Menu* menu) override {
		TrackCore* core = &getModule<Track>()->getCore();
		std::vector<std::string> labels;
		for (int n : Track::PPQN)
			labels.push_back(string::f("%d PPQN", n));
		menu->addChild(new MenuSeparator);
		menu->addChild(createIndexSubmenuItem("Clock pulses per beat", labels,
			[=]() { return std::find(std::begin(Track::PPQN), std::end(Track::PPQN), core->ppqn) - std::begin(Track::PPQN); },
			[=](size_t i) { core->ppqn = Track::PPQN[i]; }));
	}
};

} // namespace opz
