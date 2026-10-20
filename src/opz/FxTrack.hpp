#pragma once
#include "TrackWidget.hpp"
#include "../dsp/Delay.hpp"

/*
An OP-Z FX track (Track.hpp): a send effect playing FX 1 or FX 2, the OP-Z's tracks 9 and 10. It takes what the
engines send it over the bus, or with IN patched its inputs, and returns the effect alone, through the filter, to the
mix. Its dials have the OP-Z's two pages: the effect's two dials and the filter, then the LFO. Its notes, from the
keys while it's selected or from its track's steps, set the effect's time where it has one, the delay's or the
reverb's predelay: the lowest key is the shortest. The effect is a struct with:

	static constexpr const char* SLUG;
	static constexpr int TIME_KEY;                         // the key its time starts on, or -1 for an effect with none
	static void configSound(Module* m, int p1, int p2);    // configures its two dials
	static float seconds(int key, float secondsPerBeat);   // the time a key sets
	static std::string timeName(int key);
	struct Unit {
		void prepare(float sampleRate);                    // before the first sample, and when the rate changes
		// One sample of the send, about ±5V, to the effect alone.
		void process(const float in[2], float out[2], const FxControls& c);
	};
*/
namespace opz {

/** Delay's time keys, in sixty-fourths of a bar from 1/64 on the lowest key to 3/8 on the highest, starting on 3/16,
a dotted eighth. Send FX Breakout's TIME follows the same keys. */
struct TempoTime {
	static constexpr int TIME_KEY = 11;

	static float seconds(int key, float secondsPerBeat) {
		return (key + 1) / 16.f * secondsPerBeat;
	}

	/** The key's time as a share of a bar, in lowest terms. */
	static std::string timeName(int key) {
		int n = key + 1, d = 64;
		while (n % 2 == 0 && d > 1) {
			n /= 2;
			d /= 2;
		}
		return string::f("%d/%d", n, d);
	}
};

/** The FX track's dials after the LFO, and the clock, for an effect. */
struct FxControls {
	float p1 = 0.f, p2 = 0.f;
	float time = 0.f;          // seconds, where the keys set one
	float sampleRate = 48000.f, sampleTime = 1.f / 48000.f;
};

template <class Effect>
struct FxTrack : Track {
	typename Effect::Unit unit;
	FxControls fx;
	float rate = 0.f;
	const Downstream* bus = nullptr;

	FxTrack() {
		effect = true;
		timeKey = Effect::TIME_KEY;
		Effect::configSound(this, P1_PARAM, P2_PARAM);
		configTrack({"None"}, "FX tracks have no note style", {getParamQuantity(P1_PARAM)->name, getParamQuantity(P2_PARAM)->name,
			"Filter", "Resonance", "Time", "Send", "Pan", "Volume"});
		getParamQuantity(LFO_TARGET_PARAM)->description = "Time moves the delay or predelay up to an octave either way; send is the level into the effect";
		configDefault(LEVEL_PARAM, 1.f);
		configInput(RETURN_L_INPUT, "Left, or mono, in place of the bus's send");
		configInput(RETURN_R_INPUT, "Right");
		configInput(VOCT_INPUT, "1V/octave pitch: a note sets the time, from F3 up");
		configInput(GATE_INPUT, "Gate, in place of the bus's notes");
		configOutput(LEFT_OUTPUT, "Left, or mono while right is unpatched, in place of the bus's mix");
	}

	/** The sound page and the LFO page, and no others. */
	int dial(int page, int d) const override {
		return page == PAGE_SOUND || page == PAGE_LFO ? DIALS[page][d] : -1;
	}

	std::string soundName(int pitch) override {
		int key = pitch - FX_LOWEST;
		return Effect::TIME_KEY >= 0 && key >= 0 && key < FX_KEYS ? Effect::timeName(key) : "";
	}

	void noteOn(int channel) override {
		if (Effect::TIME_KEY >= 0)
			timeKey = clamp((int) std::round(notePitch(channel) * 12.f) + 60 - FX_LOWEST, 0, FX_KEYS - 1);
	}
	void noteOff(int channel) override {}
	void releaseAll() override {}
	const xyz::Adsr* envelope(int voice, float* velocity) const override {
		return nullptr;
	}

	float activity() override {
		return std::fmin(outputMeters[LEFT_OUTPUT].peak, 1.f);
	}

	void process(const ProcessArgs& args) override {
		if (args.sampleRate != rate) {
			rate = args.sampleRate;
			unit.prepare(rate);
		}
		bus = fromLeft(this);
		prepare(args);
		float offset = lfoInside ? controls.lfo : 0.f;
		int target = (int) value(LFO_TARGET_PARAM);
		fx.p1 = controls.p1;
		fx.p2 = controls.p2;
		fx.time = timeKey < 0 ? 0.f : Effect::seconds(timeKey, secondsPerBeat) * (target == TARGET_ATTACK ? std::exp2(offset) : 1.f);
		fx.sampleRate = args.sampleRate;
		fx.sampleTime = args.sampleTime;

		float in[2], out[2];
		float send = target == TARGET_PITCH ? clamp(1.f + offset, 0.f, 2.f) : 1.f;
		if (inputs[RETURN_L_INPUT].isConnected()) {
			in[0] = inputs[RETURN_L_INPUT].getVoltageSum();
			in[1] = inputs[RETURN_R_INPUT].isConnected() ? inputs[RETURN_R_INPUT].getVoltageSum() : in[0];
		}
		else
			in[0] = in[1] = busTrack >= 0 && bus ? bus->send[busTrack - FX1] : 0.f;
		for (float& x : in)
			x = std::isfinite(x) ? x * send : 0.f;
		unit.process(in, out, fx);

		// Back through the filter at the level, which a centred pan leaves whole.
		float gain = controls.gain / 5.f * float(M_SQRT2);
		for (int s = 0; s < 2; s++) {
			returnLowpasses[s].tuneLike(lowpass);
			returnHighpasses[s].tuneLike(highpass);
			out[s] = returnHighpasses[s].process(returnLowpasses[s].process(out[s]).lp).hp * gain;
		}
		emit(out[0], out[1], 0.f, 0.f, args.sampleTime);
	}

	void onReset(const ResetEvent& e) override {
		Track::onReset(e);
		timeKey = Effect::TIME_KEY;
	}

	json_t* dataToJson() override {
		json_t* root = Track::dataToJson();
		json_object_set_new(root, "timeKey", json_integer(timeKey));
		return root;
	}

	void dataFromJson(json_t* root) override {
		Track::dataFromJson(root);
		if (json_t* j = json_object_get(root, "timeKey"); j && Effect::TIME_KEY >= 0)
			timeKey = clamp((int) json_integer_value(j), 0, FX_KEYS - 1);
	}
};

template <class Effect>
struct FxTrackWidget : TrackWidget {
	FxTrackWidget(FxTrack<Effect>* module) : TrackWidget(module, Effect::SLUG) {}
};

} // namespace opz
