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
	xyz::Svf lowpasses[2], highpasses[2];

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
		outputs[CV_LFO_OUTPUT].setVoltage(5.f * controls.lfo);
		const std::pair<int, float> cvs[] = {{CV_P1_OUTPUT, controls.p1}, {CV_P2_OUTPUT, controls.p2}, {CV_FILTER_OUTPUT, controls.filter},
			{CV_RESONANCE_OUTPUT, controls.resonance}, {CV_FX1_OUTPUT, controls.fx1}, {CV_FX2_OUTPUT, controls.fx2}};
		for (auto [output, value] : cvs)
			outputs[output].setVoltage(10.f * value);

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
				lowpasses[s].tuneLike(lowpass);
				highpasses[s].tuneLike(highpass);
				x = highpasses[s].process(lowpasses[s].process(x).lp).hp;
			}
			sides[s] = x * controls.gain;
		}
		emit(sides[0], sides[1], outputs[CV_FX1_OUTPUT].isConnected() ? 1.f : controls.fx1,
			outputs[CV_FX2_OUTPUT].isConnected() ? 1.f : controls.fx2, args.sampleTime);
	}
};


Model* modelBreakout = createModel<Breakout, opz::SynthTrackWidget<BreakoutSound>>(BreakoutSound::SLUG);
