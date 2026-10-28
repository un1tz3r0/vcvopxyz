/*
Drum Breakout: hands one of the Sequencer's drum tracks to other modules, as Synth Breakout does a synth track. It sends
out what the track plays: each note's trigger, gate, sound (which of the 24 its key picks), velocity and envelope, the
LFO, PITCH and REVERSE, the filter and the FX sends. While RETURN is unpatched the kit plays, and the breakout's core
follows its notes for these; patched at RETURN, the track's sounds are made of other modules, played by the core, and
come back in there, through whatever it doesn't send out, as Breakout's do. Patched at L and R, the track's sound leaves
there instead of the mix.

Each of the notes a step plays keeps its own channel; mono puts them all on the first. The envelope follows the note
style: retrig and mono leave it open, at the sustain level, for the sound outside to end itself, while gate and loop
release it with the note.
*/
#include "opz/DrumTrack.hpp"
#include "opz/Breakout.hpp"

namespace opz {

struct DrumBreakoutCore : DrumCore {
	struct Voice {
		xyz::Adsr envelope;
		dsp::PulseGenerator trigger;
		float sound = 0.f;      // the SOUND output's voltage
		float velocity = 1.f;
		int channel = -1;       // the gate channel holding it, or -1 once its note is over
	};

	Voice voices[PORT_MAX_CHANNELS];

	const xyz::Adsr* envelope(int voice, float* velocity) const override {
		if (voice >= PORT_MAX_CHANNELS)
			return nullptr;
		*velocity = voices[voice].velocity;
		return &voices[voice].envelope;
	}

	void noteOn(int c) override {
		Voice& v = voices[style == MONO ? 0 : c];
		v.envelope = xyz::Adsr();
		v.envelope.gate(true);
		v.trigger.trigger(1e-3f);
		v.sound = soundOf(notePitch(c)) / 12.f;
		v.velocity = velocity(c);
		v.channel = c;
		noteStarted();
	}

	void release(Voice& v) {
		v.channel = -1;
		if (style == GATE || style == LOOP)
			v.envelope.gate(false);
	}

	void noteOff(int c) override {
		for (Voice& v : voices)
			if (v.channel == c)
				release(v);
	}

	/** On a change of note style, too: mono plays on the first channel alone, so the others stop. */
	void releaseAll() override {
		for (int i = 0; i < PORT_MAX_CHANNELS; i++) {
			release(voices[i]);
			if (style == MONO && i > 0)
				voices[i].envelope = xyz::Adsr();
		}
	}

	void process(const ProcessArgs& args) override {
		lfoInside = !outputs[CV_LFO_OUTPUT].isConnected();
		prepare(args);

		// As many channels as the notes coming in, and any voice still sounding beyond them.
		int notes = style == MONO ? 1 : std::max(channels, 1);
		float levels[PORT_MAX_CHANNELS], loudest = 0.f;
		for (int i = 0; i < PORT_MAX_CHANNELS; i++) {
			Voice& v = voices[i];
			levels[i] = v.envelope.active() ? v.envelope.process(controls.envelope) * v.velocity : 0.f;
			loudest = std::fmax(loudest, levels[i]);
			bool trigger = v.trigger.process(args.sampleTime);
			if (levels[i] > 0.f)
				notes = std::max(notes, i + 1);
			outputs[CV_TRIGGER_OUTPUT].setVoltage(trigger ? 10.f : 0.f, i);
			outputs[CV_GATE_OUTPUT].setVoltage(v.channel >= 0 ? 10.f : 0.f, i);
			outputs[CV_SOUND_OUTPUT].setVoltage(v.sound, i);
			outputs[CV_VELOCITY_OUTPUT].setVoltage(10.f * v.velocity, i);
			outputs[CV_ENV_OUTPUT].setVoltage(10.f * levels[i], i);
		}
		for (int o : {CV_TRIGGER_OUTPUT, CV_GATE_OUTPUT, CV_SOUND_OUTPUT, CV_VELOCITY_OUTPUT, CV_ENV_OUTPUT})
			outputs[o].setChannels(notes);
		outputs[CV_P1_OUTPUT].setVoltage(controls.p1 / 12.f + controls.pitch);
		outputs[CV_P2_OUTPUT].setVoltage(controls.p2 >= 0.5f ? 10.f : 0.f);
		emitDialCVs();
		emitReturn(levels, loudest, args.sampleTime);
	}
};


static PlugRegistration drumBreakoutPlug({"DrumBreakout", DRUM, 0, true, DrumCore::configSound, makeCore<DrumBreakoutCore>});


struct DrumBreakout : BreakoutModule {
	static constexpr const char* SLUG = "DrumBreakout";

	DrumBreakout() : BreakoutModule(DRUM) {
		configOutput(CV_P1_OUTPUT, "Pitch (1V/octave)");
		configOutput(CV_P2_OUTPUT, "Reverse (10V on)");
	}
};


struct DrumBreakoutWidget : BreakoutWidget {
	DrumBreakoutWidget(DrumBreakout* module) : BreakoutWidget(module, DrumBreakout::SLUG) {}
};

} // namespace opz


Model* modelDrumBreakout = createModel<opz::DrumBreakout, opz::DrumBreakoutWidget>(opz::DrumBreakout::SLUG);
