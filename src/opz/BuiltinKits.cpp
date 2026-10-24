/*
The drum tracks' built-in kits, synthesized, which each track plays until another kit is picked. The OP-Z's own,
sampled, are in res/kits (FactoryKits.cpp).
Every sound comes from one small drum voice: a tone that can fall in pitch, with a second partial and FM; noise and
a TR-808-style cluster of square waves through a filter; a click; and saturation over the lot.
*/
#include "Kit.hpp"
#include "Sequence.hpp"
#include "../dsp/Svf.hpp"
#include <array>

namespace opz {

namespace {

enum Band { LP, BP, HP };

/** One sound. Pitches are in Hz; decays are how long a part takes to die away (to -60 dB), in seconds. */
struct Hit {
	std::string name;
	float freq = 55.f;       // the tone
	float sweep = 0.f;       // octaves the tone starts above it (below, if negative), falling back to it
	float bend = 0.03f;      // how quickly it falls: a time constant
	float decay = 0.f;       // the tone's; 0 for no tone
	float partial = 0.f;     // a second partial's level, beside the tone's 1, at `ratio` times its pitch
	float ratio = 1.5f;
	float fm = 0.f;          // phase modulation of both, in radians, by a sine at `fmRatio` times the tone, fading with it
	float fmRatio = 1.4f;
	bool square = false;     // squarish partials rather than sines
	float noise = 0.f;       // white noise into the filter
	float metal = 0.f;       // and the six squares, at the 808 cymbal's ratios of `metalHz`
	float metalHz = 400.f;
	float noiseDecay = 0.f;
	int bursts = 1;          // a clap's quick repeats of the noise before its tail
	int band = HP;
	float cutoff = 5000.f, q = 0.7f;
	float click = 0.f;       // a millisecond of noise at the start
	float attack = 0.f;
	float drive = 0.f;       // saturation, 0 to 1
	float level = 1.f;       // where its peak lands, against the kit's loudest
};

constexpr float RATE = 44100.f, GAP = 0.009f;
constexpr float METAL[6] = {1.f, 1.483f, 1.8f, 2.546f, 2.63f, 3.897f};

std::vector<float> render(const Hit& h, uint32_t seed) {
	const float dt = 1.f / RATE, twoPi = 2.f * float(M_PI);
	bool noisy = h.noise > 0.f || h.metal > 0.f;
	float length = h.attack + std::max({h.decay, noisy ? h.noiseDecay + GAP * (h.bursts - 1) : 0.f, h.click > 0.f ? 0.005f : 0.f});
	int n = (int) (RATE * std::min(length, 2.5f));
	xyz::Svf filter;
	filter.tune(h.cutoff * dt, 1.f / h.q);
	double phase = 0.0, phase2 = 0.0, phaseFm = 0.0, metalPhase[6] = {};
	// Each part falls by 60 dB over its decay; the sweep by its time constant.
	float tone = 1.f, toneFall = std::exp(-6.9f * dt / std::max(h.decay, 1e-4f));
	float bend = 1.f, bendFall = std::exp(-dt / h.bend);
	float noiseFall = std::exp(-6.9f * dt / std::max(h.noiseDecay, 1e-4f));
	float tail = 1.f;
	uint32_t r = seed;
	std::vector<float> out(n);
	float peak = 0.f;
	for (int i = 0; i < n; i++) {
		float t = i * dt;
		r ^= r << 13;
		r ^= r >> 17;
		r ^= r << 5;
		float white = int32_t(r) * (1.f / 2147483648.f);

		float x = 0.f;
		if (h.decay > 0.f) {
			float f = h.sweep ? h.freq * std::exp2(h.sweep * bend) : h.freq;
			bend *= bendFall;
			phase += f * dt;
			phase2 += f * h.ratio * dt;
			phaseFm += f * h.fmRatio * dt;
			float mod = h.fm ? h.fm * tone * std::sin(twoPi * float(phaseFm - std::floor(phaseFm))) : 0.f;
			auto wave = [&](double p) {
				float s = std::sin(twoPi * float(p - std::floor(p)) + mod);
				return h.square ? std::tanh(6.f * s) : s;
			};
			x = (wave(phase) + (h.partial ? h.partial * wave(phase2) : 0.f)) * tone;
			tone *= toneFall;
		}

		if (noisy) {
			float metal = 0.f;
			for (int k = 0; k < 6; k++) {
				metalPhase[k] += h.metalHz * METAL[k] * dt;
				metal += metalPhase[k] - std::floor(metalPhase[k]) < 0.5 ? 1.f : -1.f;
			}
			xyz::Svf::Out o = filter.process(h.noise * white + h.metal * metal / 6.f);
			float band = h.band == LP ? o.lp : h.band == BP ? o.bp : o.hp;
			float last = GAP * (h.bursts - 1), env;
			if (t < last)
				env = std::exp(-std::fmod(t, GAP) / 0.002f);
			else {
				env = tail;
				tail *= noiseFall;
			}
			x += band * env;
		}
		if (h.click && t < 0.01f)
			x += h.click * white * std::exp(-t / 0.0008f);
		if (h.attack > 0.f)
			x *= std::min(t / h.attack, 1.f);
		if (h.drive > 0.f)
			x = std::tanh(x * (1.f + 9.f * h.drive));
		out[i] = x;
		peak = std::max(peak, std::fabs(x));
	}
	// Even out the peaks, and fade the last 10 ms so a cut tail doesn't click.
	int fade = std::min(n, (int) (0.01f * RATE));
	for (int i = 0; i < n; i++)
		out[i] *= (peak > 0.f ? 0.9f * h.level / peak : 0.f) * std::min(1.f, float(n - i) / fade);
	return out;
}

Hit kick(const char* name, float freq, float sweep, float bend, float decay, float click, float drive = 0.f) {
	Hit h;
	h.name = name;
	h.freq = freq;
	h.sweep = sweep;
	h.bend = bend;
	h.decay = decay;
	h.click = click;
	h.drive = drive;
	return h;
}

/** A noise-led sound: a snare's rattle, a clap, a hat or a swoosh. */
Hit noise(const char* name, float level, int band, float cutoff, float q, float decay) {
	Hit h;
	h.name = name;
	h.noise = level;
	h.band = band;
	h.cutoff = cutoff;
	h.q = q;
	h.noiseDecay = decay;
	return h;
}

/** A pitched sound with a tone and a partial. */
Hit tone(const char* name, float freq, float decay, float partial = 0.f, float ratio = 1.5f) {
	Hit h;
	h.name = name;
	h.freq = freq;
	h.decay = decay;
	h.partial = partial;
	h.ratio = ratio;
	return h;
}

/** A hat or cymbal: the metal cluster with some noise, high-passed. */
Hit metal(const char* name, float hz, float decay, float cutoff, float noise = 0.3f) {
	Hit h = ::opz::noise(name, noise, HP, cutoff, 0.9f, decay);
	h.metal = 1.f;
	h.metalHz = hz;
	h.level = 0.7f;
	return h;
}

Hit snare(const char* name, float freq, float decay, float noiseLevel, int band, float cutoff, float noiseDecay) {
	Hit h = tone(name, freq, decay, 0.5f, 1.78f);
	h.noise = noiseLevel;
	h.band = band;
	h.cutoff = cutoff;
	h.noiseDecay = noiseDecay;
	return h;
}

template <class F>
Hit with(Hit h, F change) {
	change(h);
	return h;
}

std::vector<Hit> kicks() {
	return {
		kick("808 long", 45.f, 2.f, 0.05f, 1.4f, 0.15f),
		kick("808 deep", 41.f, 1.5f, 0.06f, 1.8f, 0.1f),
		kick("808 short", 50.f, 2.f, 0.04f, 0.5f, 0.2f),
		kick("808 tight", 55.f, 2.5f, 0.03f, 0.35f, 0.3f, 0.2f),
		kick("808 boom", 38.f, 1.2f, 0.08f, 2.f, 0.1f),
		kick("808 hard", 48.f, 2.5f, 0.04f, 0.9f, 0.3f, 0.4f),
		with(kick("909", 55.f, 3.f, 0.012f, 0.5f, 0.6f, 0.15f), [](Hit& h) { h.noise = 0.15f; h.noiseDecay = 0.02f; h.cutoff = 3000.f; }),
		kick("909 short", 58.f, 3.2f, 0.01f, 0.3f, 0.7f, 0.15f),
		kick("909 long", 52.f, 3.f, 0.014f, 0.8f, 0.5f, 0.15f),
		kick("909 punch", 60.f, 3.5f, 0.008f, 0.4f, 0.8f, 0.3f),
		kick("909 low", 47.f, 2.8f, 0.015f, 0.6f, 0.5f, 0.1f),
		kick("909 dirty", 55.f, 3.f, 0.012f, 0.6f, 0.6f, 0.7f),
		kick("rumble", 43.f, 2.5f, 0.03f, 1.4f, 0.3f, 0.5f),
		kick("hardcore", 50.f, 4.f, 0.02f, 0.6f, 0.6f, 1.f),
		kick("distorted", 46.f, 3.f, 0.025f, 0.9f, 0.4f, 0.8f),
		kick("gabber", 60.f, 4.5f, 0.015f, 0.7f, 0.5f, 1.f),
		with(kick("acoustic", 62.f, 1.f, 0.02f, 0.4f, 0.5f), [](Hit& h) { h.noise = 0.3f; h.band = LP; h.cutoff = 1500.f; h.noiseDecay = 0.08f; }),
		kick("soft", 55.f, 1.f, 0.05f, 0.6f, 0.1f),
		with(kick("thud", 70.f, 0.8f, 0.02f, 0.25f, 0.4f), [](Hit& h) { h.noise = 0.2f; h.band = LP; h.cutoff = 800.f; h.noiseDecay = 0.06f; }),
		kick("felt", 60.f, 0.6f, 0.03f, 0.45f, 0.05f),
		kick("click", 90.f, 3.f, 0.005f, 0.12f, 0.9f),
		with(kick("blip", 120.f, 2.f, 0.01f, 0.1f, 0.5f), [](Hit& h) { h.square = true; }),
		kick("sub", 35.f, 0.5f, 0.1f, 1.8f, 0.f),
		kick("zap kick", 50.f, 6.f, 0.02f, 0.5f, 0.3f),
	};
}

std::vector<Hit> snares() {
	Hit clap = noise("808 clap", 1.f, BP, 1100.f, 1.5f, 0.3f);
	clap.bursts = 4;
	return {
		snare("808 snare", 185.f, 0.2f, 0.8f, HP, 1800.f, 0.3f),
		with(snare("909 snare", 190.f, 0.18f, 1.f, HP, 1000.f, 0.35f), [](Hit& h) { h.sweep = 0.3f; h.bend = 0.01f; h.click = 0.3f; }),
		snare("tight", 220.f, 0.12f, 1.f, HP, 3500.f, 0.15f),
		with(snare("fat", 160.f, 0.4f, 0.9f, LP, 6000.f, 0.5f), [](Hit& h) { h.sweep = 0.5f; h.bend = 0.02f; h.drive = 0.3f; }),
		with(snare("crack", 240.f, 0.1f, 1.f, BP, 5000.f, 0.2f), [](Hit& h) { h.q = 1.f; h.click = 0.7f; h.drive = 0.4f; }),
		with(snare("lo-fi", 200.f, 0.2f, 1.f, BP, 2500.f, 0.28f), [](Hit& h) { h.q = 2.f; h.drive = 0.8f; }),
		with(snare("deep", 140.f, 0.45f, 0.6f, LP, 3000.f, 0.55f), [](Hit& h) { h.partial = 0.3f; }),
		with(snare("noise snare", 250.f, 0.1f, 1.f, HP, 2000.f, 0.4f), [](Hit& h) { h.partial = 0.f; h.level = 0.9f; }),
		with(tone("rim", 455.f, 0.04f, 1.f, 3.66f), [](Hit& h) { h.click = 0.5f; }),
		tone("rim high", 800.f, 0.03f, 0.8f, 2.5f),
		with(tone("sidestick", 600.f, 0.05f, 0.6f, 2.2f), [](Hit& h) { h.noise = 0.4f; h.band = BP; h.cutoff = 3000.f; h.q = 2.f; h.noiseDecay = 0.05f; }),
		tone("wood rim", 1100.f, 0.06f, 0.4f, 1.5f),
		clap,
		with(clap, [](Hit& h) { h.name = "909 clap"; h.cutoff = 1500.f; h.q = 1.f; h.noiseDecay = 0.4f; }),
		with(clap, [](Hit& h) { h.name = "tight clap"; h.bursts = 3; h.cutoff = 1800.f; h.noiseDecay = 0.15f; }),
		with(clap, [](Hit& h) { h.name = "big clap"; h.bursts = 5; h.cutoff = 1000.f; h.q = 0.9f; h.noiseDecay = 0.7f; h.drive = 0.2f; }),
		with(noise("snap", 1.f, BP, 2500.f, 2.f, 0.06f), [](Hit& h) { h.freq = 1200.f; h.decay = 0.03f; h.click = 0.6f; }),
		with(noise("brush", 1.f, HP, 4000.f, 0.7f, 0.25f), [](Hit& h) { h.attack = 0.02f; h.level = 0.7f; }),
		with(noise("brush long", 1.f, HP, 3000.f, 0.7f, 0.6f), [](Hit& h) { h.attack = 0.04f; h.level = 0.7f; }),
		with(snare("slap", 300.f, 0.1f, 0.8f, BP, 3500.f, 0.12f), [](Hit& h) { h.partial = 0.f; }),
		snare("piccolo", 330.f, 0.12f, 1.f, HP, 5000.f, 0.2f),
		with(noise("swell", 1.f, HP, 2000.f, 0.7f, 0.1f), [](Hit& h) { h.attack = 0.15f; h.level = 0.8f; }),
		with(snare("electro", 180.f, 0.2f, 0.7f, HP, 3000.f, 0.25f), [](Hit& h) { h.sweep = 1.5f; h.bend = 0.015f; h.drive = 0.5f; }),
		with(noise("trash", 1.f, BP, 6000.f, 0.7f, 0.3f), [](Hit& h) { h.metal = 0.5f; h.metalHz = 600.f; h.drive = 0.6f; }),
	};
}

std::vector<Hit> percussion() {
	auto drum = [](const char* name, float freq, float decay) {
		return with(kick(name, freq, 0.7f, 0.04f, decay, 0.2f), [](Hit& h) {
			h.noise = 0.15f;
			h.band = LP;
			h.cutoff = 2000.f;
			h.noiseDecay = 0.06f;
		});
	};
	auto conga = [](const char* name, float freq, float decay) { return kick(name, freq, 0.3f, 0.01f, decay, 0.3f); };
	return {
		metal("closed hat", 400.f, 0.08f, 8000.f),
		metal("closed short", 400.f, 0.05f, 8000.f),
		metal("closed bright", 520.f, 0.07f, 10000.f),
		metal("closed dark", 330.f, 0.1f, 6000.f),
		with(metal("pedal hat", 400.f, 0.14f, 7000.f), [](Hit& h) { h.attack = 0.005f; }),
		metal("open hat", 400.f, 0.7f, 8000.f),
		metal("open short", 400.f, 0.35f, 8000.f),
		metal("open long", 380.f, 1.4f, 7500.f),
		metal("crash", 450.f, 2.5f, 5000.f, 0.6f),
		with(metal("ride", 350.f, 2.f, 6000.f, 0.2f), [](Hit& h) { h.band = BP; h.freq = 2900.f; h.decay = 1.2f; h.partial = 0.f; }),
		with(metal("china", 610.f, 2.f, 3000.f, 0.5f), [](Hit& h) { h.drive = 0.5f; }),
		drum("low tom", 95.f, 0.6f),
		drum("mid tom", 130.f, 0.55f),
		drum("high tom", 175.f, 0.5f),
		drum("floor tom", 75.f, 0.8f),
		conga("low conga", 200.f, 0.28f),
		conga("high conga", 300.f, 0.25f),
		conga("bongo", 420.f, 0.15f),
		with(tone("cowbell", 540.f, 0.3f, 1.f, 1.48f), [](Hit& h) { h.square = true; h.level = 0.6f; }),
		with(tone("clave", 2500.f, 0.05f), [](Hit& h) { h.level = 0.7f; }),
		with(tone("woodblock", 900.f, 0.08f, 0.3f, 2.7f), [](Hit& h) { h.sweep = 0.3f; h.bend = 0.005f; }),
		with(noise("shaker", 1.f, HP, 6000.f, 0.7f, 0.1f), [](Hit& h) { h.attack = 0.015f; h.level = 0.6f; }),
		with(metal("tambourine", 1300.f, 0.3f, 7000.f, 0.6f), [](Hit& h) { h.bursts = 2; h.level = 0.6f; }),
		with(tone("triangle", 1800.f, 1.5f, 0.5f, 2.76f), [](Hit& h) { h.level = 0.5f; }),
	};
}

std::vector<Hit> effects() {
	auto blip = [](const char* name, float freq) { return with(tone(name, freq, 0.15f), [](Hit& h) { h.square = true; h.level = 0.7f; }); };
	auto bell = [](const char* name, float freq, float decay, float fm, float ratio) {
		return with(tone(name, freq, decay), [=](Hit& h) { h.fm = fm; h.fmRatio = ratio; h.level = 0.8f; });
	};
	auto bass = [](const char* name, float freq) { return kick(name, freq, 0.3f, 0.03f, 1.6f, 0.1f, 0.2f); };
	return {
		kick("zap", 70.f, 5.f, 0.03f, 0.4f, 0.f),
		kick("zap short", 90.f, 6.f, 0.015f, 0.15f, 0.f),
		with(kick("laser", 300.f, 4.f, 0.06f, 0.6f, 0.f), [](Hit& h) { h.square = true; h.level = 0.7f; }),
		kick("drop", 40.f, 4.f, 0.25f, 1.5f, 0.f),
		blip("blip C", 523.25f),
		blip("blip E", 659.26f),
		blip("blip G", 783.99f),
		blip("blip high C", 1046.5f),
		bell("bell", 880.f, 1.6f, 3.f, 3.5f),
		bell("glass", 1320.f, 1.f, 1.5f, 7.f),
		bell("pluck", 440.f, 0.4f, 4.f, 2.f),
		bell("gong", 220.f, 2.5f, 5.f, 1.41f),
		bass("808 C", 65.41f),
		bass("808 D#", 77.78f),
		bass("808 G", 98.f),
		kick("sub drop", 55.f, 1.f, 0.3f, 1.8f, 0.f),
		noise("noise burst", 1.f, LP, 12000.f, 0.7f, 0.2f),
		with(noise("swoosh", 1.f, BP, 1500.f, 3.f, 0.3f), [](Hit& h) { h.attack = 0.3f; }),
		with(noise("static", 1.f, HP, 9000.f, 0.7f, 0.5f), [](Hit& h) { h.drive = 0.6f; h.level = 0.6f; }),
		with(noise("thunder", 1.f, LP, 300.f, 1.f, 1.5f), [](Hit& h) { h.attack = 0.02f; }),
		with(tone("glitch", 2100.f, 0.03f), [](Hit& h) { h.square = true; h.metal = 1.f; h.metalHz = 1700.f; h.band = BP; h.cutoff = 3000.f; h.q = 4.f; h.noiseDecay = 0.06f; h.level = 0.7f; }),
		with(tone("robot", 150.f, 0.5f), [](Hit& h) { h.square = true; h.fm = 6.f; h.fmRatio = 0.5f; h.level = 0.7f; }),
		with(tone("chirp", 2000.f, 0.2f), [](Hit& h) { h.sweep = -2.f; h.bend = 0.05f; h.level = 0.7f; }),
		with(tone("hit", 180.f, 0.4f), [](Hit& h) { h.metal = 0.5f; h.metalHz = 250.f; h.noise = 0.5f; h.band = BP; h.cutoff = 900.f; h.q = 2.f; h.noiseDecay = 0.4f; h.drive = 0.7f; }),
	};
}

Kit make(const char* name, const std::vector<Hit>& hits, uint32_t seed) {
	Kit kit;
	kit.name = name;
	kit.rate = RATE;
	for (const Hit& h : hits)
		kit.append(render(h, seed += 0x9e3779b9u), RATE, h.name);
	return kit;
}

} // namespace


const Kit& builtinKit(int track) {
	static const std::array<Kit, DRUM_TRACKS> kits = {
		make("Kick", kicks(), 1),
		make("Snare", snares(), 2),
		make("Perc", percussion(), 3),
		make("Sample", effects(), 4),
	};
	return kits[clamp(track, 0, DRUM_TRACKS - 1)];
}

} // namespace opz
