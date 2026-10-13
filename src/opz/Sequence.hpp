#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <limits>

/*
An OP-Z track's sequence and the player that runs it, knowing nothing of Rack.

Time is counted in sixteenth notes, the length of a step at step length ×1. The player's clock runs all the time,
for notes played live; the sequence's own time, song time, stands still while it's stopped, so it can carry on. A
step is split into 24 ticks for micro timing and note lengths, so those stretch with the step length. The player
schedules each step slot one slot ahead, which covers micro timing from -23 to +24 ticks and still starts every note
exactly on time. A slot is one step's turn under the playhead: pulse and pulse hold give a step several. A step's
parameter locks hold its engine's dials for as long as its slots are under the playhead.
*/
namespace opz {

constexpr int STEPS = 16, STEP_NOTES = 4, TICKS = 24, CHANNELS = 16;
constexpr int TRACKS = 8, DRUM_TRACKS = 4, SYNTH_TRACKS = TRACKS - DRUM_TRACKS; // kick, snare, perc, sample, then bass, lead, arp, chord
constexpr int MIN_OFFSET = 1 - TICKS, MAX_OFFSET = TICKS;
constexpr int DRONE = std::numeric_limits<int16_t>::max();
constexpr int SOUND_PARAMS = 18; // a synth track's dials, every page's and the track settings', which a step can lock

/** The step components, in the order of the white keys that pick them. */
enum Component {
	PULSE, PULSE_HOLD, MULTIPLY, VELOCITY, RAMP_UP, RAMP_DOWN, RANDOM, PORTAMENTO, SWEEP, TONALITY, JUMP,
	PARAMETER_SPARK, COMPONENT_SPARK, TRIGGER_SPARK, COMPONENTS_LEN
};

/** Jump targets, by value key. */
enum Jump { JUMP_START, JUMP_2_4, JUMP_3_4, JUMP_4_4, JUMP_FORWARD, JUMP_BACK, JUMP_RANDOM, JUMP_STAY, JUMP_ALIGN, JUMP_GATE };

/** What a sweep moves, on the engine playing the track. */
enum SweepTarget { SWEEP_FILTER, SWEEP_SYNTH, SWEEP_PAN, SWEEP_TARGETS };

struct Note {
	int8_t pitch = 60;      // MIDI note number: 60 is C4, 0V
	uint8_t velocity = 100;
	int8_t offset = 0;      // micro timing, in ticks
	int16_t length = 0;     // in ticks, or DRONE; 0 follows the track's note length
};

/** A component's value is the value key that set it, 0 to 9 for keys 1 to 9 and 0. The sparks add a mode, 0 to 3,
times 16: which pass of each N fires, cycled by pressing the same key again. */
struct Step {
	Note notes[STEP_NOTES];
	int count = 0;
	uint16_t components = 0;
	uint8_t values[COMPONENTS_LEN] = {};
	uint32_t locks = 0;                  // the dials it locks, by the synth track's param
	float lockValues[SOUND_PARAMS] = {}; // and where, 0 to 1 over each dial's travel

	bool empty() const {
		return count == 0;
	}

	bool has(int c) const {
		return components >> c & 1;
	}

	int key(int c) const {
		return values[c] & 15;
	}

	void set(int c, int value) {
		components |= 1 << c;
		values[c] = value;
	}

	void unset(int c) {
		components &= ~(1 << c);
		values[c] = 0;
	}

	void clearComponents() {
		components = 0;
		std::fill(std::begin(values), std::end(values), 0);
	}

	bool locked(int p) const {
		return locks >> p & 1;
	}

	void lock(int p, float value) {
		locks |= 1u << p;
		lockValues[p] = std::clamp(value, 0.f, 1.f);
	}

	void unlock(int p) {
		locks &= ~(1u << p);
		lockValues[p] = 0.f;
	}

	void clearLocks() {
		locks = 0;
		std::fill(std::begin(lockValues), std::end(lockValues), 0.f);
	}

	int indexOf(int pitch) const {
		for (int i = 0; i < count; i++)
			if (notes[i].pitch == pitch)
				return i;
		return -1;
	}

	/** Adds a note, or replaces the one of the same pitch. A full step ignores it. */
	Note* add(Note n) {
		int i = indexOf(n.pitch);
		if (i >= 0)
			return &(notes[i] = n);
		return count < STEP_NOTES ? &(notes[count++] = n) : nullptr;
	}

	void remove(int pitch) {
		count = int(std::remove_if(notes, notes + count, [=](const Note& n) { return n.pitch == pitch; }) - notes);
	}

	/** Lets go of the notes; the components and locks stay. */
	void clear() {
		count = 0;
	}

	/** Takes another step's notes, and its components and locks if it has any. */
	void paste(const Step& s) {
		std::copy(s.notes, s.notes + s.count, notes);
		count = s.count;
		if (s.components) {
			components = s.components;
			std::copy(std::begin(s.values), std::end(s.values), values);
		}
		if (s.locks) {
			locks = s.locks;
			std::copy(std::begin(s.lockValues), std::end(s.lockValues), lockValues);
		}
	}

	/** Moves every note by the same number of ticks, as far as micro timing reaches. */
	void nudge(int ticks) {
		for (int i = 0; i < count; i++)
			notes[i].offset = std::clamp(notes[i].offset + ticks, MIN_OFFSET, MAX_OFFSET);
	}
};

struct Pattern {
	Step steps[STEPS];

	/** Moves every step of the first `count` one place later (1) or earlier (-1), round the loop. */
	void rotate(int direction, int count) {
		Step* end = steps + std::clamp(count, 1, STEPS);
		std::rotate(steps, direction > 0 ? end - 1 : steps + 1, end);
	}

	void clear() {
		for (Step& s : steps)
			s = Step();
	}
};

inline bool operator==(const Note& a, const Note& b) {
	return a.pitch == b.pitch && a.velocity == b.velocity && a.offset == b.offset && a.length == b.length;
}
inline bool operator==(const Step& a, const Step& b) {
	return a.count == b.count && std::equal(a.notes, a.notes + a.count, b.notes) && a.components == b.components
		&& std::equal(std::begin(a.values), std::end(a.values), b.values)
		&& a.locks == b.locks && std::equal(std::begin(a.lockValues), std::end(a.lockValues), b.lockValues);
}
inline bool operator==(const Pattern& a, const Pattern& b) {
	return std::equal(a.steps, a.steps + STEPS, b.steps);
}

/** The track settings the player reads every time it schedules a step. */
struct Settings {
	int stepCount = STEPS;
	int stepLength = 1;       // sixteenths per step; 0 waits for a trigger
	int noteLength = TICKS;   // ticks, or DRONE
	float quantize = 0.f;     // 0 to 1: how far micro timing is pulled to the grid
	float swing = 0.5f;       // the share of each pair of sixteenths the first one takes, 0.5 to 0.75
	int channels = 4;
};

/** The step length set by each value key, 1 to 9 then 0: ×1 to ×8, ×16, and one step per trigger. */
inline int stepLengthOfKey(int key) {
	return key == 9 ? 0 : key == 8 ? 16 : key + 1;
}

/** The note length dial's settings, in ticks of a ×1 step: 1/64 to a bar, dotted between, then drone. */
constexpr int NOTE_LENGTHS[] = {6, 9, 12, 18, 24, 36, 48, 72, 96, 144, 192, 288, 384, DRONE};

/** Scale degrees, counted from the note, that the ramps and random walk through: 2 to 6 notes to the octave. */
inline const int* rampDegrees(int notes, int* size) {
	static const int degrees[5][5] = {{0}, {0, 4}, {0, 2, 4}, {0, 2, 4, 6}, {0, 2, 3, 4, 6}};
	*size = notes - 1;
	return degrees[notes - 2];
}

inline int floorDiv(int a, int b) {
	return a / b - (a % b != 0 && (a < 0) != (b < 0));
}

/** Moves a pitch by whole degrees of C major, the master track's key until there is one. Notes outside the scale
keep their distance from the degree below them. */
inline int diatonic(int pitch, int degrees) {
	static const int major[7] = {0, 2, 4, 5, 7, 9, 11};
	int octave = floorDiv(pitch, 12), pc = pitch - 12 * octave, d = 6;
	while (major[d] > pc)
		d--;
	int t = d + degrees, o = floorDiv(t, 7);
	return 12 * (octave + o) + major[t - 7 * o] + pc - major[d];
}


struct Player {
	/** One channel of the poly outputs. */
	struct Channel {
		float pitch = 0.f, target = 0.f; // V/oct, gliding towards the target
		float glide = 0.f;         // sixteenths to cover most of the way; 0 jumps straight to the note
		float velocity = 0.f;      // 0 to 1
		bool gate = false;
		bool retrigger = false;    // the gate drops for a sample so a note on a busy channel starts afresh
		bool live = false;         // played from the keys or inputs, rather than by the sequence
		double off = 0.0;          // when the gate falls, in sixteenths
		uint32_t id = 0;           // which note it is playing; ids count up, so lower is older
	};

	struct Event {
		double time;
		float pitch, velocity, glide;
		double length;
	};

	struct Slot {
		double start = 0.0, length = 1.0;
		int step = -1;
		bool locks = true;     // false while the step's parameter spark holds its locks back
	};

	/** An out-and-back of one of the engine's dials, in song time. */
	struct Sweep {
		int target = -1;
		float direction = 1.f;
		double start = 0.0, length = 16.0;
	};

	static constexpr int QUEUE = 128;
	Event queue[QUEUE];
	int queued = 0;
	Channel channels[CHANNELS];
	uint32_t notes = 0;

	bool running = false;
	double now = 0.0;
	double offset = 0.0;       // when song time 0 was, by now's clock
	double paused = 0.0;       // the song time it stopped at
	double scheduled = -1.0;   // start of the last slot scheduled, in song time, -1 before the first
	double next = 0.0;         // start of the slot to schedule after it
	Slot slots[2];             // the slot playing and the one after it
	int position = -1;         // the step the playhead is on
	long slotsPlayed = 0;
	bool timed = true;         // false while steps wait for triggers

	// The step under the playhead, as its components left it.
	int slotsLeft = 0;         // slots it still holds
	int holdSlots = 0;         // its pulse hold, or 0
	int jump = -1;             // where it sends the playhead next, or -1 to step on
	bool componentsOn = true, notesOn = true, locksOn = true;
	long passOrigin = 0;       // where the sparks' count of passes starts
	int ramps[3][10] = {};     // how far each ramp setting has walked
	bool gateStep = false;     // a gate step fired: the master advances the tracks that wait for triggers
	Sweep sweeps[2];           // the sweep running, and the one after it
	uint32_t seed = 0x9e3779b9;

	double song() const {
		return now - offset;
	}

	float random() {
		seed ^= seed << 13;
		seed ^= seed >> 17;
		seed ^= seed << 5;
		return seed / 4294967296.f;
	}

	int randomInt(int n) {
		return std::min(int(random() * n), n - 1);
	}

	/** Starts from the first step, letting go of the notes the sequence was playing. */
	void start() {
		stop();
		running = true;
		offset = now;
		scheduled = -1.0;
		next = 0.0;
		slots[0] = slots[1] = Slot();
		position = -1;
		slotsPlayed = 0;
		timed = true;
		slotsLeft = holdSlots = 0;
		jump = -1;
		passOrigin = 0;
		std::fill(std::begin(ramps[0]), std::end(ramps[2]), 0);
		sweeps[0] = sweeps[1] = Sweep();
	}

	void stop() {
		if (running)
			paused = song();
		running = false;
		silence();
	}

	/** Carries on from where it stopped, or starts if it never has. */
	void resume() {
		if (position < 0)
			return start();
		running = true;
		offset = now - paused;
	}

	/** Lets go of the notes the sequence is playing or has queued, and keeps going. */
	void silence() {
		queued = 0;
		for (Channel& c : channels)
			if (!c.live)
				c.gate = false;
	}

	/** Stops, and lets go of every note, the ones played live too. */
	void panic() {
		stop();
		for (Channel& c : channels)
			c.gate = false;
	}

	/** Runs time forward: schedules slots one ahead while running, and starts, glides and ends notes on the way. */
	void advance(double sixteenths, const Pattern& pattern, const Settings& s) {
		now += sixteenths;
		if (running && s.stepLength > 0) {
			if (!timed) {
				// Back from waiting for triggers: pick the grid up at its next step.
				next = std::ceil(song() / s.stepLength) * s.stepLength;
				timed = true;
			}
			while (song() >= scheduled) {
				schedule(pattern, s, next, s.stepLength);
				scheduled = next;
				next += s.stepLength;
			}
		}
		while (queued > 0 && queue[0].time <= now) {
			Event e = queue[0];
			std::copy(queue + 1, queue + queued, queue);
			queued--;
			noteOn(e.pitch, e.velocity, e.length, false, s, e.glide);
		}
		for (Channel& c : channels) {
			if (c.gate && now >= c.off)
				c.gate = false;
			c.pitch += (c.target - c.pitch) * (c.glide > 0.f ? float(std::min(sixteenths / c.glide, 1.0)) : 1.f);
		}
	}

	/** Steps on once, for a track whose step length is 0. */
	void trigger(const Pattern& pattern, const Settings& s) {
		if (!running)
			return;
		timed = false;
		schedule(pattern, s, song(), 1.0);
	}

	const Slot& current() const {
		return slots[1].step >= 0 && song() >= slots[1].start ? slots[1] : slots[0];
	}

	/** The step under the playhead now, or -1. */
	int playing() const {
		return current().step;
	}

	/** The step whose parameter locks hold the engine's dials now, or -1. */
	int locking() const {
		return running && current().locks ? current().step : -1;
	}

	/** Where a note played now belongs: the nearest step, and its micro timing in ticks. */
	bool locate(int* step, int* ticks) const {
		const Slot& a = slots[0];
		const Slot& b = slots[1];
		const Slot& slot = b.step >= 0 && (a.step < 0 || song() - a.start > a.length / 2) ? b : a;
		if (slot.step < 0)
			return false;
		*step = slot.step;
		*ticks = std::clamp((int) std::lround((song() - slot.start) / slot.length * TICKS), MIN_OFFSET, MAX_OFFSET);
		return true;
	}

	/** The length of the step playing, in sixteenths. */
	double stepLength() const {
		return slots[0].step >= 0 ? slots[0].length : slots[1].length;
	}

	/** How far a sweep moves `target` now, -0.5 to 0.5 of the dial's travel. */
	float sweep(int target) const {
		const Sweep& w = sweeps[1].target >= 0 && song() >= sweeps[1].start ? sweeps[1] : sweeps[0];
		double t = (song() - w.start) / w.length;
		if (w.target != target || t < 0.0 || t >= 1.0)
			return 0.f;
		return 0.5f * w.direction * float(std::sin(M_PI * t));
	}

	/** Ticks of a step `length` sixteenths long, in sixteenths. */
	static double ticks(double t, double length) {
		return t * length / TICKS;
	}

	/** Swing delays every second sixteenth; the time between follows it, so ratchets and offsets stay in order. */
	static double swung(double t, float swing) {
		double pair = 2.0 * std::floor(t / 2.0), u = t - pair, mid = 2.0 * swing;
		return pair + (u < 1.0 ? u * mid : mid + (u - 1.0) * (2.0 - mid));
	}

	int count(const Settings& s) const {
		return std::clamp(s.stepCount, 1, STEPS);
	}

	/** Whether a spark fires on this pass. A pass is a step count's worth of slots, whatever the playhead does in
	them, like the hardware's global count, so a step that stays put still sees the passes go by. */
	bool sparked(int value, const Settings& s) {
		int key = value & 15, mode = value >> 4, length = count(s);
		if (key == 8)
			return random() < 0.5f;
		if (key == 9) {
			passOrigin = slotsPlayed - (slotsPlayed - passOrigin) % length;
			return true;
		}
		int n = key + 1, k = int((slotsPlayed - passOrigin) / length % n);
		switch (mode) {
			case 0: return k == n - 1;
			case 1: return k == 0;
			case 2: return k != n - 1;
			default: return k != 0;
		}
	}

	/** Moves the playhead to the next step, or where the last one's jump sends it, and reads its components. */
	void enterStep(const Pattern& pattern, const Settings& s, double start) {
		int n = count(s), from = position, to;
		switch (from < 0 ? -1 : jump) {
			case JUMP_START: to = 0; break;
			case JUMP_2_4: to = 4; break;
			case JUMP_3_4: to = 8; break;
			case JUMP_4_4: to = 12; break;
			case JUMP_FORWARD: to = from + 2; break;
			case JUMP_BACK: to = from - 1 + n; break;
			case JUMP_RANDOM: to = randomInt(n); break;
			case JUMP_STAY: to = from; break;
			case JUMP_ALIGN: to = int(slotsPlayed % n); break;
			default: to = from + 1; break;
		}
		position = to % n;

		const Step& step = pattern.steps[position];
		componentsOn = !step.has(COMPONENT_SPARK) || sparked(step.values[COMPONENT_SPARK], s);
		notesOn = !step.has(TRIGGER_SPARK) || sparked(step.values[TRIGGER_SPARK], s);
		locksOn = !step.has(PARAMETER_SPARK) || sparked(step.values[PARAMETER_SPARK], s);
		auto on = [&](int c) { return componentsOn && step.has(c); };
		auto amount = [&](int c) { return step.key(c) == 9 ? 1 + randomInt(9) : step.key(c) + 1; };
		holdSlots = on(PULSE_HOLD) ? amount(PULSE_HOLD) : 0;
		slotsLeft = holdSlots ? holdSlots : on(PULSE) ? amount(PULSE) : 1;
		jump = on(JUMP) ? step.key(JUMP) : -1;
		gateStep = gateStep || jump == JUMP_GATE;
		if (on(SWEEP)) {
			// Filter up and down, synth up and down and pan over a bar; the first four over two bars; or any of them.
			static const Sweep kinds[9] = {{SWEEP_FILTER, 1.f}, {SWEEP_FILTER, -1.f}, {SWEEP_SYNTH, 1.f}, {SWEEP_SYNTH, -1.f},
				{SWEEP_PAN, 1.f}, {SWEEP_FILTER, 1.f, 0.0, 32.0}, {SWEEP_FILTER, -1.f, 0.0, 32.0}, {SWEEP_SYNTH, 1.f, 0.0, 32.0}, {SWEEP_SYNTH, -1.f, 0.0, 32.0}};
			Sweep w = kinds[step.key(SWEEP) == 9 ? randomInt(9) : step.key(SWEEP)];
			w.start = start;
			if (sweeps[1].target >= 0 && song() >= sweeps[1].start)
				sweeps[0] = sweeps[1];
			sweeps[1] = w;
		}
	}

	/** Works out one slot of the playhead and queues its notes. `start` is in song time. */
	void schedule(const Pattern& pattern, const Settings& s, double start, double length) {
		bool first = slotsLeft <= 0;
		if (first)
			enterStep(pattern, s, start);
		slotsLeft--;
		slots[0] = slots[1];
		slots[1] = {start, length, position, locksOn};
		slotsPlayed++;
		if (!notesOn || (holdSlots && !first))
			return;

		const Step& step = pattern.steps[position];
		auto on = [&](int c) { return componentsOn && step.has(c); };
		int multiply = on(MULTIPLY) && step.key(MULTIPLY) < 8 ? step.key(MULTIPLY) + 1 : 1;
		bool brokenChord = on(MULTIPLY) && step.key(MULTIPLY) == 8;
		bool grid = on(MULTIPLY) && step.key(MULTIPLY) == 9;
		float glide = 0.f;
		if (on(PORTAMENTO)) {
			// Glide 1 to 8, up to a beat; direct; or any of them.
			int k = step.key(PORTAMENTO) == 9 ? randomInt(9) : step.key(PORTAMENTO);
			float g = k < 8 ? (k + 1) / 8.f : 0.f;
			glide = 4.f * g * g;
		}

		for (int r = 0; r < multiply; r++) {
			// Each trigger walks the ramps one degree on; ramp down walks ramp up's degrees back from the top.
			int shift = 0;
			for (int c : {RAMP_UP, RAMP_DOWN, RANDOM}) {
				if (!on(c))
					continue;
				int key = step.key(c), octaves = key < 5 ? 1 : 3, size;
				const int* degrees = rampDegrees(key % 5 + 2, &size);
				auto up = [&](int i) { return degrees[i % size] + 7 * (i / size); };
				int walk = size * octaves + 1, i = c == RANDOM ? randomInt(walk) : ramps[c - RAMP_UP][key]++ % walk;
				shift = c == RAMP_DOWN ? up(walk - 1 - i) - 7 * octaves : up(i);
			}
			for (int i = 0; i < step.count; i++) {
				const Note& note = step.notes[i];
				double at = start + length * (brokenChord ? double(i) / step.count : double(r) / multiply);
				double micro = grid ? 0.0 : ticks(note.offset, length) * (1.f - s.quantize);
				int lengthTicks = note.length ? note.length : s.noteLength;
				double noteLength = holdSlots ? holdSlots * length : lengthTicks == DRONE ? INFINITY : ticks(lengthTicks, length);
				if (multiply > 1)
					noteLength = std::min(noteLength, length / multiply);
				int pitch = std::clamp(shift ? diatonic(note.pitch, shift) : int(note.pitch), 0, 127);
				double time = offset + (s.stepLength > 0 ? swung(at, s.swing) : at) + micro;
				queueNote({std::max(time, now), (pitch - 60) / 12.f, velocity(step, note.velocity), glide, noteLength});
			}
		}
	}

	/** A note's velocity after the step's velocity component: -4 to +3 in sixteenths of the range, mute, or any. */
	float velocity(const Step& step, int v) {
		if (componentsOn && step.has(VELOCITY)) {
			int key = step.key(VELOCITY);
			v = key == 9 ? 1 + randomInt(127) : key == 8 ? 0 : std::clamp(v + 16 * (key - 4), 1, 127);
		}
		return v / 127.f;
	}

	/** Plays a step once now, as the playhead would, to hear it while editing. */
	void preview(const Step& step, const Settings& s) {
		for (int i = 0; i < step.count; i++) {
			const Note& note = step.notes[i];
			int ticks = note.length ? note.length : s.noteLength;
			queueNote({now, (note.pitch - 60) / 12.f, note.velocity / 127.f, 0.f, ticks == DRONE ? INFINITY : Player::ticks(ticks, std::max(s.stepLength, 1))});
		}
	}

	void queueNote(const Event& e) {
		if (queued == QUEUE || e.velocity <= 0.f)
			return;
		int i = queued++;
		for (; i > 0 && queue[i - 1].time > e.time; i--)
			queue[i] = queue[i - 1];
		queue[i] = e;
	}

	/** A channel for a new note: the newest one when it glides, so it slides from the last note; otherwise the one
	silent longest, else the one playing longest. */
	int channelFor(bool glide, const Settings& s) const {
		int n = std::clamp(s.channels, 1, CHANNELS), best = 0;
		for (int i = 1; i < n; i++) {
			const Channel& c = channels[i];
			const Channel& b = channels[best];
			if (glide ? c.id > b.id : c.gate == b.gate ? c.id < b.id : b.gate)
				best = i;
		}
		return best;
	}

	/** Starts a note now for `length` sixteenths and returns its id. Live notes pass INFINITY and end with release(). */
	uint32_t noteOn(float pitch, float velocity, double length, bool live, const Settings& s, float glide = 0.f) {
		Channel& c = channels[channelFor(glide > 0.f, s)];
		c.retrigger = c.gate;
		c.gate = true;
		c.target = pitch;
		if (glide <= 0.f)
			c.pitch = pitch;
		c.glide = glide;
		c.velocity = velocity;
		c.live = live;
		c.off = now + length;
		c.id = ++notes;
		return c.id;
	}

	/** The channel playing note `id`, or null once the note has ended or lost its channel to a newer one. */
	Channel* find(uint32_t id) {
		for (Channel& c : channels)
			if (c.id == id && c.gate)
				return &c;
		return nullptr;
	}

	void release(uint32_t id) {
		if (Channel* c = find(id))
			c->gate = false;
	}
};

} // namespace opz
