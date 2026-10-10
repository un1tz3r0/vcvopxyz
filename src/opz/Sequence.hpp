#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

/*
An OP-Z track's sequence and the player that runs it, knowing nothing of Rack.

Time is counted in sixteenth notes, the length of a step at step length ×1. A step is split into 24 ticks for micro
timing and note lengths, so those stretch with the step length. The player schedules each step one step ahead,
which covers micro timing from -23 to +24 ticks and still starts every note exactly on time.
*/
namespace opz {

constexpr int STEPS = 16, STEP_NOTES = 4, TICKS = 24, CHANNELS = 16;
constexpr int MIN_OFFSET = 1 - TICKS, MAX_OFFSET = TICKS;
constexpr int DRONE = std::numeric_limits<int16_t>::max();

struct Note {
	int8_t pitch = 60;      // MIDI note number: 60 is C4, 0V
	uint8_t velocity = 100;
	int8_t offset = 0;      // micro timing, in ticks
	int16_t length = 0;     // in ticks, or DRONE; 0 follows the track's note length
};

struct Step {
	Note notes[STEP_NOTES];
	int count = 0;

	bool empty() const {
		return count == 0;
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

	void clear() {
		count = 0;
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
			s.clear();
	}
};

inline bool operator==(const Note& a, const Note& b) {
	return a.pitch == b.pitch && a.velocity == b.velocity && a.offset == b.offset && a.length == b.length;
}
inline bool operator==(const Step& a, const Step& b) {
	return a.count == b.count && std::equal(a.notes, a.notes + a.count, b.notes);
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


struct Player {
	/** One channel of the poly outputs. */
	struct Channel {
		float pitch = 0.f;         // V/oct
		float velocity = 0.f;      // 0 to 1
		bool gate = false;
		bool retrigger = false;    // the gate drops for a sample so a note on a busy channel starts afresh
		bool live = false;         // played from the keys or inputs, rather than by the sequence
		double off = 0.0;          // when the gate falls, in sixteenths
		uint32_t id = 0;           // which note it is playing; ids count up, so lower is older
	};

	struct Event {
		double time;
		float pitch, velocity;
		double length;
	};

	/** One step's turn under the playhead. */
	struct Slot {
		double start = 0.0, length = 1.0;
		int step = -1;
	};

	static constexpr int QUEUE = 64;
	Event queue[QUEUE];
	int queued = 0;
	Channel channels[CHANNELS];
	uint32_t notes = 0;

	bool running = false;
	double now = 0.0;
	double scheduled = -1.0;   // start of the last slot scheduled, -1 before the first
	double next = 0.0;         // start of the slot to schedule after it
	Slot slots[2];             // the slot playing and the one after it
	int position = -1;         // the step the playhead is on
	long slotsPlayed = 0;
	bool timed = true;         // false while steps wait for triggers

	/** Starts from the first step, letting go of the notes the sequence was playing. */
	void start() {
		stop();
		running = true;
		now = 0.0;
		scheduled = -1.0;
		next = 0.0;
		slots[0] = slots[1] = Slot();
		position = -1;
		slotsPlayed = 0;
		timed = true;
	}

	void stop() {
		running = false;
		silence();
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

	/** Runs time forward: schedules steps one ahead while running, and starts and ends notes on the way. */
	void advance(double sixteenths, const Pattern& pattern, const Settings& s) {
		now += sixteenths;
		if (running && s.stepLength > 0) {
			if (!timed) {
				// Back from waiting for triggers: pick the grid up at its next step.
				next = std::ceil(now / s.stepLength) * s.stepLength;
				timed = true;
			}
			while (now >= scheduled) {
				schedule(pattern, s, next, s.stepLength);
				scheduled = next;
				next += s.stepLength;
			}
		}
		while (queued > 0 && queue[0].time <= now) {
			Event e = queue[0];
			std::copy(queue + 1, queue + queued, queue);
			queued--;
			noteOn(e.pitch, e.velocity, e.length, false, s);
		}
		for (Channel& c : channels)
			if (c.gate && now >= c.off)
				c.gate = false;
	}

	/** Steps on once, for a track whose step length is 0. */
	void trigger(const Pattern& pattern, const Settings& s) {
		if (!running)
			return;
		timed = false;
		schedule(pattern, s, now, 1.0);
	}

	/** The step under the playhead now, or -1. */
	int playing() const {
		return slots[1].step >= 0 && now >= slots[1].start ? slots[1].step : slots[0].step;
	}

	/** Where a note played now belongs: the nearest step, and its micro timing in ticks. */
	bool locate(int* step, int* ticks) const {
		const Slot& a = slots[0];
		const Slot& b = slots[1];
		const Slot& slot = b.step >= 0 && (a.step < 0 || now - a.start > a.length / 2) ? b : a;
		if (slot.step < 0)
			return false;
		*step = slot.step;
		*ticks = std::clamp((int) std::lround((now - slot.start) / slot.length * TICKS), MIN_OFFSET, MAX_OFFSET);
		return true;
	}

	/** The length of the step playing, in sixteenths. */
	double stepLength() const {
		return slots[0].step >= 0 ? slots[0].length : slots[1].length;
	}

	/** Ticks of a step `length` sixteenths long, in sixteenths. */
	static double ticks(double t, double length) {
		return t * length / TICKS;
	}

	/** Swing delays every second sixteenth; the time between follows it, so offsets stay in order. */
	static double swung(double t, float swing) {
		double pair = 2.0 * std::floor(t / 2.0), u = t - pair, mid = 2.0 * swing;
		return pair + (u < 1.0 ? u * mid : mid + (u - 1.0) * (2.0 - mid));
	}

	/** Works out where the playhead goes next and queues that step's notes. */
	void schedule(const Pattern& pattern, const Settings& s, double start, double length) {
		position = (position + 1) % std::clamp(s.stepCount, 1, STEPS);
		slots[0] = slots[1];
		slots[1] = {start, length, position};
		slotsPlayed++;
		const Step& step = pattern.steps[position];
		for (int i = 0; i < step.count; i++) {
			const Note& note = step.notes[i];
			int ticks = note.length ? note.length : s.noteLength;
			double time = (s.stepLength > 0 ? swung(start, s.swing) : start) + Player::ticks(note.offset, length) * (1.f - s.quantize);
			queueNote({std::max(time, now), (note.pitch - 60) / 12.f, note.velocity / 127.f, ticks == DRONE ? INFINITY : Player::ticks(ticks, length)});
		}
	}

	/** Plays a step once now, as the playhead would, to hear it while editing. */
	void preview(const Step& step, const Settings& s) {
		for (int i = 0; i < step.count; i++) {
			const Note& note = step.notes[i];
			int ticks = note.length ? note.length : s.noteLength;
			queueNote({now, (note.pitch - 60) / 12.f, note.velocity / 127.f, ticks == DRONE ? INFINITY : Player::ticks(ticks, std::max(s.stepLength, 1))});
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

	/** A channel for a new note: the one silent longest, else the one playing longest. */
	int channelFor(const Settings& s) const {
		int n = std::clamp(s.channels, 1, CHANNELS), best = 0;
		for (int i = 1; i < n; i++) {
			const Channel& c = channels[i];
			const Channel& b = channels[best];
			if (c.gate == b.gate ? c.id < b.id : b.gate)
				best = i;
		}
		return best;
	}

	/** Starts a note now for `length` sixteenths and returns its id. Live notes pass INFINITY and end with release(). */
	uint32_t noteOn(float pitch, float velocity, double length, bool live, const Settings& s) {
		Channel& c = channels[channelFor(s)];
		c.retrigger = c.gate;
		c.gate = true;
		c.pitch = pitch;
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
