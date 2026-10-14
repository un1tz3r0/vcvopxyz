/*
Breakout: an OP-Z synth track whose voice is made of other modules. It sends out what the track would play with,
each note's pitch, gate and envelope, the LFO, both sound dials, the filter and the FX sends, and takes the sound
back in at RETURN. Whatever it sends out, it stops doing itself: with ENV patched the return isn't enveloped, with
FILTER or RESO patched it isn't filtered, with an FX amount patched that FX output carries the full signal for an
outside VCA, and with LFO patched the LFO moves nothing inside. The level and the pan always apply.

A polyphonic return is enveloped channel by channel, one voice each; a mono one by the loudest voice.
*/
#include "opz/SynthTrackWidget.hpp"

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


struct Breakout : opz::SynthTrack<BreakoutSound> {
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


Model* modelBreakout = createModel<Breakout, opz::SynthTrackWidget<BreakoutSound>>(BreakoutSound::SLUG);
