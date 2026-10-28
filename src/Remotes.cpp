/*
Remotes: plays the Sequencer's patterns from jacks. It sits on the Sequencer's left, finds its hub by walking right
across this plugin's modules, and hands it its inputs each frame (Hub.hpp's RemoteIn); the Sequencer makes the moves
(Sequencer::handleRemotes) and sends back counts of the events the outputs pulse for.

SELECT's channel n plays pattern n, as PROJECT and step n do; NEXT and PREV step along the chain, or with one pattern
playing, to the next or last pattern with notes; SONG picks a saved chain, played on LOAD. With BAR patched, moves wait
for the next BAR and start every track from step 1 on it. START pulses as a pattern starts, switches or loops, END as
its cycle ends and it loops, and SONG END as a chain comes round to its first pattern. PATTERN is high on the channel
of the pattern playing, for another Sequencer's SELECT to follow.
*/
#include "plugin.hpp"
#include "ui.hpp"
#include "opz/Hub.hpp"

struct Remotes : Module {
	enum ParamId { PARAMS_LEN };
	enum InputId { SELECT_INPUT, NEXT_INPUT, PREV_INPUT, BAR_INPUT, SONG_INPUT, LOAD_INPUT, INPUTS_LEN };
	enum OutputId { START_OUTPUT, END_OUTPUT, SONG_END_OUTPUT, PATTERN_OUTPUT, OUTPUTS_LEN };
	enum LightId { LINK_LIGHT, ENUMS(INPUT_LIGHT, INPUTS_LEN), ENUMS(OUTPUT_LIGHT, OUTPUTS_LEN), LIGHTS_LEN };
	static_assert((int) INPUTS_LEN == (int) opz::RemoteIn::JACKS, "the inputs are RemoteIn's jacks, in order");

	static constexpr float WINDOWS[] = {0.f, 0.001f, 0.002f, 0.005f, 0.01f, 0.02f}; // seconds
	static constexpr float FLASH = 0.1f;   // seconds an LED stays lit after a trigger

	opz::Hub* hub = nullptr;
	uint32_t layoutSeen = ~0u;
	float window = 0.005f;
	opz::RemoteOut seen;
	bool linked = false;
	dsp::PulseGenerator pulses[3];
	float flashes[INPUTS_LEN + OUTPUTS_LEN] = {};
	dsp::ClockDivider lightDivider;

	Remotes() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configInput(SELECT_INPUT, "Pattern select: a trigger on channel n plays pattern n");
		configInput(NEXT_INPUT, "Next pattern, or the chain's next");
		configInput(PREV_INPUT, "Previous pattern, or the chain's last");
		configInput(BAR_INPUT, "Bar: patched, moves wait for it and start every track from step 1");
		configInput(SONG_INPUT, "Song: 0-10 V picks saved chain 1 to 14");
		configInput(LOAD_INPUT, "Load the song SONG picks; unpatched, it loads as SONG moves");
		configOutput(START_OUTPUT, "Pattern start: playing from the top, a switch, or a loop");
		configOutput(END_OUTPUT, "Pattern end: its cycle ends and it loops");
		configOutput(SONG_END_OUTPUT, "Song end: the chain comes round to its first pattern");
		configOutput(PATTERN_OUTPUT, "Pattern: high on the channel of the pattern playing");
		lightDivider.setDivision(64);
	}

	void onExpanderChange(const ExpanderChangeEvent& e) override {
		opz::busMoved();
	}

	void findHub() {
		hub = nullptr;
		for (Module* m = rightExpander.module; opz::onBus(m) && !hub; m = m->rightExpander.module)
			hub = dynamic_cast<opz::Hub*>(m);
	}

	void process(const ProcessArgs& args) override {
		uint32_t layout = opz::busLayout.load(std::memory_order_relaxed);
		if (layout != layoutSeen) {
			layoutSeen = layout;
			findHub();
			linked = false;
		}
		int pattern = -1;
		if (hub) {
			opz::RemoteIn& in = hub->remoteInFor(args.frame);
			in.frame = args.frame;
			in.window = window;
			in.patched = 0;
			for (int j = 0; j < INPUTS_LEN; j++) {
				in.patched |= uint32_t(inputs[j].isConnected()) << j;
				in.volts[j] = inputs[j].getVoltage();
			}
			in.selectChannels = inputs[SELECT_INPUT].getChannels();
			std::copy(inputs[SELECT_INPUT].voltages, inputs[SELECT_INPUT].voltages + in.selectChannels, in.select);

			// The counts only move on: the first frame beside a Sequencer just takes them.
			const opz::RemoteOut& out = hub->lastRemoteOut(args.frame);
			if (linked) {
				uint32_t moved[3] = {out.starts - seen.starts, out.ends - seen.ends, out.songEnds - seen.songEnds};
				for (int k = 0; k < 3; k++)
					if (moved[k])
						pulses[k].trigger(1e-3f);
			}
			seen = out;
			linked = true;
			pattern = out.pattern;
		}
		for (int k = 0; k < 3; k++)
			outputs[START_OUTPUT + k].setVoltage(pulses[k].process(args.sampleTime) ? 10.f : 0.f);
		outputs[PATTERN_OUTPUT].setChannels(16);
		for (int c = 0; c < 16; c++)
			outputs[PATTERN_OUTPUT].setVoltage(c == pattern ? 10.f : 0.f, c);

		for (int j = 0; j < INPUTS_LEN; j++) {
			float v = j == SELECT_INPUT ? inputs[j].getVoltageSum() : inputs[j].getVoltage();
			if (v >= 1.f)
				flashes[j] = FLASH;
		}
		for (int k = 0; k < 3; k++)
			if (outputs[START_OUTPUT + k].getVoltage() > 0.f)
				flashes[INPUTS_LEN + k] = FLASH;
		if (lightDivider.process()) {
			float dt = args.sampleTime * lightDivider.getDivision();
			lights[LINK_LIGHT].setBrightness(hub ? 1.f : 0.f);
			for (int k = 0; k < INPUTS_LEN + OUTPUTS_LEN; k++) {
				lights[INPUT_LIGHT + k].setBrightness(flashes[k] > 0.f ? 1.f : 0.f);
				flashes[k] -= dt;
			}
			lights[OUTPUT_LIGHT + PATTERN_OUTPUT].setBrightness(pattern >= 0 ? 1.f : 0.f);
		}
	}

	json_t* dataToJson() override {
		json_t* root = json_object();
		json_object_set_new(root, "window", json_real(window));
		return root;
	}

	void dataFromJson(json_t* root) override {
		if (json_t* j = json_object_get(root, "window"))
			window = clamp((float) json_number_value(j), 0.f, 0.02f);
	}
};

struct RemotesWidget : ModuleWidget {
	RemotesWidget(Remotes* module) {
		setModule(module);
		std::string panel = asset::plugin(pluginInstance, "res/Remotes.svg");
		setPanel(createPanel(panel, asset::plugin(pluginInstance, "res/Remotes-dark.svg")));
		addScrews(this);

		PanelLayout layout(panel);
		addChild(createLightCentered<SmallLight<GreenLight>>(layout.center("LINK_LIGHT"), module, Remotes::LINK_LIGHT));
		static const char* const inputs[] = {"SELECT", "NEXT", "PREV", "BAR", "SONG", "LOAD"};
		static const char* const outputs[] = {"START", "END", "SONG_END", "PATTERN"};
		for (int i = 0; i < Remotes::INPUTS_LEN; i++) {
			std::string name = inputs[i];
			addInput(createInputCentered<ThemedPJ301MPort>(layout.center(name + "_INPUT"), module, i));
			addChild(createLightCentered<DotLight<GreenLight>>(layout.center(name + "_LIGHT"), module, Remotes::INPUT_LIGHT + i));
		}
		for (int i = 0; i < Remotes::OUTPUTS_LEN; i++) {
			std::string name = outputs[i];
			addOutput(createOutputCentered<ThemedPJ301MPort>(layout.center(name + "_OUTPUT"), module, i));
			addChild(createLightCentered<DotLight<GreenLight>>(layout.center(name + "_LIGHT"), module, Remotes::OUTPUT_LIGHT + i));
		}
	}

	void appendContextMenu(Menu* menu) override {
		Remotes* m = getModule<Remotes>();
		std::vector<std::string> labels = {"Off"};
		for (int i = 1; i < (int) std::size(Remotes::WINDOWS); i++)
			labels.push_back(string::f("%g ms", Remotes::WINDOWS[i] * 1000.f));
		menu->addChild(new MenuSeparator);
		menu->addChild(createIndexSubmenuItem("Late triggers count as on time within", labels,
			[=]() {
				auto at = std::find(std::begin(Remotes::WINDOWS), std::end(Remotes::WINDOWS), m->window);
				return (size_t) (at == std::end(Remotes::WINDOWS) ? 3 : at - std::begin(Remotes::WINDOWS));
			},
			[=](size_t i) { m->window = Remotes::WINDOWS[i]; }));
	}
};

Model* modelRemotes = createModel<Remotes, RemotesWidget>("Remotes");
