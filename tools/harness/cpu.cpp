// CPU cost of every module: a Sequencer driving 4 Drums, Digital, Analog, Cluster, Organ and a Screen over the bus,
// 10 s of audio at 48 kHz, each module's process() timed with rdtsc. Then each engine alone at 1, 4 and 16 voices.
// Rack internals first, before rack.hpp marks them PRIVATE, so the harness can make an Engine for APP.
#include <common.hpp>
#include <context.hpp>
#include <engine/Engine.hpp>
#include "Digital.cpp"
#include "Analog.cpp"
#include "Cluster.cpp"
#include "Organ.cpp"
#include "Breakout.cpp"
#include "Drum.cpp"
#include "opz/Screen.cpp"
#include "opz/Sequencer.cpp"
#include <chrono>
#include <x86intrin.h>
Plugin* pluginInstance;
using namespace opz;

static const float SR = 48000.f;
static double tscPerUs = 0;

static void calibrate() {
	auto t0 = std::chrono::steady_clock::now();
	uint64_t c0 = __rdtsc();
	while (std::chrono::steady_clock::now() - t0 < std::chrono::milliseconds(300)) {}
	uint64_t c1 = __rdtsc();
	double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
	tscPerUs = (c1 - c0) / us;
}

static void flip(std::vector<Module*>& mods) {
	for (Module* m : mods)
		for (Module::Expander* e : {&m->leftExpander, &m->rightExpander})
			if (e->messageFlipRequested) {
				std::swap(e->producerMessage, e->consumerMessage);
				e->messageFlipRequested = false;
			}
}

static Note note(int pitch, int vel = 100, int len = 0) {
	Note n;
	n.pitch = pitch;
	n.velocity = vel;
	n.length = len;
	return n;
}

int main(int argc, char** argv) {
	pluginInstance = new Plugin;
	contextSet(new Context);
	APP->engine = new engine::Engine;
	for (Model* m : {modelSequencer, modelDrum, modelDigital, modelAnalog, modelCluster, modelOrgan, modelScreen, modelBreakout})
		m->plugin = pluginInstance;
	calibrate();
	bool lights = argc > 1 && std::string(argv[1]) == "nolights" ? false : true;

	// --- The row ---
	Sequencer* seq = new Sequencer;
	seq->model = modelSequencer;
	std::vector<Module*> row = {seq};
	std::vector<std::string> names = {"Sequencer"};
	for (int i = 0; i < 4; i++) {
		Drum* d = new Drum;
		d->model = modelDrum;
		row.push_back(d);
		names.push_back(string::f("Drum %d", i + 1));
	}
	auto add = [&](Module* m, Model* model, const char* name) {
		m->model = model;
		row.push_back(m);
		names.push_back(name);
	};
	add(new SynthTrack<Digital>, modelDigital, "Digital (bass, 1 ch)");
	add(new SynthTrack<Analog>, modelAnalog, "Analog (lead, 3 ch)");
	add(new SynthTrack<Cluster>, modelCluster, "Cluster (arp, 1 ch)");
	add(new SynthTrack<Organ>, modelOrgan, "Organ (chord, 4 ch)");
	add(new opz::Screen, modelScreen, "Screen (relay)");
	for (size_t i = 0; i < row.size(); i++) {
		row[i]->id = i + 1;
		if (i > 0)
			row[i]->leftExpander.module = row[i - 1];
		if (i + 1 < row.size())
			row[i]->rightExpander.module = row[i + 1];
	}
	for (Module* m : row) {
		if (auto* t = dynamic_cast<Track*>(m))
			if (!lights)
				t->lightDivider.setDivision(1 << 30);
	}
	if (!lights)
		seq->lightDivider.setDivision(1 << 30);

	// A busy pattern on all 8 tracks.
	for (int s = 0; s < 16; s++) {
		if (s % 4 == 0) seq->lanes[0].pattern.steps[s].add(note(53));
		if (s % 8 == 4) seq->lanes[1].pattern.steps[s].add(note(55));
		if (s % 2 == 1) seq->lanes[2].pattern.steps[s].add(note(58 + s % 3));
		if (s % 3 == 0) seq->lanes[3].pattern.steps[s].add(note(65 + s % 5));
		seq->lanes[4].pattern.steps[s].add(note(36 + (s * 5) % 12));
		if (s % 2 == 0)
			for (int k : {60, 64, 67})
				seq->lanes[5].pattern.steps[s].add(note(k + s % 4));
		seq->lanes[6].pattern.steps[s].add(note(72 + (s * 7) % 12));
		if (s % 4 == 0)
			for (int k : {48, 52, 55, 59})
				seq->lanes[7].pattern.steps[s].add(note(k, 100, 4 * TICKS));
	}
	// Some modulation so the control-rate paths run: an LFO on each synth track's filter.
	for (size_t i = 5; i < 9; i++) {
		row[i]->params[Track::LFO_AMOUNT_PARAM].setValue(0.4f);
		row[i]->params[Track::RELEASE_PARAM].setValue(0.4f);
	}
	seq->play();

	Module::ProcessArgs a;
	a.sampleRate = SR;
	a.sampleTime = 1.f / SR;
	std::vector<uint64_t> cycles(row.size(), 0);
	const int WARM = 48000, N = (argc > 2 ? atoi(argv[2]) : 10) * 48000;
	double peak = 0, sum = 0;
	for (int i = 0; i < WARM + N; i++) {
		a.frame = i;
		for (size_t k = 0; k < row.size(); k++) {
			uint64_t t0 = __rdtsc();
			row[k]->process(a);
			uint64_t t1 = __rdtsc();
			if (i >= WARM)
				cycles[k] += t1 - t0;
		}
		flip(row);
		float l = seq->outputs[Sequencer::LEFT_OUTPUT].getVoltage();
		if (i >= WARM) {
			peak = std::max(peak, (double) std::fabs(l));
			sum += l * l;
			if (!std::isfinite(l)) { printf("non-finite at %d\n", i); return 1; }
		}
	}
	// rdtsc's own overhead
	uint64_t o0 = __rdtsc();
	uint64_t ov = 0;
	for (int i = 0; i < 100000; i++) { uint64_t t0 = __rdtsc(); uint64_t t1 = __rdtsc(); ov += t1 - t0; }
	double overhead = ov / 100000.0;
	(void) o0;

	printf("Bus run, lights %s: 10 s at 48 kHz, L peak %.2f V, rms %.2f V\n", lights ? "on" : "off", peak, std::sqrt(sum / N));
	printf("%-24s %10s %10s\n", "module", "us/sample", "% of core");
	double total = 0;
	for (size_t k = 0; k < row.size(); k++) {
		double us = (cycles[k] / (double) N - overhead) / tscPerUs;
		total += us;
		printf("%-24s %10.3f %9.1f%%\n", names[k].c_str(), us, 100.0 * us * SR / 1e6);
	}
	printf("%-24s %10.3f %9.1f%%\n", "TOTAL", total, 100.0 * total * SR / 1e6);

	{
		// What one bus check costs: onBus is a dynamic_cast from Module* to BusNode*, four per module per sample.
		volatile int hits = 0;
		uint64_t t0 = __rdtsc();
		for (int i = 0; i < 1000000; i++)
			hits += BusNode::onBus(row[1 + i % 9]);
		uint64_t t1 = __rdtsc();
		printf("onBus: %.1f ns per call (%d hits)\n", (t1 - t0) / 1e6 / tscPerUs * 1000.0, (int) hits);
		t0 = __rdtsc();
		for (int i = 0; i < 1000000; i++)
			hits += row[1 + i % 9]->model->plugin == pluginInstance && row[1 + i % 9]->model != nullptr;
		t1 = __rdtsc();
		printf("plugin check alone: %.1f ns per call\n", (t1 - t0) / 1e6 / tscPerUs * 1000.0);
	}
	// --- Each engine alone, from its own GATE and V/OCT, with 1, 4 and 16 notes held ---
	auto solo = [&](Module* m, const char* name) {
		m->model = modelDigital;  // any model of this plugin; it's off the bus anyway
		if (auto* t = dynamic_cast<Track*>(m))
			if (!lights) t->lightDivider.setDivision(1 << 30);
		m->params[Track::LFO_AMOUNT_PARAM].setValue(0.4f);
		for (int voices : {1, 4, 16}) {
			m->inputs[Track::GATE_INPUT].channels = voices;
			m->inputs[Track::VOCT_INPUT].channels = voices;
			uint64_t c = 0;
			for (int i = 0; i < 48000 * 3; i++) {
				for (int v = 0; v < voices; v++) {
					// retrigger every quarter second so drums and envelopes keep going
					bool on = (i % 12000) < 9000;
					m->inputs[Track::GATE_INPUT].setVoltage(on ? 10.f : 0.f, v);
					m->inputs[Track::VOCT_INPUT].setVoltage(-1.f + v * 0.13f, v);
				}
				a.frame = i;
				uint64_t t0 = __rdtsc();
				m->process(a);
				uint64_t t1 = __rdtsc();
				if (i >= 48000) c += t1 - t0;
			}
			double us = (c / (48000.0 * 2) - overhead) / tscPerUs;
			printf("  %-10s %2d voices %8.3f us/sample %6.1f%%\n", name, voices, us, 100.0 * us * SR / 1e6);
		}
	};
	printf("\nEach engine alone, own inputs (gate 3/4 of every 1/4 s):\n");
	solo(new SynthTrack<Digital>, "Digital");
	solo(new SynthTrack<Analog>, "Analog");
	solo(new SynthTrack<Cluster>, "Cluster");
	solo(new SynthTrack<Organ>, "Organ");
	solo(new Drum, "Drum");
	{
		Breakout* b = new Breakout;
		b->inputs[Track::RETURN_L_INPUT].channels = 1;
		solo(b, "Breakout");
	}
	return 0;
}
