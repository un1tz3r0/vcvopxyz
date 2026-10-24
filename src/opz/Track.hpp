#pragma once
#include "TrackCore.hpp"
#include "../ui.hpp"

/*
An engine as a module of its own: it holds a track's core (TrackCore.hpp), gives it the module's params, jacks and
lights, and joins the bus for it, as the first to fourth engine of its kind to the right of a Sequencer.
*/
namespace opz {

/** What every engine module does, whatever its core plays. */
struct Track : Module, TrackIds {
	Hub* hub = nullptr;          // the Sequencer to its left, or null off the bus
	uint32_t layoutSeen = ~0u;   // the bus layout its hub and track were found in

	Track() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
	}

	virtual TrackCore& getCore() = 0;

	/** The lights and jacks, after the core's params. */
	void configPorts() {
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
			"Filter (0-10V)", "Resonance (0-10V)", "FX 1 send (0-10V)", "FX 2 send (0-10V)",
			"Trigger", "Sound (1/12V a sound, 0V for the first)", "Velocity (0-10V)",
			"Send left (±5V)", "Send right (±5V)", "Time (1V a second)"};
		for (int i = CV_VOCT_OUTPUT; i < OUTPUTS_LEN; i++)
			configOutput(i, cvs[i - CV_VOCT_OUTPUT]);
		configOutput(CV_P1_OUTPUT, getParamQuantity(P1_PARAM)->name + " (0-10V)");
		configOutput(CV_P2_OUTPUT, getParamQuantity(P2_PARAM)->name + " (0-10V)");
	}

	/** Finds its hub, the first Sequencer to its left across this plugin's modules, and its core's track, by the
	engines of its kind in between. */
	void findPlace(TrackCore& core) {
		Hub* found = nullptr;
		int taken = 0;
		for (Module* m = leftExpander.module; onBus(m) && !(found = dynamic_cast<Hub*>(m)); m = m->leftExpander.module)
			if (Track* t = dynamic_cast<Track*>(m))
				taken += t->getCore().kind() == core.kind();
		hub = found;
		core.busTrack = found && taken < core.tracks() ? core.firstTrack() + taken : -1;
	}

	/** Hands its core the bus for this frame, finding its place first if modules have moved since it last looked. */
	void joinBus(TrackCore& core, int64_t frame) {
		uint32_t layout = busLayout.load(std::memory_order_relaxed);
		if (layout != layoutSeen) {
			layoutSeen = layout;
			findPlace(core);
		}
		core.frame = frame;
		core.bus = hub ? &hub->lastDownstream(frame) : nullptr;
		core.slot = hub && core.busTrack >= 0 ? &hub->slot(core.busTrack, frame) : nullptr;
	}

	void onExpanderChange(const ExpanderChangeEvent& e) override {
		busMoved();
	}

	/** What plays its track, for the Sequencer's record of its row, which a version without engine modules picks the
	same sounds from: its slug, its dials as shares of their travel, and its settings. */
	virtual json_t* rowJson() {
		json_t* sound = json_array();
		for (int i = 0; i < PARAMS_LEN; i++)
			json_array_append_new(sound, json_real(paramQuantities[i]->getScaledValue()));
		return json_pack("{s:s, s:o, s:o}", "slug", model->slug.c_str(), "sound", sound, "data", dataToJson());
	}

	void onReset(const ResetEvent& e) override {
		Module::onReset(e);
		getCore().resetSettings();
	}

	/** A patch from before the arp has no arp dials. It played the arp track's notes as they were, so it still does. */
	void paramsFromJson(json_t* root) override {
		Module::paramsFromJson(root);
		size_t i;
		json_t* p;
		json_array_foreach(root, i, p) {
			json_t* id = json_object_get(p, "id");
			if ((id ? json_integer_value(id) : (json_int_t) i) == ARP_SPEED_PARAM)
				return;
		}
		params[ARP_SPEED_PARAM].setValue(0.f);
	}

	json_t* dataToJson() override {
		json_t* root = json_object();
		getCore().dataToJson(root);
		return root;
	}

	void dataFromJson(json_t* root) override {
		getCore().dataFromJson(root);
	}
};


/** An engine module playing a core of type `Core`, which has configure(m, base) for its params. */
template <class Core>
struct TrackModule : Track {
	Core core;

	TrackModule() {
		core.bind(this);
		core.configure(this, 0);
		configPorts();
	}

	TrackCore& getCore() override {
		return core;
	}

	void process(const ProcessArgs& args) override {
		joinBus(core, args.frame);
		core.process(args);
	}
};

} // namespace opz
