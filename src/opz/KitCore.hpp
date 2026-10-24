#pragma once
#include "DrumTrack.hpp"
#include <functional>
#include <mutex>

/*
A drum track's kit player: kick, snare, perc or sample. It plays a kit of 24 sounds, one per key from F3 to E5, the
octaves either side playing them again. Its ten slots, on the black keys in track mode, each hold a kit: one of the
OP-Z's own, sampled; one of the built-in kits, synthesized; or a kit from a file, an OP-1 or OP-Z kit, any AIFF or WAV
cut into 24, or a folder of sounds, whose samples are kept beside the patch.

PITCH tunes every sound and REVERSE plays them backwards. The note styles are the OP-Z's drum ones: retrig plays each
sound to its end, so a sound's tail rings under the next, as many at once as there are notes (two at least, as on the
OP-Z); mono cuts the last sound off; gate stops each sound with its note; loop repeats it while its note is held,
then plays it out. A kit sound that plays gated or loops does so whatever the style.
*/
namespace opz {

struct KitCore : DrumCore {
	static_assert(SOUNDS == Kit::SLICES, "a kit has a sound for each key");
	static constexpr int SLOTS = 10;
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

	/** A kit on one of the black keys. */
	struct KitSlot {
		std::atomic<int> factory{-1};            // an OP-Z kit, by its place in factoryKits()
		std::atomic<int> builtin{-1};            // or a built-in kit, by its track
		std::atomic<const Kit*> shared{nullptr}; // and the kit itself, read on the UI thread
		// A kit from a file: the audio thread's, and one the UI thread hands it, swapped under the mutex. The one
		// swapped out stays in `incoming` until the UI thread replaces it, so it's never freed on the audio thread.
		// The UI thread edits the slices of the one playing in place; see editKit().
		std::unique_ptr<Kit> loaded, incoming;
		std::atomic<bool> handing{false};
		std::atomic<bool> filed{false};          // whether a kit from a file plays here, or is on its way
		json_t* saved = nullptr;                 // a patch's kit, from dataFromJson until readSavedKits() reads its samples
	};

	Voice voices[PORT_MAX_CHANNELS];
	KitSlot slots[SLOTS];
	std::atomic<int> current{0};   // the slot playing
	std::mutex kitMutex;
	const Kit* playing = nullptr;  // the kit the voices are playing
	std::atomic<int> lastSound{0}; // the slice played last, for the kit editor
	// The file a slot's kit from a file keeps its samples in beside the patch; `make` makes the file's directory first.
	std::function<std::string(int slot, bool make)> kitFile;

	KitCore() {
		// Made here, on the UI thread, rather than when the audio thread first wants one.
		for (int t = 0; t < DRUM_TRACKS; t++)
			builtinKit(t);
	}

	~KitCore() {
		for (KitSlot& s : slots)
			json_decref(s.saved);
	}

	void pick(int s) override {
		current = clamp(s, 0, SLOTS - 1);
	}

	// For the UI thread

	/** A track's kits to begin with: the OP-Z's for it, then its built-in one. */
	void fillDefaults(int track) {
		const std::vector<FactoryKit>& kits = factoryKits();
		int s = 0;
		for (int i = 0; i < (int) kits.size() && s < SLOTS - 1; i++)
			if (kits[i].track == track && setFactory(s, i))
				s++;
		setBuiltin(s, track);
		for (s++; s < SLOTS; s++)
			clearSlot(s);
	}

	/** Puts an OP-Z kit in a slot, and says if its files were read. */
	bool setFactory(int s, int i) {
		const Kit* k = factoryKit(i);
		if (!k)
			return false;
		setShared(s, k, i, -1);
		return true;
	}

	void setBuiltin(int s, int track) {
		setShared(s, &builtinKit(track), -1, track);
	}

	void clearSlot(int s) {
		setShared(s, nullptr, -1, -1);
	}

	void setShared(int s, const Kit* k, int factory, int builtin) {
		KitSlot& slot = slots[s];
		slot.shared = k;
		slot.factory = factory;
		slot.builtin = builtin;
		setKit(s, nullptr);
	}

	/** A kit from a file to play in a slot, or null for its shared kit. */
	void setKit(int s, std::unique_ptr<Kit> kit) {
		std::lock_guard<std::mutex> lock(kitMutex);
		slots[s].filed = bool(kit);
		slots[s].incoming = std::move(kit);
		slots[s].handing = true;
	}

	/** Plays a kit from a file in a slot, and keeps its sound beside the patch at once, so that an autosave, which
	saves the kit's JSON without an onSave, has it too. */
	void useKit(int s, std::unique_ptr<Kit> kit) {
		system::writeFile(kitFile(s, true), kit->toWav());
		setShared(s, nullptr, -1, -1);
		setKit(s, std::move(kit));
	}

	/** What a slot holds, to put back for undo: an OP-Z kit, a built-in one, a copy of a file's, or nothing. */
	struct Held {
		int factory = -1, builtin = -1;
		std::shared_ptr<const Kit> file;
		const Kit* was = nullptr;   // the kit it was taken from, only to tell whether the slot changed

		bool operator==(const Held& o) const {
			return was == o.was && factory == o.factory && builtin == o.builtin;
		}
	};

	Held held(int s) {
		std::lock_guard<std::mutex> lock(kitMutex);
		Held h;
		h.was = kitIn(s);
		if (const Kit* k = fileKit(s))
			h.file = std::make_shared<const Kit>(*k);
		else {
			h.factory = slots[s].factory;
			h.builtin = slots[s].builtin;
		}
		return h;
	}

	void restore(int s, const Held& h) {
		if (h.file)
			useKit(s, std::make_unique<Kit>(*h.file));
		else if (h.factory < 0 || !setFactory(s, h.factory)) {
			if (h.builtin >= 0)
				setBuiltin(s, h.builtin);
			else
				clearSlot(s);
		}
	}

	/** The kit from a file in a slot, playing or handed over, or null. Under the mutex. */
	Kit* fileKit(int s) {
		KitSlot& slot = slots[s];
		return slot.handing ? slot.incoming.get() : slot.loaded.get();
	}

	/** The kit in a slot, or null for an empty one. Under the mutex. */
	const Kit* kitIn(int s) {
		const Kit* f = fileKit(s);
		return f ? f : slots[s].shared.load();
	}

	bool filled(int s) {
		std::lock_guard<std::mutex> lock(kitMutex);
		return kitIn(s);
	}

	/** What a slot holds, for menus and labels, `tagged` with where it's from. */
	std::string kitName(int s, bool tagged = true) {
		std::lock_guard<std::mutex> lock(kitMutex);
		const Kit* k = kitIn(s);
		if (!k)
			return "";
		return fileKit(s) || !tagged ? k->name : k->name + (slots[s].factory >= 0 ? " (OP-Z)" : " (built in)");
	}

	/** The slot holding the kit a Drum module played before 2.1.0, put in the first empty slot where none does. A kit
	from a file comes from the module's `storage` folder, and is -1 without it. */
	int oldKit(json_t* data, int track, const char* storage) {
		if (json_t* saved = json_object_get(data, "kit")) {
			std::unique_ptr<Kit> k;
			std::string path = storage ? system::join(storage, "kit.wav") : "";
			try {
				if (storage && system::isFile(path))
					k = Kit::fromSaved(system::readFile(path), saved);
			}
			catch (const Exception&) {}
			if (!k) {
				WARN("Drum track %d: its Drum module's kit from a file isn't beside the patch, and needs loading again", track + 1);
				return -1;
			}
			int s = 0;
			while (s < SLOTS - 1 && filled(s))
				s++;
			useKit(s, std::move(k));
			return s;
		}
		const char* opz = json_string_value(json_object_get(data, "opz"));
		int f = opz ? factoryKitIndex(opz) : -1;
		json_t* b = json_object_get(data, "builtin");
		int builtin = b ? clamp((int) json_integer_value(b), -1, DRUM_TRACKS - 1) : -1;
		if (builtin < 0)
			builtin = track;
		for (int s = 0; s < SLOTS; s++)
			if (f >= 0 ? slots[s].factory == f : slots[s].builtin == builtin && !filedKit(s))
				return s;
		for (int s = 0; s < SLOTS; s++)
			if (!filled(s)) {
				if (f < 0 || !setFactory(s, f))
					setBuiltin(s, builtin);
				return s;
			}
		return -1;
	}

	bool filedKit(int s) {
		std::lock_guard<std::mutex> lock(kitMutex);
		return fileKit(s);
	}

	/** The kit to edit: the current slot's from a file, or a copy of its shared one, which then plays from the slot
	as a file's. Its slices change in place while it plays, so the audio thread reads each slice's ends once a sample,
	and a voice that catches them half changed stops. Null for an empty slot. */
	Kit* editKit() {
		int s = current;
		std::unique_ptr<Kit> copy;
		{
			std::lock_guard<std::mutex> lock(kitMutex);
			if (Kit* k = fileKit(s))
				return k;
			if (const Kit* k = slots[s].shared)
				copy = std::make_unique<Kit>(*k);
		}
		if (!copy)
			return nullptr;
		Kit* k = copy.get();
		useKit(s, std::move(copy));
		return k;
	}

	/** A slice's settings, its ends kept in the kit and at least two samples apart. */
	void setSlice(int i, const Kit::Slice& to) {
		Kit* k = editKit();
		if (!k)
			return;
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
		return kitIn(current);
	}

	void editSlice(int i, const Kit::Slice& to) override {
		setSlice(i, to);
	}

	int lastSlice() override {
		return lastSound;
	}

	std::string soundName(int pitch) override {
		std::lock_guard<std::mutex> lock(kitMutex);
		const Kit* k = kitIn(current);
		int i = soundOf((pitch - 60) / 12.f);
		if (!k)
			return "no kit";
		const Kit::Slice& s = k->slices[i];
		return s.empty() ? "no sound" : !s.name.empty() ? s.name : string::f("sound %d", i + 1);
	}

	// For the audio thread

	/** Whether a slot holds a kit: a shared one, or one from a file, playing or on its way. */
	bool holds(int s) const {
		const KitSlot& slot = slots[clamp(s, 0, SLOTS - 1)];
		return slot.shared.load() || slot.filed;
	}

	/** Takes the kits the UI thread handed over, unless it's busy handing one. */
	void takeKits() {
		for (KitSlot& slot : slots) {
			if (!slot.handing)
				continue;
			std::unique_lock<std::mutex> lock(kitMutex, std::try_to_lock);
			if (!lock.owns_lock())
				return;
			std::swap(slot.loaded, slot.incoming);
			slot.handing = false;
		}
	}

	/** The current slot's kit, or null. */
	const Kit* playable() const {
		const KitSlot& slot = slots[current];
		return slot.loaded ? slot.loaded.get() : slot.shared.load();
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
		lastSound = sound;
		if (!playing || playing->slices[sound].empty())
			return;
		const Kit::Slice& s = playing->slices[sound];
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
		takeKits();
		const Kit* k = playable();
		if (k != playing) {
			for (Voice& v : voices)
				v.stop();
			playing = k;
		}
		prepare(args);

		float mix = 0.f;
		if (playing) {
			double step = dsp::exp2_taylor5(controls.p1 / 12.f + controls.pitch) * playing->rate * args.sampleTime;
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
		}
		float x = filter(mix);
		emit(x, x, controls.fx1, controls.fx2, args.sampleTime);
	}

	// Patches

	/** Its kits, as a list by slot: an OP-Z kit's id, a built-in kit's track, a file's slices, or null. */
	void dataToJson(json_t* root) override {
		DrumCore::dataToJson(root);
		json_t* kits = json_array();
		std::lock_guard<std::mutex> lock(kitMutex);
		for (int s = 0; s < SLOTS; s++) {
			const KitSlot& slot = slots[s];
			json_t* j = json_null();
			if (const Kit* k = fileKit(s))
				j = json_pack("{s:o}", "kit", k->toJson());
			else if (slot.factory >= 0)
				j = json_pack("{s:s}", "opz", factoryKits()[slot.factory].id.c_str());
			else if (slot.builtin >= 0)
				j = json_pack("{s:i}", "builtin", (int) slot.builtin);
			json_array_append_new(kits, j);
		}
		json_object_set_new(root, "kits", kits);
	}

	void dataFromJson(json_t* root) override {
		DrumCore::dataFromJson(root);
		json_t* kits = json_object_get(root, "kits");
		if (!kits)
			return;
		for (int s = 0; s < SLOTS; s++) {
			json_t* j = json_array_get(kits, s);
			json_decref(slots[s].saved);
			slots[s].saved = json_incref(json_object_get(j, "kit"));
			const char* opz = json_string_value(json_object_get(j, "opz"));
			json_t* builtin = json_object_get(j, "builtin");
			int i = opz ? factoryKitIndex(opz) : -1;
			if (opz && (i < 0 || !setFactory(s, i))) {
				WARN("Drum track: no OP-Z kit %s to read, so its slot is empty", opz);
				clearSlot(s);
			}
			else if (builtin)
				setBuiltin(s, clamp((int) json_integer_value(builtin), 0, DRUM_TRACKS - 1));
			else if (!opz)
				clearSlot(s);
		}
	}

	/** Once its patch is loaded: a file's kit keeps its samples beside the patch, in kitFile, and its slices in the
	JSON. A pasted Sequencer has no storage of its own yet, and a patch can lose the file, so without it the slot is
	empty. */
	void readSavedKits() {
		for (int s = 0; s < SLOTS; s++) {
			KitSlot& slot = slots[s];
			if (!slot.saved)
				continue;
			std::string path = kitFile(s, false);
			std::unique_ptr<Kit> k;
			try {
				if (system::isFile(path))
					k = Kit::fromSaved(system::readFile(path), slot.saved);
			}
			catch (const Exception&) {}
			if (k)
				setKit(s, std::move(k));
			else
				WARN("Drum track: can't read %s, so its slot is empty", path.c_str());
			json_decref(slot.saved);
			slot.saved = nullptr;
		}
	}

	/** When the patch is saved: each file's kit's samples into its kitFile, and no file for the other slots. */
	void saveKits() {
		std::lock_guard<std::mutex> lock(kitMutex);
		for (int s = 0; s < SLOTS; s++) {
			const Kit* k = fileKit(s);
			std::string path = kitFile(s, k != nullptr);
			if (k)
				system::writeFile(path, k->toWav());
			else if (system::isFile(path))
				system::remove(path);
		}
	}
};

/** For the UI thread, from Drum.cpp: asks for a kit file, or a folder of sounds, to play in a slot; and saves a slot's
kit as an OP-1 and OP-Z kit file. */
void loadKit(KitCore* m, int slot, bool folder);
void saveKit(KitCore* m, int slot);

} // namespace opz
