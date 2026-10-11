#pragma once
#include "SynthTrack.hpp"

namespace opz {

/** Every synth track's panel has the same layout, so one widget serves them all.

Each dial position holds a dial for every page, and only the page showing is visible. So each parameter keeps a
dial of its own, with its tooltip, MIDI mapping and undo, while the panel looks like the hardware's four. */
template <class Engine>
struct SynthTrackWidget : ModuleWidget {
	using Track = SynthTrack<Engine>;

	app::ParamWidget* dials[PAGES_LEN][4] = {};
	IdleDial* idle[4] = {};

	SynthTrackWidget(Track* module) {
		setModule(module);
		std::string panel = asset::plugin(pluginInstance, string::f("res/%s.svg", Engine::SLUG));
		setPanel(createPanel(panel, asset::plugin(pluginInstance, string::f("res/%s-dark.svg", Engine::SLUG))));
		addScrews(this);

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

		for (int i = 0; i < PORT_MAX_CHANNELS; i++)
			addChild(createLightCentered<DotLight<NoteLight>>(layout.center(string::f("VOICE%d_LIGHT", i)), module, Track::VOICE_LIGHT + i));

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

	void step() override {
		Track* track = getModule<Track>();
		int shown = track ? track->shownPage() : PAGE_SOUND;
		for (int d = 0; d < 4; d++) {
			for (int p = 0; p < PAGES_LEN; p++)
				if (dials[p][d])
					dials[p][d]->setVisible(p == shown);
			idle[d]->setVisible(!dials[shown][d]);
		}
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
	}
};

} // namespace opz
