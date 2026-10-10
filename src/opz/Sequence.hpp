#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <limits>

/*
An OP-Z track's sequence and the player that runs it, knowing nothing of Rack.

Time is counted in sixteenth notes, the length of a step at step length ×1. A step is split into 24 ticks for micro
timing and note lengths, so those stretch with the step length. The player schedules each step slot one slot ahead,
which covers micro timing from -23 to +24 ticks and still fires every note exactly on time.
*/
namespace opz {

constexpr int STEPS = 16, STEP_NOTES = 4, TICKS = 24, CHANNELS = 16;
constexpr int DRONE = std::numeric_limits<int16_t>::max();

/** The step components, in the order of the white keys that pick them. */
enum Component {
	PULSE, PULSE_HOLD, MULTIPLY, VELOCITY, RAMP_UP, RAMP_DOWN, RANDOM, PORTAMENTO, SWEEP, TONALITY, JUMP,
	PARAMETER_SPARK, COMPONENT_SPARK, TRIGGER_SPARK, COMPONENTS_LEN
};

/** Jump targets, by value key. */
enum Jump { JUMP_START, JUMP_2_4, JUMP_3_4, JUMP_4_4, JUMP_FORWARD, JUMP_BACK, JUMP_RANDOM, JUMP_STAY, JUMP_ALIGN, JUMP_GATE };

struct Note {
	int8_t pitch = 60;      // MIDI note number: 60 is C4, 0V
	uint8_t velocity = 100;
	int8_t offset = 0;      // micro timing, in ticks
	int16_t length = 0;     // in ticks; 0 follows the track's note length
};

/** A component's value is the value key that set it, 0 to 9 for keys 1 to 9 and 0. The sparks add a mode, 0 to 3,
times 16: which pass of each N fires, cycled by pressing the same key again. */
struct Step {
	Note notes[STEP_NOTES];
	int count = 0;
	uint16_t components = 0;
	uint8_t values[COMPONENTS_LEN] = {};

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
	void clearComponent(int c) {
		components &= ~(1 << c);
		values[c] = 0;
	}

	/** Adds a note, or replaces one of the same pitch. A full step ignores it. */
	Note* add(Note n) {
		for (int i = 0; i < count; i++)
			if (notes[i].pitch == n.pitch)
				return &(notes[i] = n);
		return count < STEP_NOTES ? &(notes[count++] = n) : nullptr;
	}
	void remove(int pitch) {
		count = std::remove_if(notes, notes + count, [=](const Note& n) { return n.pitch == pitch; }) - notes;
	}
};

struct Pattern {
	Step steps[STEPS];
};

/** The track settings the player reads every time it schedules a step. */
struct Settings {
	int stepCount = STEPS;
	int stepLength = 1;       // sixteenths per step; 0 advances one step per trigger
	int noteLength = TICKS;   // ticks, or DRONE
	float quantize = 0.f;     // 0 to 1: how far towards the grid micro timing is pulled
	float swing = 0.5f;       // the share of each pair of sixteenths the first one takes, 0.5 to 0.75
	int channels = 4;
};

/** The value keys of the step length setting: ×1 to ×8, ×16, and 0 for one step per trigger. */
inline int stepLengthOfKey(int key) {
	return key == 9 ? 0 : key == 8 ? 16 : key + 1;
}

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
	/** One voice of the poly outputs. */
	struct Channel {
		float pitch = 0.f, target = 0.f;   // V/oct, gliding towards the target
		float glide = 0.f;                 // seconds; 0 jumps straight to the note
		float velocity = 0.f;              // 0 to 1
		bool gate = false;
		bool retrigger = false;            // the gate drops for one sample so the note starts again
		double off = 0.0;                  // when the gate falls, in sixteenths
		uint32_t started = 0;
	};

	struct Event {
		double time;
		float pitch, velocity, glide;
		double length;
	};

	/** A slot is one step's turn: a step holds several with pulse and pulse hold. */
	struct Slot {
		double start = 0.0, length = 1.0;
		int step = -1;
	};

	static constexpr int QUEUE = 128;
	Event queue[QUEUE];
	int queued = 0;
	Channel channels[CHANNELS];
	uint32_t notesStarted = 0;

	double now = 0.0;
	double scheduled = -1.0;   // start of the last slot scheduled, -1 before the first
	double next = 0.0;         // start of the next slot to schedule
	Slot slots[2];             // the slot playing now and the one after it
	int position = -1;         // the step the playhead is on
	int slotsLeft = 0;         // slots the step still holds
	int holdSlots = 0;         // the length of its pulse hold, or 0
	int jump = -1;             // where it sends the playhead next, or -1 to step on
	bool componentsOn = true, notesOn = true;
	long slotsPlayed = 0;
	long passOrigin = 0;       // where the sparks' count of passes starts
	bool timed = true;         // false while steps wait for triggers
	int ramps[3][10] = {};     // how far each ramp setting has walked
	bool gateStep = false;     // a gate step fired: advance the tracks that wait for one
	uint32_t seed = 0x9e3779b9;

	float random() {
		seed ^= seed << 13;
		seed ^= seed >> 17;
		seed ^= seed << 5;
		return seed / 4294967296.f;
	}

	int randomInt(int n) {
		return std::min(int(random() * n), n - 1);
	}

	/** Starts again from the first step, letting go of every note. */
	void start() {
		releaseAll();
		now = 0.0;
		scheduled = -1.0;
		next = 0.0;
		slots[0] = slots[1] = Slot();
		position = -1;
		slotsLeft = 0;
		jump = -1;
		slotsPlayed = 0;
		passOrigin = 0;
		timed = true;
		std::fill(std::begin(ramps[0]), std::end(ramps[2]), 0);
	}

	void releaseAll() {
		for (Channel& c : channels)
			c.gate = false;
		queued = 0;
	}

	/** Runs time forward, scheduling slots one ahead and starting and ending notes on the way. */
	void advance(double sixteenths, Pattern& pattern, const Settings& s) {
		now += sixteenths;
		if (s.stepLength > 0 && !timed) {
			// Back from waiting for triggers: pick the grid up at its next step.
			next = std::ceil(now / s.stepLength) * s.stepLength;
			timed = true;
		}
		while (s.stepLength > 0 && now >= scheduled) {
			schedule(pattern, s, next, s.stepLength);
			scheduled = next;
			next += s.stepLength;
		}
		while (queued > 0 && queue[0].time <= now) {
			play(queue[0], s);
			std::copy(queue + 1, queue + queued, queue);
			queued--;
		}
		for (Channel& c : channels)
			if (c.gate && now >= c.off)
				c.gate = false;
	}

	/** Steps on once, for a track whose step length is 0. */
	void trigger(Pattern& pattern, const Settings& s) {
		timed = false;
		schedule(pattern, s, now, 1);
	}

	/** The step under the playhead now. */
	int playing() const {
		return slots[1].step >= 0 && now >= slots[1].start ? slots[1].step : slots[0].step;
	}

	/** Where a note played now belongs: the nearest step slot, and its offset in ticks. */
	bool locate(int* step, int* ticks) const {
		const Slot& a = slots[0];
		const Slot& b = slots[1];
		const Slot& slot = b.step >= 0 && now - a.start > a.length / 2 ? b : a;
		if (slot.step < 0)
			return false;
		*step = slot.step;
		*ticks = std::clamp((int) std::lround((now - slot.start) / slot.length * TICKS), -TICKS + 1, TICKS);
		return true;
	}

	/** Ticks of a step `length` sixteenths long, in sixteenths. */
	static double ticks(double t, double length) {
		return t * length / TICKS;
	}

	/** Swing delays every second sixteenth; the time between follows, so ratchets and offsets stay in order. */
	double swung(double t, float swing) const {
		double pair = 2.0 * std::floor(t / 2.0), u = t - pair, mid = 2.0 * swing;
		return pair + (u < 1.0 ? u * mid : mid + (u - 1.0) * (2.0 - mid));
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

	int count(const Settings& s) const {
		return std::clamp(s.stepCount, 1, STEPS);
	}

	/** Moves the playhead to the next step, or where the last one's jump sends it. */
	void enterStep(Pattern& pattern, const Settings& s) {
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

		Step& step = pattern.steps[position];
		componentsOn = !step.has(COMPONENT_SPARK) || sparked(step.values[COMPONENT_SPARK], s);
		notesOn = !step.has(TRIGGER_SPARK) || sparked(step.values[TRIGGER_SPARK], s);
		auto on = [&](int c) { return componentsOn && step.has(c); };
		auto amount = [&](int c) { return step.key(c) == 9 ? 1 + randomInt(9) : step.key(c) + 1; };
		holdSlots = on(PULSE_HOLD) ? amount(PULSE_HOLD) : 0;
		slotsLeft = holdSlots ? holdSlots : on(PULSE) ? amount(PULSE) : 1;
		jump = on(JUMP) ? step.key(JUMP) : -1;
		gateStep = gateStep || jump == JUMP_GATE;
	}

	/** Works out one slot of the playhead and queues its notes. */
	void schedule(Pattern& pattern, const Settings& s, double start, double length) {
		bool first = slotsLeft <= 0;
		if (first)
			enterStep(pattern, s);
		slotsLeft--;
		slots[0] = slots[1];
		slots[1] = {start, length, position};
		slotsPlayed++;
		if (!notesOn || (holdSlots && !first))
			return;

		Step& step = pattern.steps[position];
		auto on = [&](int c) { return componentsOn && step.has(c); };
		int multiply = on(MULTIPLY) && step.key(MULTIPLY) < 8 ? step.key(MULTIPLY) + 1 : 1;
		bool brokenChord = on(MULTIPLY) && step.key(MULTIPLY) == 8;
		bool grid = on(MULTIPLY) && step.key(MULTIPLY) == 9;
		float glide = 0.f;
		if (on(PORTAMENTO)) {
			int k = step.key(PORTAMENTO) == 9 ? randomInt(9) : step.key(PORTAMENTO);
			float g = k < 8 ? (k + 1) / 8.f : 0.f;
			glide = g * g;
		}

		for (int r = 0; r < multiply; r++) {
			// Each trigger walks the ramps one degree on; ramp down walks ramp up's degrees backwards from the top.
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
				double offset = grid ? 0.0 : ticks(note.offset, length) * (1.f - s.quantize);
				double noteLength = holdSlots ? holdSlots * length
					: note.length == DRONE || (note.length == 0 && s.noteLength == DRONE) ? INFINITY
					: ticks(note.length ? note.length : s.noteLength, length);
				if (multiply > 1)
					noteLength = std::min(noteLength, length / multiply);
				int pitch = shift ? diatonic(note.pitch, shift) : note.pitch;
				double time = (s.stepLength > 0 ? swung(at, s.swing) : at) + offset;
				queueNote({std::max(time, now), (pitch - 60) / 12.f, velocity(step, note.velocity), glide, noteLength});
			}
		}
	}

	float velocity(const Step& step, int v) {
		if (componentsOn && step.has(VELOCITY)) {
			int key = step.key(VELOCITY);
			v = key == 9 ? 1 + randomInt(127) : key == 8 ? 0 : std::clamp(v + 16 * (key - 4), 1, 127);
		}
		return v / 127.f;
	}

	void queueNote(const Event& e) {
		if (queued == QUEUE || e.velocity <= 0.f)
			return;
		int i = queued++;
		for (; i > 0 && queue[i - 1].time > e.time; i--)
			queue[i] = queue[i - 1];
		queue[i] = e;
	}

	/** A channel for a new note: the newest one when it glides, so it slides from the last note; otherwise a silent
	one, else the one playing longest. */
	int channelFor(bool glide, const Settings& s) {
		int n = std::clamp(s.channels, 1, CHANNELS), best = 0;
		if (glide) {
			for (int i = 1; i < n; i++)
				if (channels[i].started > channels[best].started)
					best = i;
			return best;
		}
		for (int i = 0; i < n; i++)
			if (!channels[i].gate && (channels[best].gate || channels[i].started < channels[best].started))
				best = i;
		if (!channels[best].gate)
			return best;
		for (int i = 1; i < n; i++)
			if (channels[i].started < channels[best].started)
				best = i;
		return best;
	}

	/** Starts a note now, for `length` sixteenths, and returns its channel. Live notes pass INFINITY and end with
	release(). */
	int noteOn(float pitch, float velocity, float glide, double length, const Settings& s) {
		int i = channelFor(glide > 0.f, s);
		Channel& c = channels[i];
		c.retrigger = c.gate;
		c.gate = true;
		c.target = pitch;
		if (glide <= 0.f)
			c.pitch = pitch;
		c.glide = glide;
		c.velocity = velocity;
		c.off = now + length;
		c.started = ++notesStarted;
		return i;
	}

	void release(int channel) {
		channels[channel].gate = false;
	}

	void play(const Event& e, const Settings& s) {
		noteOn(e.pitch, e.velocity, e.glide, e.length, s);
	}
};

} // namespace opz
