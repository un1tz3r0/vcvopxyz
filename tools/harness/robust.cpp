// Robustness checks, built with ASan and UBSan. Each test prints what it found; ASan aborts on the first bad access,
// so each runs in its own process: ./robust <test>.
#include <common.hpp>
#include <context.hpp>
#include <engine/Engine.hpp>
#include "Digital.cpp"
#include "Organ.cpp"
#include "Drum.cpp"
#include "opz/Sequencer.cpp"
#include <random>
#include <thread>
#include <chrono>
Plugin* pluginInstance;
using namespace opz;

static Module::ProcessArgs args() {
	Module::ProcessArgs a;
	a.sampleRate = 48000.f;
	a.sampleTime = 1.f / 48000.f;
	a.frame = 0;
	return a;
}

static void flip(std::vector<Module*>& mods) {
	for (Module* m : mods)
		for (Module::Expander* e : {&m->leftExpander, &m->rightExpander})
			if (e->messageFlipRequested) {
				std::swap(e->producerMessage, e->consumerMessage);
				e->messageFlipRequested = false;
			}
}

static std::vector<uint8_t> readAll(const std::string& p) {
	return system::readFile(p);
}

static void writeAll(const std::string& p, const std::vector<uint8_t>& d) {
	system::writeFile(p, d);
}

/** Plays every slice of a kit through a Drum in loop style and returns whether everything stayed finite. */
static bool playKit(std::unique_ptr<Kit> kit, int style, int samples, float* worst) {
	Drum d;
	d.lightDivider.setDivision(1 << 30);
	d.params[Track::STYLE_PARAM].setValue(style);
	d.setKit(std::move(kit));
	d.inputs[Track::GATE_INPUT].channels = 1;
	d.inputs[Track::VOCT_INPUT].channels = 1;
	Module::ProcessArgs a = args();
	bool finite = true;
	*worst = 0.f;
	for (int k = 0; k < 24; k++) {
		for (int i = 0; i < samples; i++) {
			d.inputs[Track::VOCT_INPUT].setVoltage((k - 7) / 12.f);
			d.inputs[Track::GATE_INPUT].setVoltage(i < samples * 3 / 4 ? 10.f : 0.f);
			d.process(a);
			float v = d.outputs[Track::LEFT_OUTPUT].getVoltage();
			if (!std::isfinite(v))
				finite = false;
			else
				*worst = std::max(*worst, std::fabs(v));
		}
	}
	return finite;
}

int main(int argc, char** argv) {
	pluginInstance = new Plugin;
	contextSet(new Context);
	APP->engine = new engine::Engine;
	for (Model* m : {modelSequencer, modelDrum, modelDigital, modelOrgan})
		m->plugin = pluginInstance;
	std::string test = argc > 1 ? argv[1] : "";
	std::string dir = argc > 2 ? argv[2] : "/tmp";

	if (test == "oldparams") {
		// A Sequencer saved by 2.0.4 (02c1b0b): TEMPO, SWING, 8 LENGTH, 8 QUANTIZE, 8 STEP_COUNT, 8 STEP_LENGTH, then
		// TRACK REC PLAY STOP OCT- OCT+ SHIFT METRONOME (all 0, keys are up), 16 steps, 24 keys, 8 selects, 4 pages.
		json_t* params = json_array();
		auto add = [&](int id, float v) { json_array_append_new(params, json_pack("{s:i, s:f}", "id", id, "value", v)); };
		int id = 0;
		add(id++, 120.f);
		add(id++, 0.5f);
		for (int i = 0; i < 8; i++) add(id++, 4.f);
		for (int i = 0; i < 8; i++) add(id++, 0.f);
		for (int i = 0; i < 8; i++) add(id++, 16.f);
		for (int i = 0; i < 8; i++) add(id++, 0.f);
		while (id < 94) add(id++, 0.f);
		Sequencer* s = new Sequencer;
		s->model = modelSequencer;
		s->paramsFromJson(params);
		printf("after loading a 2.0.4 Sequencer: drum level %.2f, synth level %.2f, punch %.2f, master %.2f, metronome level %.2f\n",
			s->params[Sequencer::LEVEL_PARAM + DRUM_LEVEL].getValue(), s->params[Sequencer::LEVEL_PARAM + SYNTH_LEVEL].getValue(),
			s->params[Sequencer::LEVEL_PARAM + PUNCH].getValue(), s->params[Sequencer::LEVEL_PARAM + MASTER_LEVEL].getValue(),
			s->params[Sequencer::METRONOME_LEVEL_PARAM].getValue());
		// And what that sounds like: the bass track into a Digital, one second.
		auto* e = new SynthTrack<Digital>;
		e->model = modelDigital;
		e->lightDivider.setDivision(1 << 30);
		s->lightDivider.setDivision(1 << 30);
		s->rightExpander.module = e;
		e->leftExpander.module = s;
		for (int st = 0; st < 16; st++) {
			Note n;
			n.pitch = 48;
			s->lanes[4].pattern.steps[st].add(n);
		}
		s->play();
		std::vector<Module*> row = {s, e};
		Module::ProcessArgs a = args();
		double sum = 0;
		for (int i = 0; i < 48000; i++) {
			s->process(a);
			e->process(a);
			flip(row);
			float v = s->outputs[Sequencer::LEFT_OUTPUT].getVoltage();
			sum += v * v;
		}
		printf("Sequencer L rms over 1 s with a bass line playing: %.4f V\n", std::sqrt(sum / 48000));
	}
	else if (test == "component_label") {
		// A patch whose step has a sweep component of value 12 (key nibble 12): componentsFromJson accepts 0 to 63.
		json_t* root = json_loads("{\"tracks\":[{},{},{},{},{\"pattern\":[[[60,100,0,0]]],\"components\":[[[8,12]]]}]}", 0, nullptr);
		Sequencer s;
		s.dataFromJson(root);
		printf("loaded: step 1 of the bass track has sweep %d (value keys go to 9)\n", s.lanes[4].pattern.steps[0].values[SWEEP]);
		// Hovering the step builds its tooltip, StepQuantity::getLabel -> componentValue:
		printf("tooltip text: %s\n", componentValue(SWEEP, s.lanes[4].pattern.steps[0].values[SWEEP]).c_str());
	}
	else if (test == "component_play") {
		json_t* root = json_loads("{\"tracks\":[{},{},{},{},{\"pattern\":[[[60,100,0,0]]],\"components\":[[[8,13]]]}]}", 0, nullptr);
		Sequencer s;
		s.lightDivider.setDivision(1 << 30);
		s.dataFromJson(root);
		s.play();
		Module::ProcessArgs a = args();
		for (int i = 0; i < 48000; i++)
			s.process(a);
		printf("played a second; sweep on the bass lane now %f\n", s.lanes[4].player.sweep(SWEEP_FILTER));
	}
	else if (test == "organ_sine") {
		float x = -1e-9f;
		printf("x - floor(x) = %.9g, so the index is %d of a %d-entry table\n", x - std::floor(x), int((x - std::floor(x)) * Organ::TABLE), (int) Organ::SINE.size());
		printf("Organ::sine(%g) = %g\n", x, Organ::sine(x));
	}
	else if (test == "kit_rate") {
		// An AIFF whose COMM sample rate is an 80-bit infinity (exponent 0x7fff), e.g. a corrupt file: the reader takes it.
		std::vector<uint8_t> f = readAll(dir + "/op1kit.aif");
		f[12 + 8 + 8] = 0x7f;
		f[12 + 8 + 9] = 0xff;
		writeAll("/tmp/inf_rate.aif", f);
		std::string error;
		std::unique_ptr<Kit> kit = Kit::load("/tmp/inf_rate.aif", &error);
		printf("loaded: %s, rate %g\n", kit ? "yes" : ("no: " + error).c_str(), kit ? kit->rate : 0.f);
		if (kit) {
			float worst;
			bool finite = playKit(std::move(kit), Drum::LOOP, 4800, &worst);
			printf("loop style: output %s\n", finite ? "finite" : "NaN/inf (and the track's filter stays NaN)");
		}
	}
	else if (test == "kit_nan") {
		// A float WAV with one NaN sample in its first slice, played once in retrig style, then 2 s of silence.
		Kit k;
		std::vector<float> tone(4410);
		for (size_t i = 0; i < tone.size(); i++) tone[i] = 0.5f * std::sin(i * 0.05f);
		tone[100] = NAN;
		k.append(tone, 44100.f, "t");
		writeAll("/tmp/nan.wav", k.toWav());
		std::string error;
		std::unique_ptr<Kit> kit = Kit::load("/tmp/nan.wav", &error);
		printf("load: %s, %zu samples, sample 100 = %f, slice 0 = %d..%d\n", kit ? "ok" : error.c_str(), kit ? kit->samples.size() : 0, kit ? kit->samples[100] : 0.f, kit ? kit->slices[0].start : 0, kit ? kit->slices[0].end : 0);
		Drum d;
		d.lightDivider.setDivision(1 << 30);
		d.setKit(std::move(kit));
		d.inputs[Track::GATE_INPUT].channels = 1;
		d.inputs[Track::VOCT_INPUT].channels = 1;
		d.inputs[Track::VOCT_INPUT].setVoltage(-7 / 12.f);
		Module::ProcessArgs a = args();
		int bad = 0;
		for (int i = 0; i < 48000 * 3; i++) {
			d.inputs[Track::GATE_INPUT].setVoltage(i > 0 && i < 2400 ? 10.f : 0.f);
			d.process(a);
			bad += !std::isfinite(d.outputs[Track::LEFT_OUTPUT].getVoltage());
		}
		printf("one NaN sample in a 0.1 s sound: %d of %d output samples over 3 s are NaN; last one %f\n", bad, 48000 * 3, d.outputs[Track::LEFT_OUTPUT].getVoltage());
	}
	else if (test == "kit_fuzz") {
		// Random byte and chunk-size mutations of a real OP-1 kit and of a WAV, each loaded and played.
		std::vector<uint8_t> aif = readAll(dir + "/op1kit.aif");
		Kit k;
		std::vector<float> tone(4410);
		for (size_t i = 0; i < tone.size(); i++) tone[i] = std::sin(i * 0.05f);
		k.append(tone, 44100.f, "t");
		std::vector<uint8_t> wav = k.toWav();
		std::mt19937 rng(1);
		int loaded = 0, nonfinite = 0, n = argc > 3 ? atoi(argv[3]) : 3000;
		for (int iter = 0; iter < n; iter++) {
			std::vector<uint8_t> f = iter % 2 ? aif : wav;
			int muts = 1 + rng() % 8;
			for (int m = 0; m < muts; m++) {
				size_t at = rng() % f.size();
				switch (rng() % 4) {
					case 0: f[at] = rng(); break;
					case 1: f[at] ^= 1 << (rng() % 8); break;
					case 2: if (at + 4 <= f.size()) { uint32_t v = rng(); memcpy(&f[at], &v, 4); } break;
					case 3: f.resize(std::max<size_t>(12, at)); break;
				}
			}
			std::string p = string::f("/tmp/fuzz%d%s", iter % 2, iter % 2 ? ".aif" : ".wav");
			writeAll(p, f);
			std::string error;
			std::unique_ptr<Kit> kit = Kit::load(p, &error);
			if (!kit)
				continue;
			loaded++;
			float worst;
			if (!playKit(std::move(kit), iter % 4 < 2 ? Drum::LOOP : Drum::RETRIG, 300, &worst))
				nonfinite++;
		}
		printf("%d mutated files, %d loaded, %d played NaN/inf\n", n, loaded, nonfinite);
	}
	else if (test == "opz_fuzz") {
		std::mt19937 rng(2);
		int ok = 0;
		for (int iter = 0; iter < 300; iter++) {
			std::vector<uint8_t> f(opzfile::SIZE);
			for (auto& b : f) b = rng();
			Project p = Sequencer::blankProject();
			std::string error;
			if (opzfile::read(f, p, &error)) {
				ok++;
				std::vector<uint8_t> out = opzfile::write(p);
				Sequencer s;
				s.projects[0] = p;
				s.recallProject(p);
			}
		}
		printf("300 random .opz files: %d read, written back and recalled without a fault\n", ok);
	}
	else if (test == "slots_export") {
		// A projects.json ("Load the slots from a file…") whose note length index is out of range, then an export.
		json_t* a = json_loads("[{\"tracks\":[{\"pattern\":[[[60,100,0,0]]],\"settings\":[16,0,40,0.0,1]}]}]", 0, nullptr);
		Sequencer s;
		s.projectsFromJson(a);
		printf("slot 1, track 1 note length index: %d (NOTE_LENGTHS has %d)\n", s.projects[0].parts[0].noteLength, (int) std::size(NOTE_LENGTHS));
		std::vector<uint8_t> f = opzfile::write(s.projects[0]);
		printf("exported %zu bytes\n", f.size());
	}
	else if (test == "kit_tiny_rate") {
		Kit k;
		std::vector<float> tone(4410, 0.1f);
		k.append(tone, 44100.f, "t");
		k.rate = 1.f;
		writeAll("/tmp/tiny.wav", k.toWav());
		std::string error;
		std::unique_ptr<Kit> kit = Kit::load("/tmp/tiny.wav", &error);
		printf("1 Hz WAV: %s\n", kit ? "loaded" : error.c_str());
	}
	else if (test == "kit_names") {
		Kit k;
		std::vector<float> tone(441, 0.1f);
		for (int i = 0; i < 24; i++)
			k.append(tone, 44100.f, i == 3 ? std::string("bad\xff\xfe") : string::f("s%d", i));
		json_t* j = k.toJson();
		printf("slice 3 name '%s', %zu slices in JSON\n", k.slices[3].name.c_str(), json_array_size(json_object_get(j, "slices")));
		json_decref(j);
	}
	else if (test == "kit_saved_speed") {
		Kit k;
		std::vector<float> tone(441, 0.1f);
		k.append(tone, 44100.f, "t");
		json_t* j = json_loads("{\"name\":\"x\",\"slices\":[[0,441,0.0,100.0,1,false,\"t\"]]}", 0, nullptr);
		auto kit = Kit::fromSaved(k.toWav(), j);
		printf("saved speed 0, gain 100 load as speed %g, gain %g\n", kit->slices[0].speed, kit->slices[0].gain);
	}
	else if (test == "svf_nan") {
		xyz::Svf f;
		f.tune(0.01f, 1.f);
		f.process(NAN);
		float y = 0.f;
		for (int i = 0; i < 100; i++)
			y = f.process(1.f).lp;
		printf("after a NaN in, then 100 samples of 1: lp = %f\n", y);
	}
	else if (test == "components_clamp") {
		json_t* root = json_loads("{\"tracks\":[{},{},{},{},{\"pattern\":[[[60,100,0,0]]],\"components\":[[[8,12],[13,63],[0,-5]]]}]}", 0, nullptr);
		Sequencer s;
		s.dataFromJson(root);
		const Step& st = s.lanes[4].pattern.steps[0];
		printf("sweep %d, trigger spark %d (key %d, mode %d), pulse %d; tooltip %s\n", st.values[SWEEP], st.values[TRIGGER_SPARK],
			st.key(TRIGGER_SPARK), st.values[TRIGGER_SPARK] >> 4, st.values[PULSE], componentValue(SWEEP, st.values[SWEEP]).c_str());
	}
	else if (test == "ppqn") {
		for (const char* t : {"{\"ppqn\":2147483647}", "{\"ppqn\":5}", "{\"ppqn\":24}"}) {
			Sequencer s;
			json_t* root = json_loads(t, 0, nullptr);
			s.dataFromJson(root);
			printf("%s -> ppqn %d\n", t, s.ppqn);
		}
	}
	else if (test == "switch_locked") {
		Sequencer s;
		s.lightDivider.setDivision(1 << 30);
		for (int st = 0; st < 4; st++)
			s.lanes[4].pattern.steps[st].add(Note());
		s.projects[2].used = true;
		Module::ProcessArgs a = args();
		std::atomic<bool> held{false}, release{false};
		std::thread ui([&]() {
			std::lock_guard<std::mutex> lock(s.projectsMutex);
			held = true;
			while (!release) std::this_thread::yield();
		});
		while (!held) std::this_thread::yield();
		s.keyed = 2;
		s.keyedSave = false;
		auto t0 = std::chrono::steady_clock::now();
		for (int i = 0; i < 4800; i++)
			s.process(a);
		double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
		printf("lock held by UI: 4800 samples in %.1f ms, project %d, keyed %d\n", ms, s.project, s.keyed);
		release = true;
		ui.join();
		for (int i = 0; i < 4800; i++)
			s.process(a);
		printf("lock free: project %d, keyed %d, recall %d, left slot %d, left track 5 has %d steps with notes, live %d\n", s.project, s.keyed, (int) s.recall,
			s.leftSlot, (int) std::count_if(std::begin(s.left.parts[4].pattern.steps), std::end(s.left.parts[4].pattern.steps), [](const Step& x) { return !x.empty(); }),
			(int) std::count_if(std::begin(s.lanes[4].pattern.steps), std::end(s.lanes[4].pattern.steps), [](const Step& x) { return !x.empty(); }));
		// Undo puts the left project back.
		{
			std::lock_guard<std::mutex> lock(s.projectsMutex);
			s.restored = s.left;
			s.restoring = s.leftSlot;
		}
		for (int i = 0; i < 4800; i++)
			s.process(a);
		printf("after undo: project %d, live track 5 has %d steps with notes, recall %d\n", s.project,
			(int) std::count_if(std::begin(s.lanes[4].pattern.steps), std::end(s.lanes[4].pattern.steps), [](const Step& x) { return !x.empty(); }), (int) s.recall);
		printf("sizeof(Project) = %zu\n", sizeof(Project));
	}
	return 0;
}
