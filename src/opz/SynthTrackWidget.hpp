#pragma once
#include "SynthTrack.hpp"

namespace opz {

/** Every synth track's panel has the same layout, so one widget serves them all.

Each dial position holds a dial for every page, and only the page showing is visible. So each parameter keeps a
dial of its own, with its tooltip, MIDI mapping and undo, while the panel looks like the hardware's four. */
template <class Engine>
struct SynthTrackWidget : ModuleWidget {
	using Track = SynthTrack<Engine>;
	static constexpr double HOLD = 1.0; // seconds to hold a slot's key to save to it

	app::ParamWidget* dials[PAGES_LEN][4] = {};
	IdleDial* idle[4] = {};
	double pressed[SLOTS];
	bool saved[SLOTS] = {};
	bool wasTrackMode = false;

	SynthTrackWidget(Track* module) {
		setModule(module);
		std::string panel = asset::plugin(pluginInstance, string::f("res/%s.svg", Engine::SLUG));
		setPanel(createPanel(panel, asset::plugin(pluginInstance, string::f("res/%s-dark.svg", Engine::SLUG))));
		addScrews(this);
		std::fill(std::begin(pressed), std::end(pressed), -1.0);

		PanelLayout layout(panel);
#define AT(ID) layout.center(#ID), module, Track::ID
		addParam(createParamCentered<PageKey>(AT(PAGE_PARAM)));
		addChild(createLightCentered<SmallSimpleLight<PageLight>>(AT(PAGE_LIGHT)));
		for (int d = 0; d < 4; d++) {
			math::Vec pos = layout.center(string::f("DIAL%d_WIDGET", d));
			idle[d] = new IdleDial(d);
			idle[d]->box.pos = pos.minus(idle[d]->box.size.div(2));
			addChild(idle[d]);
			for (int p = 0; p < PAGES_LEN; p++) {
				if (Track::DIALS[p][d] >= 0) {
					dials[p][d] = createParamCentered<Dial>(pos, module, Track::DIALS[p][d])->color(d);
					addParam(dials[p][d]);
				}
			}
			addChild(createLightCentered<SmallSimpleLight<PageLight>>(layout.center(string::f("DIAL%d_LIGHT", d)), module, Track::DIAL_LIGHT + PAGES_LEN * d));
		}
		for (int i = 0; i < PAGES_LEN; i++) {
			math::Rect bar = layout.box(string::f("SELECT%d_LIGHT", i));
			PageBar* light = createLight<PageBar>(bar.pos, module, Track::SELECT_LIGHT + i);
			light->box.size = bar.size;
			light->setPage(i);
			addChild(light);
			math::Rect spot = layout.box(string::f("SELECT%d_PARAM", i));
			HotSpot* button = createParam<HotSpot>(spot.pos, module, Track::SELECT_PARAM + i);
			button->box.size = spot.size;
			addParam(button);
		}

		addParam(createLightParamCentered<OctaveDownKey>(AT(OCTAVE_DOWN_PARAM), Track::OCTAVE_DOWN_LIGHT));
		addParam(createLightParamCentered<OctaveUpKey>(AT(OCTAVE_UP_PARAM), Track::OCTAVE_UP_LIGHT));
		for (int i = 0; i < PORT_MAX_CHANNELS; i++)
			addChild(createLightCentered<DotLight<NoteLight>>(layout.center(string::f("VOICE%d_LIGHT", i)), module, Track::VOICE_LIGHT + i));
		for (int k = 0; k < KEYS; k++) {
			math::Vec pos = layout.center(string::f("KEY%d_PARAM", k));
			if (keySlot(k) >= 0)
				addParam(createLightParamCentered<WhiteKey>(pos, module, Track::KEY_PARAM + k, Track::KEY_LIGHT + k));
			else
				addParam(createLightParamCentered<BlackKey>(pos, module, Track::KEY_PARAM + k, Track::KEY_LIGHT + k));
		}

		static const char* const inputs[] = {"VOCT", "GATE", "VELOCITY", "CLOCK", "RESET", "EXT"};
		static const char* const outputs[] = {"LEFT", "RIGHT", "FX1", "FX2"};
		for (int i = 0; i < Track::INPUTS_LEN; i++) {
			addInput(createInputCentered<ThemedPJ301MPort>(layout.center(string::f("%s_INPUT", inputs[i])), module, i));
			addChild(createLightCentered<DotLight<GreenRedLight>>(layout.center(string::f("%s_LIGHT", inputs[i])), module, Track::INPUT_LIGHT + 2 * i));
		}
		for (int i = 0; i < Track::OUTPUTS_LEN; i++) {
			addOutput(createOutputCentered<ThemedPJ301MPort>(layout.center(string::f("%s_OUTPUT", outputs[i])), module, i));
			addChild(createLightCentered<DotLight<GreenRedLight>>(layout.center(string::f("%s_LIGHT", outputs[i])), module, Track::OUTPUT_LIGHT + 2 * i));
		}
#undef AT
	}

	std::string slotPath(int s) {
		return system::join(model->getUserPresetDirectory(), "Slots", string::f("Slot %02d.vcvm", s + 1));
	}

	void refreshSlots(Track* track) {
		int filled = 0;
		for (int s = 0; s < SLOTS; s++)
			if (system::isFile(slotPath(s)))
				filled |= 1 << s;
		track->slotsFilled = filled;
	}

	void saveSlot(Track* track, int s) {
		system::createDirectories(system::getDirectory(slotPath(s)));
		save(slotPath(s));
		track->showSlot(s, true);
		refreshSlots(track);
	}

	/** A slot holds a sound, so the page, the keyboard's octave and the clock setting stay as they are. */
	void loadSlot(Track* track, int s) {
		if (!system::isFile(slotPath(s)))
			return;
		int page = track->page, octave = track->octave, ppqn = track->ppqn;
		try {
			loadAction(slotPath(s));
		}
		catch (Exception& e) {
			WARN("Could not load %s: %s", slotPath(s).c_str(), e.what());
			return;
		}
		track->page = page;
		track->octave = octave;
		track->ppqn = ppqn;
		track->showSlot(s, false);
	}

	/** In track mode the white keys are quick slots, like the hardware's: a click loads one, and holding the key
	for a second saves to it. Slots are presets in a Slots folder, so every copy of the module shares them. */
	void handleSlots(Track* track) {
		if (track->trackMode && !wasTrackMode)
			refreshSlots(track);
		wasTrackMode = track->trackMode;
		double now = system::getTime();
		for (int s = 0; s < SLOTS; s++) {
			bool down = track->params[Track::KEY_PARAM + slotKey(s)].getValue() > 0.f;
			if (down && track->trackMode) {
				if (pressed[s] < 0.0) {
					pressed[s] = now;
					saved[s] = false;
				}
				else if (!saved[s] && now - pressed[s] >= HOLD) {
					saveSlot(track, s);
					saved[s] = true;
				}
			}
			else if (pressed[s] >= 0.0) {
				if (!saved[s] && track->trackMode)
					loadSlot(track, s);
				pressed[s] = -1.0;
			}
		}
	}

	void step() override {
		Track* track = getModule<Track>();
		int shown = track ? track->shownPage() : PAGE_SOUND;
		for (int d = 0; d < 4; d++) {
			for (int p = 0; p < PAGES_LEN; p++)
				if (dials[p][d])
					dials[p][d]->setVisible(p == shown);
			idle[d]->setVisible(!dials[shown][d]);
		}
		if (track)
			handleSlots(track);
		ModuleWidget::step();
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
		menu->addChild(createMenuItem("Open the quick slots folder", "", [=]() {
			std::string dir = system::getDirectory(slotPath(0));
			system::createDirectories(dir);
			system::openDirectory(dir);
		}));
	}
};

} // namespace opz
