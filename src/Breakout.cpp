/*
Synth Breakout: hands one of the Sequencer's synth tracks to other modules (opz/Breakout.hpp). Its core inside the
Sequencer sends out what the track plays with: each note's pitch, gate and envelope, the LFO, both sound dials, the
filter and the FX sends. While RETURN is unpatched the track's plug plays, and the core follows its notes for these.
Patched at RETURN, the track's voice is made of other modules instead, played by the core, which takes the sound back in
there. Whatever it sends out, it then stops doing itself: with ENV patched the return isn't enveloped, with FILTER or
RESO patched it isn't filtered, with an FX amount patched that FX output carries the full signal for an outside VCA, and
with LFO patched the LFO moves nothing inside. The level and the pan always apply. Patched at L and R, the track's
sound, the plug's or the return's, leaves there instead of the mix.

A polyphonic return is enveloped channel by channel, one voice each; a mono one by the loudest voice.
*/
#include "opz/SynthTrack.hpp"
#include "opz/Breakout.hpp"

struct BreakoutSound {
	static constexpr const char* SLUG = "Breakout";

	static void configSound(Module* m, int p1, int p2) {
		m->configParam(p1, 0.f, 1.f, 0.5f, "CV 1", " V", 0.f, 10.f);
		m->configParam(p2, 0.f, 1.f, 0.5f, "CV 2", " V", 0.f, 10.f);
	}

	struct Voice {
		float process(float, float, float, float, float) {
			return 0.f;
		}
	};
};


struct BreakoutCore : opz::SynthCore<BreakoutSound> {
	void process(const ProcessArgs& args) override {
		lfoInside = !outputs[CV_LFO_OUTPUT].isConnected();
		prepare(args);

		int notes = style == opz::POLY ? channels : 1;
		float levels[PORT_MAX_CHANNELS], loudest = 0.f;
		for (int i = 0; i < PORT_MAX_CHANNELS; i++) {
			Voice& v = voices[i];
			if (style == opz::POLY && i < notes)
				v.pitch = notePitch(i);
			levels[i] = v.envelope.active() ? advance(v) : 0.f;
			loudest = std::fmax(loudest, levels[i]);
		}
		for (int i = 0; i < notes; i++) {
			bool gate = style == opz::POLY ? gates[i].isHigh() : !held.empty();
			outputs[CV_VOCT_OUTPUT].setVoltage(voices[i].pitch + controls.pitch, i);
			outputs[CV_GATE_OUTPUT].setVoltage(gate ? 10.f : 0.f, i);
			outputs[CV_ENV_OUTPUT].setVoltage(10.f * levels[i], i);
		}
		for (int o : {CV_VOCT_OUTPUT, CV_GATE_OUTPUT, CV_ENV_OUTPUT})
			outputs[o].setChannels(notes);
		outputs[CV_P1_OUTPUT].setVoltage(10.f * controls.p1);
		outputs[CV_P2_OUTPUT].setVoltage(10.f * controls.p2);
		emitDialCVs();
		emitReturn(levels, loudest, args.sampleTime);
	}
};

static opz::PlugRegistration synthBreakoutPlug({BreakoutSound::SLUG, opz::SYNTH, 0, true, BreakoutSound::configSound, opz::makeCore<BreakoutCore>});


struct Breakout : opz::BreakoutModule {
	Breakout() : BreakoutModule(opz::SYNTH) {}
};

struct BreakoutWidget : opz::BreakoutWidget {
	BreakoutWidget(Breakout* module) : opz::BreakoutWidget(module, BreakoutSound::SLUG) {}
};

Model* modelBreakout = createModel<Breakout, BreakoutWidget>(BreakoutSound::SLUG);
