/*
Send FX Breakout: hands one of the Sequencer's FX tracks to other modules, as Synth Breakout does a synth track.
Patched at L and R, the effect's sound leaves there; patched at SEND instead, the effect is made of other modules,
played by the breakout's core inside the Sequencer. It sends out what the tracks send its FX track, at SEND, with the track's notes, the time they set (the
Delay's keys, in seconds), the LFO, both sound dials and the filter; and it takes the effect back in at RETURN,
through the filter unless FILTER or RESO is patched, at the track's level, into the mix.
*/
#include "opz/FxTrack.hpp"
#include "opz/Breakout.hpp"

struct FxBreakoutSound : opz::TempoTime {
	static constexpr const char* SLUG = "FxBreakout";

	static void configSound(Module* m, int p1, int p2) {
		m->configParam(p1, 0.f, 1.f, 0.5f, "CV 1", " V", 0.f, 10.f);
		m->configParam(p2, 0.f, 1.f, 0.5f, "CV 2", " V", 0.f, 10.f);
	}

	/** Unused: the effect is outside. */
	struct Unit {
		void prepare(float) {}
		void process(const float in[2], float out[2], const opz::FxControls&) {
			out[0] = in[0];
			out[1] = in[1];
		}
	};
};


struct FxBreakoutCore : opz::FxCore<FxBreakoutSound> {
	void process(const ProcessArgs& args) override {
		lfoInside = !outputs[CV_LFO_OUTPUT].isConnected();
		prepare(args);
		float offset = lfoInside ? controls.lfo : 0.f;
		int target = (int) value(LFO_TARGET_PARAM);

		float send = !silenced && busTrack >= 0 && bus ? bus->send[busTrack - opz::FX1] : 0.f;
		send = std::isfinite(send) ? send * (target == opz::TARGET_PITCH ? clamp(1.f + offset, 0.f, 2.f) : 1.f) : 0.f;
		outputs[CV_SEND_L_OUTPUT].setVoltage(send);
		outputs[CV_SEND_R_OUTPUT].setVoltage(send);
		float time = FxBreakoutSound::seconds(timeKey, secondsPerBeat) * (target == opz::TARGET_ATTACK ? std::exp2(offset) : 1.f);
		outputs[CV_TIME_OUTPUT].setVoltage(clamp(time, 0.f, 10.f));

		int notes = std::max(channels, 1);
		for (int c = 0; c < notes; c++) {
			outputs[CV_VOCT_OUTPUT].setVoltage(c < channels ? notePitch(c) : 0.f, c);
			outputs[CV_GATE_OUTPUT].setVoltage(c < channels && gates[c].isHigh() ? 10.f : 0.f, c);
		}
		outputs[CV_VOCT_OUTPUT].setChannels(notes);
		outputs[CV_GATE_OUTPUT].setChannels(notes);
		outputs[CV_P1_OUTPUT].setVoltage(10.f * controls.p1);
		outputs[CV_P2_OUTPUT].setVoltage(10.f * controls.p2);
		emitDialCVs();

		// The effect back, through the filter and at the level, as an FX module's is.
		bool filtered = !outputs[CV_FILTER_OUTPUT].isConnected() && !outputs[CV_RESONANCE_OUTPUT].isConnected();
		float gain = controls.gain / 5.f * float(M_SQRT2), out[2];
		for (int s = 0; s < 2; s++) {
			float x = inputs[s && inputs[RETURN_R_INPUT].isConnected() ? RETURN_R_INPUT : RETURN_L_INPUT].getVoltageSum();
			x = std::isfinite(x) ? x : 0.f;
			if (filtered) {
				returnLowpasses[s].tuneLike(lowpass);
				returnHighpasses[s].tuneLike(highpass);
				x = returnHighpasses[s].process(returnLowpasses[s].process(x).lp).hp;
			}
			out[s] = x * gain;
		}
		emit(out[0], out[1], 0.f, 0.f, args.sampleTime);
	}
};


static opz::PlugRegistration fxBreakoutPlug({FxBreakoutSound::SLUG, opz::FX, 0, true, FxBreakoutSound::configSound, opz::makeCore<FxBreakoutCore>});


struct FxBreakout : opz::BreakoutModule {
	FxBreakout() : BreakoutModule(opz::FX) {
		configInput(VOCT_INPUT, "1V/octave pitch: a note sets the time, from F3 up");
		configOutput(LEFT_OUTPUT, "Left, or mono while right is unpatched, in place of the mix");
	}
};

struct FxBreakoutWidget : opz::BreakoutWidget {
	FxBreakoutWidget(FxBreakout* module) : opz::BreakoutWidget(module, FxBreakoutSound::SLUG) {}
};

Model* modelFxBreakout = createModel<FxBreakout, FxBreakoutWidget>(FxBreakoutSound::SLUG);
