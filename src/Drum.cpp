/*
Drum: an OP-Z drum track, kick, snare, perc or sample, as the first to fourth drum engine to the right of a Sequencer
takes. It plays a kit of 24 sounds, one per key from F3 to E5, the octaves either side playing them again. Each track
starts on a kit of its own, synthesized; the context menu loads an OP-1 or OP-Z kit file, any AIFF or WAV cut into 24,
or a folder of sounds.

PITCH tunes every sound and REVERSE plays them backwards. The note styles are the OP-Z's drum ones: retrig plays each
sound to its end, so a sound's tail rings under the next, as many at once as there are notes (two at least, as on the
OP-Z); mono cuts the last sound off; gate stops each sound with its note; loop repeats it while its note is held,
then plays it out. A kit sound that plays gated or loops does so whatever the style.
*/
#include "opz/TrackWidget.hpp"
#include "opz/Kit.hpp"
#include <mutex>
#include <osdialog.h>

namespace opz {

struct Drum : Track {
	static constexpr const char* SLUG = "Drum";
	enum NoteStyle { RETRIG, MONO, GATE, LOOP };
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
	std::unique_ptr<const Kit> loaded, incoming;
	std::mutex kitMutex;
	std::atomic<bool> handing{false};
	std::atomic<int> builtin{-1};  // the built-in kit played without a file: -1 for its track's own
	const Kit* playing = nullptr;  // the kit the voices are playing
	json_t* savedKit = nullptr;    // a patch's kit, from dataFromJson until onAdd reads its samples

	Drum() {
		drum = true;
		configParam(P1_PARAM, -12.f, 12.f, 0.f, "Pitch", " semitones")->description = "Tunes every sound, up to an octave either way";
		configSwitch(P2_PARAM, 0.f, 1.f, 0.f, "Reverse", {"Off", "On"})->description = "Plays each sound from its end, as it starts";
		configTrack({"Retrig", "Mono", "Gate", "Loop"},
			"Retrig plays each sound to its end, and mono cuts the last one off; gate stops it with its note, and loop repeats it while the note is held",
			{"Pitch", "Reverse", "Filter", "Resonance", "Attack", "Pitch, 2 octaves", "Pan", "Volume"});
		configDefault(SUSTAIN_PARAM, 1.f);
		configDefault(RELEASE_PARAM, 0.4f);
		getParamQuantity(GLIDE_PARAM)->description = "Synth tracks only";
		// Made here, on the UI thread, rather than when the audio thread first wants one.
		for (int t = 0; t < DRUM_TRACKS; t++)
			builtinKit(t);
	}

	~Drum() {
		json_decref(savedKit);
	}

	/** Drum tracks have no portamento. */
	int dial(int page, int d) const override {
		return page == PAGE_TRACK && d == 3 ? -1 : DIALS[page][d];
	}

	/** The sound a note plays: the keys F3 to E5 play the 24 in turn, and the octaves either side play them again. */
	static int sliceOf(float pitch) {
		int key = (int) std::round(12.f * pitch) + 7;
		return (key % Kit::SLICES + Kit::SLICES) % Kit::SLICES;
	}

	const Kit& builtinFor() const {
		int b = builtin;
		return builtinKit(b >= 0 ? b : std::max(busTrack, 0));
	}

	/** The kit to play: one from a file, or the built-in one. */
	const Kit& kit() const {
		return loaded ? *loaded : builtinFor();
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
	void setKit(std::unique_ptr<const Kit> kit) {
		std::lock_guard<std::mutex> lock(kitMutex);
		incoming = std::move(kit);
		handing = true;
	}

	/** For the UI thread: the kit from a file that plays, or will once handed over, or null. */
	const Kit* fileKit() {
		return handing ? incoming.get() : loaded.get();
	}

	std::string soundName(int pitch) override {
		std::lock_guard<std::mutex> lock(kitMutex);
		const Kit* k = fileKit();
		int i = sliceOf((pitch - 60) / 12.f);
		const Kit::Slice& s = (k ? *k : builtinFor()).slices[i];
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
		const Kit::Slice& s = playing->slices[sliceOf(notePitch(c))];
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
		const Kit* k = &kit();
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
			mix += playing->at(v.position, s) * s.gain * v.envelope.process(controls.envelope) * v.velocity * v.fade;
			v.position += (v.reverse ? -step : step) * s.speed;
			if (v.position >= s.end || v.position < s.start) {
				double length = s.end - s.start, into = std::fmod(v.position - s.start, length);
				if (v.looping)
					v.position = s.start + (into < 0.0 ? into + length : into);
				else
					v.stop();
			}
			if (v.choked && (v.fade -= args.sampleTime / CHOKE) <= 0.f)
				v.stop();
		}
		float x = filter(mix);
		emit(x, x, controls.fx1, controls.fx2, args.sampleTime);
	}

	json_t* dataToJson() override {
		json_t* root = Track::dataToJson();
		json_object_set_new(root, "builtin", json_integer(builtin));
		std::lock_guard<std::mutex> lock(kitMutex);
		if (const Kit* k = fileKit())
			json_object_set_new(root, "kit", k->toJson());
		return root;
	}

	void dataFromJson(json_t* root) override {
		Track::dataFromJson(root);
		if (json_t* j = json_object_get(root, "builtin"))
			builtin = clamp((int) json_integer_value(j), -1, DRUM_TRACKS - 1);
		json_decref(savedKit);
		savedKit = json_incref(json_object_get(root, "kit"));
		if (!savedKit)
			setKit(nullptr);
	}

	/** A file's kit lives in the patch's storage, as kit.wav, beside the slices in its JSON. */
	void onAdd(const AddEvent& e) override {
		if (!savedKit)
			return;
		std::vector<uint8_t> wav = system::readFile(system::join(getPatchStorageDirectory(), "kit.wav"));
		if (std::unique_ptr<Kit> k = Kit::fromSaved(wav, savedKit))
			setKit(std::move(k));
		json_decref(savedKit);
		savedKit = nullptr;
	}

	void onSave(const SaveEvent& e) override {
		std::lock_guard<std::mutex> lock(kitMutex);
		const Kit* k = fileKit();
		std::string path = system::join(k ? createPatchStorageDirectory() : getPatchStorageDirectory(), "kit.wav");
		if (k)
			system::writeFile(path, k->toWav());
		else if (system::isFile(path))
			system::remove(path);
	}
};


struct DrumWidget : TrackWidget {
	DrumWidget(Drum* module) : TrackWidget(module, Drum::SLUG) {}

	static void load(Drum* m, bool folder) {
		osdialog_filters* filters = folder ? nullptr : osdialog_filters_parse("Kits and sounds:aif,aiff,aifc,wav");
		char* chosen = osdialog_file(folder ? OSDIALOG_OPEN_DIR : OSDIALOG_OPEN, nullptr, nullptr, filters);
		if (filters)
			osdialog_filters_free(filters);
		if (!chosen)
			return;
		std::string path = chosen, error;
		std::free(chosen);
		std::unique_ptr<Kit> kit = folder ? Kit::loadFolder(path, &error) : Kit::load(path, &error);
		if (kit)
			m->setKit(std::move(kit));
		else
			osdialog_message(OSDIALOG_WARNING, OSDIALOG_OK, ("Couldn't load " + path + ": " + error).c_str());
	}

	void appendContextMenu(Menu* menu) override {
		TrackWidget::appendContextMenu(menu);
		Drum* m = getModule<Drum>();
		std::string name;
		{
			std::lock_guard<std::mutex> lock(m->kitMutex);
			const Kit* k = m->fileKit();
			name = k ? k->name : m->builtinFor().name + " (built in)";
		}
		menu->addChild(new MenuSeparator);
		menu->addChild(createMenuLabel("Kit: " + name));
		menu->addChild(createMenuItem("Load a kit file…", "OP-1, OP-Z, AIFF or WAV", [=]() { load(m, false); }));
		menu->addChild(createMenuItem("Load a folder of sounds…", "one per key", [=]() { load(m, true); }));
		menu->addChild(createSubmenuItem("Built-in kit", "", [=](Menu* sub) {
			static const char* const names[] = {"Its track's", "Kick", "Snare", "Perc", "Sample"};
			for (int i = -1; i < DRUM_TRACKS; i++)
				sub->addChild(createCheckMenuItem(names[i + 1], "",
					[=]() { std::lock_guard<std::mutex> lock(m->kitMutex); return !m->fileKit() && m->builtin == i; },
					[=]() { m->builtin = i; m->setKit(nullptr); }));
		}));
	}
};

} // namespace opz


Model* modelDrum = createModel<opz::Drum, opz::DrumWidget>(opz::Drum::SLUG);
