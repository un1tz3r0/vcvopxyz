/*
Drum: an OP-Z drum track, kick, snare, perc or sample, as the first to fourth drum engine to the right of a Sequencer
takes. It plays a kit of 24 sounds, one per key from F3 to E5, the octaves either side playing them again. Each track
starts on a kit of its own, synthesized; the context menu picks one of the OP-Z's own kits, sampled, or loads an OP-1
or OP-Z kit file, any AIFF or WAV cut into 24, or a folder of sounds.

PITCH tunes every sound and REVERSE plays them backwards. The note styles are the OP-Z's drum ones: retrig plays each
sound to its end, so a sound's tail rings under the next, as many at once as there are notes (two at least, as on the
OP-Z); mono cuts the last sound off; gate stops each sound with its note; loop repeats it while its note is held,
then plays it out. A kit sound that plays gated or loops does so whatever the style.
*/
#include "opz/DrumTrack.hpp"
#include "opz/TrackWidget.hpp"
#include "opz/Kit.hpp"
#include <functional>
#include <mutex>
#include <osdialog.h>

namespace opz {

/** Drum's core: the kit and the voices playing it. */
struct KitCore : DrumCore {
	static_assert(SOUNDS == Kit::SLICES, "a kit has a sound for each key");
	static constexpr float CHOKE = 0.003f; // seconds a cut-off sound takes to fade

	struct Voice {
		xyz::Adsr envelope;
		const Kit::Slice* slice = nullptr;
		double position = 0.0;  // in the kit's samples
		int channel = -1;       // the gate channel holding it, or -1 once its note is over
		bool reverse = false;
		bool held = false;      // its envelope follows its note, rather than staying open to the sound's end
		bool looping = false;
		bool choked = false;    // fading, for a newer sound
		float velocity = 1.f, fade = 1.f;
		uint32_t started = 0;

		bool active() const {
			return envelope.active();
		}

		void stop() {
			envelope = xyz::Adsr();
			channel = -1;
		}
	};

	Voice voices[PORT_MAX_CHANNELS];
	// A kit from a file: the audio thread's, and one the UI thread hands it, swapped under the mutex. The one swapped
	// out stays in `incoming` until the UI thread replaces it, so it's never freed on the audio thread.
	// The UI thread edits the slices of the one playing in place; see editKit().
	std::unique_ptr<Kit> loaded, incoming;
	std::mutex kitMutex;
	std::atomic<bool> handing{false};
	std::atomic<int> builtin{-1};  // the built-in kit played without a file: -1 for its track's own
	std::atomic<int> factoryIndex{-1};          // an OP-Z kit played without a file, in factoryKits(), or -1
	std::atomic<const Kit*> factory{nullptr};   // and the kit itself, once the UI thread has read it
	const Kit* playing = nullptr;  // the kit the voices are playing
	std::atomic<int> lastSound{0}; // the slice played last, for the kit editor
	json_t* savedKit = nullptr;    // a patch's kit, from dataFromJson until readSavedKit() reads its samples
	// The file a kit from a file keeps its samples in beside the patch, as its module gives it; `make` makes the
	// file's directory first.
	std::function<std::string(bool make)> kitFile;

	KitCore() {
		// Made here, on the UI thread, rather than when the audio thread first wants one.
		for (int t = 0; t < DRUM_TRACKS; t++)
			builtinKit(t);
	}

	~KitCore() {
		json_decref(savedKit);
	}

	const Kit& builtinFor() const {
		int b = builtin;
		return builtinKit(b >= 0 ? b : std::max(busTrack, 0));
	}

	/** The kit played without a file: an OP-Z kit, or a built-in one. */
	const Kit& sharedKit() const {
		const Kit* f = factory;
		return f ? *f : builtinFor();
	}

	/** The kit to play: one from a file, or a shared one. */
	const Kit& playable() const {
		return loaded ? *loaded : sharedKit();
	}

	/** For the UI thread: plays an OP-Z kit, or -1 for none, rather than a file's, and says if its files were read. */
	bool useFactory(int i) {
		const Kit* k = factoryKit(i);
		if (i >= 0 && !k)
			return false;
		factory = k;
		factoryIndex = k ? i : -1;
		setKit(nullptr);
		return true;
	}

	/** Takes a kit the UI thread handed over, unless it's busy handing it. */
	void takeKit() {
		if (!handing)
			return;
		std::unique_lock<std::mutex> lock(kitMutex, std::try_to_lock);
		if (!lock.owns_lock())
			return;
		std::swap(loaded, incoming);
		handing = false;
	}

	/** For the UI thread: a kit from a file to play, or null to go back to the built-in ones. */
	void setKit(std::unique_ptr<Kit> kit) {
		std::lock_guard<std::mutex> lock(kitMutex);
		incoming = std::move(kit);
		handing = true;
	}

	/** For the UI thread: plays a kit from a file, and keeps its sound beside the patch at once, so that an autosave,
	which saves the kit's JSON without an onSave, has it too. */
	void useKit(std::unique_ptr<Kit> kit) {
		system::writeFile(kitFile(true), kit->toWav());
		setKit(std::move(kit));
	}

	/** For the UI thread: the kit from a file that plays, or will once handed over, or null. */
	Kit* fileKit() {
		return handing ? incoming.get() : loaded.get();
	}

	/** For the UI thread: the kit to edit, a copy of the shared one if no file's is loaded. Its slices change in
	place while it plays, so the audio thread reads each slice's ends once a sample, and a voice that catches them
	half changed stops. */
	Kit* editKit() {
		{
			std::lock_guard<std::mutex> lock(kitMutex);
			if (Kit* k = fileKit())
				return k;
		}
		auto copy = std::make_unique<Kit>(sharedKit());
		Kit* k = copy.get();
		useKit(std::move(copy));
		return k;
	}

	/** For the UI thread: a slice's settings, its ends kept in the kit and at least two samples apart. */
	void setSlice(int i, const Kit::Slice& to) {
		Kit* k = editKit();
		Kit::Slice& s = k->slices[i];
		int n = (int) k->samples.size(), start = clamp(to.start, 0, std::max(n - 2, 0)), end = clamp(to.end, start + 2, n);
		s.start = start;
		s.end = end;
		s.speed = clamp(to.speed, 0.25f, 4.f);
		s.gain = clamp(to.gain, 0.f, 4.f);
		s.mode = clamp(to.mode, 0, (int) Kit::LOOP);
		s.reverse = to.reverse;
	}

	const Kit* kit() override {
		std::lock_guard<std::mutex> lock(kitMutex);
		const Kit* k = fileKit();
		return k ? k : &sharedKit();
	}

	void editSlice(int i, const Kit::Slice& to) override {
		setSlice(i, to);
	}

	int lastSlice() override {
		return lastSound;
	}

	std::string soundName(int pitch) override {
		std::lock_guard<std::mutex> lock(kitMutex);
		const Kit* k = fileKit();
		int i = soundOf((pitch - 60) / 12.f);
		const Kit::Slice& s = (k ? *k : sharedKit()).slices[i];
		return s.empty() ? "no sound" : !s.name.empty() ? s.name : string::f("sound %d", i + 1);
	}

	const xyz::Adsr* envelope(int voice, float* velocity) const override {
		if (voice >= PORT_MAX_CHANNELS)
			return nullptr;
		*velocity = voices[voice].velocity;
		return &voices[voice].envelope;
	}

	/** The unchoked voice that started first; or, with `spare`, an idle voice, else the choked one that started first. */
	Voice* oldest(bool spare) {
		Voice* best = nullptr;
		for (Voice& v : voices) {
			if (spare && !v.active())
				return &v;
			if (v.active() && v.choked == spare && (!best || v.started < best->started))
				best = &v;
		}
		return best;
	}

	void noteOn(int c) override {
		int sound = soundOf(notePitch(c));
		const Kit::Slice& s = playing->slices[sound];
		lastSound = sound;
		if (s.empty())
			return;
		int limit = style == MONO ? 1 : std::max(2, channels), sounding = 0;
		for (Voice& v : voices)
			sounding += v.active() && !v.choked;
		for (; sounding >= limit; sounding--)
			oldest(false)->choked = true;
		Voice& v = *oldest(true);
		v.envelope = xyz::Adsr();
		v.envelope.gate(true);
		v.slice = &s;
		v.reverse = s.reverse != (controls.p2 >= 0.5f);
		v.position = v.reverse ? s.end - 1 : s.start;
		v.held = style == GATE || style == LOOP || s.mode != Kit::ONESHOT;
		v.looping = style == LOOP || s.mode == Kit::LOOP;
		v.channel = c;
		v.velocity = velocity(c);
		v.fade = 1.f;
		v.choked = false;
		noteStarted();
		v.started = notesPlayed;
	}

	/** Lets go of a note: a held sound releases, and a looping one plays out to its end. */
	void release(Voice& v) {
		v.channel = -1;
		if (v.held) {
			v.envelope.gate(false);
			v.looping = false;
		}
	}

	void noteOff(int c) override {
		for (Voice& v : voices)
			if (v.channel == c)
				release(v);
	}

	void releaseAll() override {
		for (Voice& v : voices)
			release(v);
	}

	void process(const ProcessArgs& args) override {
		takeKit();
		const Kit* k = &playable();
		if (k != playing) {
			for (Voice& v : voices)
				v.stop();
			playing = k;
		}
		prepare(args);

		double step = dsp::exp2_taylor5(controls.p1 / 12.f + controls.pitch) * playing->rate * args.sampleTime;
		float mix = 0.f;
		for (Voice& v : voices) {
			if (!v.active())
				continue;
			const Kit::Slice& s = *v.slice;
			int start = s.start, end = s.end;
			if (end - start < 2) {
				v.stop();
				continue;
			}
			mix += playing->at(v.position, start, end) * s.gain * v.envelope.process(controls.envelope) * v.velocity * v.fade;
			v.position += (v.reverse ? -step : step) * s.speed;
			if (v.position >= end || v.position < start) {
				double length = end - start, into = std::fmod(v.position - start, length);
				if (v.looping)
					v.position = start + (into < 0.0 ? into + length : into);
				else
					v.stop();
			}
			if (v.choked && (v.fade -= args.sampleTime / CHOKE) <= 0.f)
				v.stop();
		}
		float x = filter(mix);
		emit(x, x, controls.fx1, controls.fx2, args.sampleTime);
	}

	void dataToJson(json_t* root) override {
		DrumCore::dataToJson(root);
		json_object_set_new(root, "builtin", json_integer(builtin));
		int f = factoryIndex;
		if (f >= 0)
			json_object_set_new(root, "opz", json_string(factoryKits()[f].id.c_str()));
		std::lock_guard<std::mutex> lock(kitMutex);
		if (const Kit* k = fileKit())
			json_object_set_new(root, "kit", k->toJson());
	}

	void dataFromJson(json_t* root) override {
		DrumCore::dataFromJson(root);
		if (json_t* j = json_object_get(root, "builtin"))
			builtin = clamp((int) json_integer_value(j), -1, DRUM_TRACKS - 1);
		const char* opz = json_string_value(json_object_get(root, "opz"));
		int i = opz ? factoryKitIndex(opz) : -1;
		if (opz && (i < 0 || !useFactory(i)))
			WARN("Drum: no OP-Z kit %s to read, so the built-in kit plays", opz);
		if (factoryIndex != i)
			useFactory(-1);
		json_decref(savedKit);
		savedKit = json_incref(json_object_get(root, "kit"));
		if (!savedKit)
			setKit(nullptr);
	}

	/** Once its module is added: a file's kit keeps its samples beside the patch, in kitFile, and its slices in the
	JSON. A pasted Drum has no storage of its own yet, and a patch can lose the file, so without it the built-in kit
	plays. */
	void readSavedKit() {
		if (!savedKit)
			return;
		std::string path = kitFile(false);
		std::unique_ptr<Kit> k;
		try {
			if (system::isFile(path))
				k = Kit::fromSaved(system::readFile(path), savedKit);
		}
		catch (const Exception&) {}
		if (k)
			setKit(std::move(k));
		else
			WARN("Drum: can't read %s, so the built-in kit plays", path.c_str());
		json_decref(savedKit);
		savedKit = nullptr;
	}

	/** When the patch is saved: a file's kit's samples into kitFile, or no file without one. */
	void saveKit() {
		std::lock_guard<std::mutex> lock(kitMutex);
		const Kit* k = fileKit();
		std::string path = kitFile(k != nullptr);
		if (k)
			system::writeFile(path, k->toWav());
		else if (system::isFile(path))
			system::remove(path);
	}
};


struct Drum : TrackModule<KitCore> {
	static constexpr const char* SLUG = "Drum";

	Drum() {
		core.kitFile = [this](bool make) {
			return system::join(make ? createPatchStorageDirectory() : getPatchStorageDirectory(), "kit.wav");
		};
	}

	json_t* rowJson() override {
		json_t* root = Track::rowJson();
		int f = core.factoryIndex;
		if (f >= 0)
			json_object_set_new(root, "opz", json_string(factoryKits()[f].id.c_str()));
		else
			json_object_set_new(root, "builtin", json_integer(core.builtin));
		std::lock_guard<std::mutex> lock(core.kitMutex);
		json_object_set_new(root, "file", json_boolean(core.fileKit()));
		return root;
	}

	void onAdd(const AddEvent& e) override {
		core.readSavedKit();
	}

	void onSave(const SaveEvent& e) override {
		core.saveKit();
	}
};


struct DrumWidget : TrackWidget {
	DrumWidget(Drum* module) : TrackWidget(module, Drum::SLUG) {}

	static void load(KitCore* m, bool folder) {
		osdialog_filters* filters = folder ? nullptr : osdialog_filters_parse("Kits and sounds:aif,aiff,aifc,wav");
		char* chosen = osdialog_file(folder ? OSDIALOG_OPEN_DIR : OSDIALOG_OPEN, nullptr, nullptr, filters);
		if (filters)
			osdialog_filters_free(filters);
		if (!chosen)
			return;
		std::string path = chosen, error;
		std::free(chosen);
		try {
			if (std::unique_ptr<Kit> kit = folder ? Kit::loadFolder(path, &error) : Kit::load(path, &error))
				return m->useKit(std::move(kit));
		}
		catch (const Exception& e) {
			error = e.what();
		}
		osdialog_message(OSDIALOG_WARNING, OSDIALOG_OK, ("Couldn't load " + path + ": " + error).c_str());
	}

	/** The kit playing as an OP-1 or OP-Z kit file, built in or not. */
	static void save(KitCore* m) {
		std::vector<uint8_t> file;
		std::string name;
		bool cut = false;
		{
			std::lock_guard<std::mutex> lock(m->kitMutex);
			const Kit* k = m->fileKit();
			const Kit& kit = k ? *k : m->sharedKit();
			file = kit.toOp1(&cut);
			name = kit.name;
		}
		osdialog_filters* filters = osdialog_filters_parse("OP-1 and OP-Z kits:aif");
		char* chosen = osdialog_file(OSDIALOG_SAVE, nullptr, (name + ".aif").c_str(), filters);
		osdialog_filters_free(filters);
		if (!chosen)
			return;
		std::string path = chosen;
		std::free(chosen);
		if (system::getExtension(path).empty())
			path += ".aif";
		try {
			system::writeFile(path, file);
		}
		catch (const Exception& e) {
			return (void) osdialog_message(OSDIALOG_WARNING, OSDIALOG_OK, ("Couldn't write " + path + ": " + e.what()).c_str());
		}
		if (cut)
			osdialog_message(OSDIALOG_INFO, OSDIALOG_OK, "The kit is longer than the 12 seconds a kit file holds, so it was cut short there.");
	}

	void appendContextMenu(Menu* menu) override {
		TrackWidget::appendContextMenu(menu);
		KitCore* m = &getModule<Drum>()->core;
		std::string name;
		{
			std::lock_guard<std::mutex> lock(m->kitMutex);
			const Kit* k = m->fileKit();
			name = k ? k->name : m->factory ? m->factory.load()->name + " (OP-Z)" : m->builtinFor().name + " (built in)";
		}
		menu->addChild(new MenuSeparator);
		menu->addChild(createMenuLabel("Kit: " + name));
		menu->addChild(createMenuItem("Load a kit file…", "OP-1, OP-Z, AIFF or WAV", [=]() { load(m, false); }));
		menu->addChild(createMenuItem("Load a folder of sounds…", "one per key", [=]() { load(m, true); }));
		menu->addChild(createMenuItem("Save as a kit file…", "OP-1 and OP-Z", [=]() { save(m); }));
		menu->addChild(createSubmenuItem("OP-Z kit", "", [=](Menu* sub) {
			const std::vector<FactoryKit>& kits = factoryKits();
			if (kits.empty())
				sub->addChild(createMenuLabel("None in the plugin's res/kits"));
			for (int i = 0; i < (int) kits.size(); i++) {
				if (i > 0 && kits[i].track != kits[i - 1].track)
					sub->addChild(new MenuSeparator);
				sub->addChild(createCheckMenuItem(kits[i].name, "",
					[=]() { std::lock_guard<std::mutex> lock(m->kitMutex); return !m->fileKit() && m->factoryIndex == i; },
					[=]() {
						if (!m->useFactory(i))
							osdialog_message(OSDIALOG_WARNING, OSDIALOG_OK, ("Couldn't read the kit " + kits[i].id + " in the plugin's res/kits.").c_str());
					}));
			}
		}));
		menu->addChild(createSubmenuItem("Built-in kit", "", [=](Menu* sub) {
			static const char* const names[] = {"Its track's", "Kick", "Snare", "Perc", "Sample"};
			for (int i = -1; i < DRUM_TRACKS; i++)
				sub->addChild(createCheckMenuItem(names[i + 1], "",
					[=]() { std::lock_guard<std::mutex> lock(m->kitMutex); return !m->fileKit() && !m->factory && m->builtin == i; },
					[=]() { m->builtin = i; m->useFactory(-1); }));
		}));
	}
};

} // namespace opz


Model* modelDrum = createModel<opz::Drum, opz::DrumWidget>(opz::Drum::SLUG);
