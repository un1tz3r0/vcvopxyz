#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <limits>
#include <tuple>

/*
An OP-Z track's sequence and the player that runs it, knowing nothing of Rack.

Time is counted in sixteenth notes, the length of a step at step length ×1. The player's clock runs all the time,
for notes played live; the sequence's own time, song time, stands still while it's stopped, so it can carry on. A
step is split into 24 ticks for micro timing and note lengths, so those stretch with the step length. The player
schedules each step slot one slot ahead, which covers micro timing from -23 to +24 ticks and still starts every note
exactly on time. A slot is one step's turn under the playhead: pulse and pulse hold give a step several. A step's
parameter locks hold its engine's dials for as long as its slots are under the playhead.

Notes are in the key the Sequencer finds in the synth tracks (Scale), and a note is moved as the master track says
when it starts rather than when it's scheduled, so it follows a master note starting with it (Tonality).

The arp track's player can arpeggiate (Arp): its channels then hold the notes, and the arp plays them one at a time
on channels of its own, in key, on a grid of song time.
*/
namespace opz {

constexpr int STEPS = 16, STEP_NOTES = 4, TICKS = 24, CHANNELS = 16;
constexpr int TRACKS = 8, DRUM_TRACKS = 4, SYNTH_TRACKS = TRACKS - DRUM_TRACKS; // kick, snare, perc, sample, then bass, lead, arp, chord
constexpr int MASTER = TRACKS;             // the master track, sequenced after the eight it transposes
constexpr int FX1 = MASTER + 1, FX_TRACKS = 2; // then the two FX tracks, which play the send effects
constexpr int LANES = FX1 + FX_TRACKS;
constexpr int ARP_TRACK = 6;

/** A lane's number on the OP-Z: 1 to 8 for the tracks, 9 and 10 for the FX tracks, 12 for the master track. */
constexpr int trackNumber(int lane) {
	return lane < TRACKS ? lane + 1 : lane == MASTER ? 12 : lane;
}
constexpr int MIN_OFFSET = 1 - TICKS, MAX_OFFSET = TICKS;
constexpr int DRONE = std::numeric_limits<int16_t>::max();
constexpr int SOUND_PARAMS = 22; // a synth track's dials, every page's, the track settings' and the arp's, which a step can lock
constexpr int ARP_PARAMS = 18;   // the first of the arp's four: speed, pattern, style and range

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

/** The arp's patterns, by its blue dial. */
enum ArpPattern { ARP_PLAYED, ARP_UP, ARP_DOWN, ARP_UP_DOWN, ARP_DOWN_UP, ARP_RANDOM, ARP_PATTERNS };
constexpr int ARP_SPEEDS = 8, ARP_STYLES = 6;
constexpr double ARP_PERIODS[ARP_SPEEDS] = {4.0, 2.0, 4.0 / 3, 1.0, 2.0 / 3, 0.5, 1.0 / 3, 0.25}; // 1/4 to 1/64, in sixteenths
/** Each style's beats in turn: H plays a note at its velocity, M at three quarters of it, L at half, and - rests. */
constexpr const char* ARP_RHYTHMS[ARP_STYLES] = {"H", "LHLLHLLH", "HLLHLLHL", "LLHLLHLL", "LMH", "LL-"};

/** The arp track's arpeggiator, as its engine's dials set it. */
struct Arp {
	int speed = -1;            // 1 to 8 for ARP_PERIODS, 0 off, and -1 on a track with no arp
	int pattern = ARP_UP, style = 0;
	float range = 0.f;         // 0 to 1, over 1 to 4 octaves

	int octaves() const {
		return 1 + std::min(int(range * 4.f), 3);
	}

	/** With a step's locks on the arp's dials, as the engine's dials would take them. */
	void lock(const Step& step) {
		auto locked = [&](int i, int top, int* v) {
			if (step.locked(ARP_PARAMS + i))
				*v = (int) std::lround(step.lockValues[ARP_PARAMS + i] * top);
		};
		locked(0, ARP_SPEEDS, &speed);
		locked(1, ARP_PATTERNS - 1, &pattern);
		locked(2, ARP_STYLES - 1, &style);
		if (step.locked(ARP_PARAMS + 3))
			range = step.lockValues[ARP_PARAMS + 3];
	}

	/** Which of `m` notes, low to high or as played, the arp's `k`th note plays, but for random. */
	static int pick(int pattern, int m, long k) {
		if (pattern == ARP_DOWN)
			return m - 1 - int(k % m);
		if (pattern == ARP_UP_DOWN || pattern == ARP_DOWN_UP) {
			if (m < 2)
				return 0;
			int i = int(k % (2 * m - 2)), up = i < m ? i : 2 * m - 2 - i;
			return pattern == ARP_UP_DOWN ? up : m - 1 - up;
		}
		return int(k % m);
	}
};

/** The track settings the player reads every time it schedules a step. */
struct Settings {
	int stepCount = STEPS;
	int stepLength = 1;       // sixteenths per step; 0 waits for a trigger
	int noteLength = TICKS;   // ticks, or DRONE
	float quantize = 0.f;     // 0 to 1: how far micro timing is pulled to the grid
	float swing = 0.5f;       // the share of each pair of sixteenths the first one takes, 0.5 to 0.75
	int channels = 4;
	Arp arp;
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

inline int mod(int a, int b) {
	return a - b * floorDiv(a, b);
}

/** The major scale, in semitones above its key. */
constexpr int MAJOR[7] = {0, 2, 4, 5, 7, 9, 11};

/** A key and mode, as the master track has them: the major scale on `key`, a pitch class from 0 for C, played from
its degree `mode`, 0 for Ionian (major) to 6 for Locrian, as the tonic. The modes share the major scale's notes, so
only the tonic tells them apart. */
struct Scale {
	int key = 0, mode = 0;

	int tonic() const {
		return (key + MAJOR[mode]) % 12;
	}

	/** Where a pitch is on the scale, in degrees up from its key's lowest octave; a pitch outside it, the degree below. */
	int degree(int pitch) const {
		int p = pitch - key, octave = floorDiv(p, 12), d = 6;
		while (MAJOR[d] > p - 12 * octave)
			d--;
		return 7 * octave + d;
	}

	int pitch(int degree) const {
		int octave = floorDiv(degree, 7);
		return key + 12 * octave + MAJOR[degree - 7 * octave];
	}

	bool contains(int p) const {
		return pitch(degree(p)) == p;
	}

	/** Moves a pitch by whole degrees. One outside the scale keeps its distance from the degree below it. */
	int shift(int p, int degrees) const {
		int d = degree(p);
		return pitch(d + degrees) + p - pitch(d);
	}

	/** The nearest pitch, the lower of two, on the degrees of the bit mask `chord`, counted from the tonic: 0x7f is
	the whole scale, 0x15 its triad, 0x11 its root and fifth. */
	int snap(int p, int chord) const {
		for (int distance = 0; distance < 12; distance++)
			for (int q : {p - distance, p + distance})
				if (contains(q) && chord >> mod(degree(q) - mode, 7) & 1)
					return q;
		return p;
	}
};

/** The key and mode that fit some notes best, given by pitch class as weights for the scale and for the tonic: the
scale holding the most weight, on the tonic with the most, then major, minor and the other modes in turn. No notes
at all is C major. */
inline Scale detect(const int notes[12], const int tonics[12]) {
	static const int preference[7] = {6, 3, 1, 2, 4, 5, 0}; // by mode: Ionian, Aeolian, Mixolydian, Dorian, Lydian, Phrygian, Locrian
	Scale best;
	std::tuple<int, int, int> top(-1, -1, -1);
	for (int tonic = 0; tonic < 12; tonic++)
		for (int mode = 0; mode < 7; mode++) {
			Scale s{mod(tonic - MAJOR[mode], 12), mode};
			int fit = 0;
			for (int d : MAJOR)
				fit += notes[(s.key + d) % 12];
			std::tuple<int, int, int> score(fit, tonics[tonic], preference[mode]);
			if (score > top) {
				top = score;
				best = s;
			}
		}
	return best;
}

/** How a track's notes play in key: the scale, and the master track's move from it, by degrees of the scale and then
by semitones. */
struct Tonality {
	Scale scale;
	int degrees = 0, semitones = 0;

	/** The key and mode after the move. */
	Scale moved() const {
		return {mod(scale.key + semitones, 12), mod(scale.mode + degrees, 7)};
	}

	/** A sequenced note's pitch as it starts: moved, then as its step's tonality component says (its value key, or -1
	for none): ignore the master track, follow its root in semitones alone, an octave, fifth or third up, a semitone
	up or down, or pulled onto the scale, the chord, or the chord's root and fifth. */
	int apply(int pitch, int component) const {
		int d = degrees, s = semitones, tonic = 60 + scale.tonic();
		switch (component) {
			case 0: return pitch;
			case 1: return pitch + scale.shift(tonic, d) - tonic + s;
			case 2: s += 12; break;
			case 3: d += 4; break;
			case 4: d += 2; break;
			case 5: s++; break;
			case 6: s--; break;
		}
		static const int chords[3] = {0x7f, 0x15, 0x11};
		int p = scale.shift(pitch, d) + s;
		return component >= 7 ? moved().snap(p, chords[component - 7]) : p;
	}
};

/** The move a note on the master track makes, from the tonic nearest middle C: a note in the scale moves by its
degrees, which changes the mode, and one outside it by its semitones, which changes the key. */
inline Tonality masterMove(Scale scale, int pitch) {
	int home = 54 + mod(scale.tonic() - 6, 12); // F#3 to F4
	Tonality t{scale};
	if (scale.contains(pitch))
		t.degrees = scale.degree(pitch) - scale.degree(home);
	else
		t.semitones = pitch - home;
	return t;
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
		int midi = -1, tonality = -1; // a sequenced note's Event pitch and tonality, for the arp to move as it plays
		long slot = 0;             // the slot a sequenced note came from, counted as slotsPlayed does; 0 for live ones
	};

	struct Event {
		double time;
		int pitch;                 // MIDI note number, before the master track moves it
		float velocity, glide;
		double length;
		int tonality;              // the step's tonality component, or -1
		long slot = 0;             // the slot it came from
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

	/** Where the scheduler stood just before it scheduled a slot: what redo() goes back to. */
	struct Mark {
		double start = 0.0;    // the slot's start, in song time
		double scheduled, next;
		Slot slots[2];
		int position, slotsLeft, holdSlots, jump;
		bool componentsOn, notesOn, locksOn;
		long slotsPlayed, passOrigin;
		int ramps[3][10];
		Sweep sweeps[2];
		uint32_t seed;
	};

	static constexpr int QUEUE = 128, MARKS = 4;
	Event queue[QUEUE];
	int queued = 0;
	Channel channels[CHANNELS];
	int used = 0;              // how many channels, from the first, notes have played on: the rest stay idle
	uint32_t notes = 0;

	bool running = false;
	double now = 0.0;
	double offset = 0.0;       // when song time 0 was, by now's clock
	double paused = 0.0;       // the song time it stopped at
	double scheduled = -1.0;   // start of the last slot scheduled, in song time, -1 before the first
	double next = 0.0;         // start of the slot to schedule after it
	double hold = INFINITY;    // song time no slot may start at or after yet: where a chain's next pattern comes in
	Mark marks[MARKS];         // before each of the last slots scheduled, oldest first
	int marked = 0;
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
	Tonality tonality;         // the key, and the master track's move, which the Sequencer keeps up to date

	// The arp: with it on, the channels hold the notes, and it plays them on its own.
	Arp arp;                   // as it plays now: the track's, with the locks of the step under the playhead
	Channel arped[CHANNELS];
	bool arping = false;       // notes are held, and the arp's grid runs
	bool arpRunning = false;   // the grid is in song time, or by the clock while stopped
	double arpPeriod = 0.0, arpOrigin = 0.0; // while stopped, the grid starts with the first note held
	double arpFrom = 0.0, arpDue = 0.0;       // when the last tick was, and the next is due
	long arpTick = 0, arpNotes = 0, arpBeats = 0; // the last grid tick; notes and beats since the notes were first held

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
		hold = INFINITY;
		marked = 0;
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

	/** Starts the pattern over at song time `at`, from its first step: a chain's next pattern comes in. A track on a
	grid that doesn't meet `at` steps onto it there. */
	void restartPattern(double at) {
		position = -1;
		slotsLeft = holdSlots = 0;
		jump = -1;
		passOrigin = slotsPlayed;
		if (timed)
			next = at;
		hold = INFINITY;
	}

	/** Lets go of the notes the sequence is playing or has queued, and keeps going. */
	void silence() {
		queued = 0;
		for (Channel& c : channels)
			if (!c.live)
				c.gate = false;
		for (Channel& c : arped)
			c.gate = false;
		arping = false;
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
			while (song() >= scheduled && next < hold) {
				schedule(pattern, s, next, s.stepLength);
				scheduled = next;
				next += s.stepLength;
			}
		}
		arp = s.arp;
		if (arp.speed >= 0 && locking() >= 0)
			arp.lock(pattern.steps[locking()]);
		while (queued > 0 && queue[0].time <= now) {
			Event e = queue[0];
			std::copy(queue + 1, queue + queued, queue);
			queued--;
			noteOn(volts(e.pitch, e.tonality), e.velocity, e.time + e.length, false, s, e.glide, e.pitch, e.tonality, e.slot);
		}
		play(channels, used, sixteenths);
		if (arp.speed > 0)
			arpeggiate(s, sixteenths);
		else if (arping)
			silenceArp();
	}

	/** A sequenced note's pitch in V/oct, moved as the master track says. */
	float volts(int pitch, int tonality) const {
		return (std::clamp(this->tonality.apply(pitch, tonality), 0, 127) - 60) / 12.f;
	}

	/** Ends and glides the notes of `n` channels. */
	void play(Channel* cs, int n, double sixteenths) {
		for (int i = 0; i < n; i++) {
			Channel& c = cs[i];
			if (c.gate && now >= c.off)
				c.gate = false;
			c.pitch += (c.target - c.pitch) * (c.glide > 0.f ? float(std::min(sixteenths / c.glide, 1.0)) : 1.f);
		}
	}

	/** What the engine plays: the channels, or with the arp on, the arp's. */
	const Channel* voices() const {
		return arp.speed > 0 ? arped : channels;
	}

	void silenceArp() {
		for (Channel& c : arped)
			c.gate = false;
		arping = false;
	}

	/** Plays the held notes one at a time: the first as soon as they're held, then on each tick of the arp's grid, in
	its pattern over its octaves and in its style's rhythm. A held sequenced note follows the master track's moves. The
	grid is song time while running, swung like the steps, and while stopped, starts with the first note. */
	void arpeggiate(const Settings& s, double sixteenths) {
		const Arp& a = arp;
		int n = polyphony(s);
		play(arped, n, sixteenths);
		int order[CHANNELS], held = 0;
		for (int i = 0; i < used; i++)
			if (channels[i].gate)
				order[held++] = i;
		if (!held) {
			arping = false;
			return;
		}

		bool fire = !arping;
		if (!arping) {
			arpNotes = arpBeats = 0;
			arpOrigin = now;
		}
		double period = ARP_PERIODS[a.speed - 1], clock = running ? song() : now - arpOrigin;
		bool swing = running && s.stepLength > 0;
		auto due = [&](long k) { return swing ? swung(k * period, s.swing) : k * period; };
		if (!arping || period != arpPeriod || running != arpRunning || clock < arpFrom) {
			arping = true;
			arpRunning = running;
			arpPeriod = period;
			for (arpTick = (long) std::floor(clock / period); due(arpTick) > clock; arpTick--) {}
			arpFrom = due(arpTick);
			arpDue = due(arpTick + 1);
		}
		for (; clock >= arpDue; arpDue = due(++arpTick + 1)) {
			arpFrom = arpDue;
			fire = true;
		}
		const char* rhythm = ARP_RHYTHMS[a.style];
		char beat = fire ? rhythm[arpBeats++ % std::strlen(rhythm)] : '-';
		if (beat == '-')
			return;

		auto pitch = [&](const Channel& c) { return c.midi >= 0 ? volts(c.midi, c.tonality) : c.target; };
		auto before = [&](int i, int j) {
			const Channel &x = channels[i], &y = channels[j];
			return a.pattern == ARP_PLAYED || pitch(x) == pitch(y) ? x.id < y.id : pitch(x) < pitch(y);
		};
		std::sort(order, order + held, before);
		int m = held * a.octaves(), k = a.pattern == ARP_RANDOM ? randomInt(m) : Arp::pick(a.pattern, m, arpNotes++);
		const Channel& h = channels[order[k % held]];
		Channel& c = arped[freeChannel(arped, n, h.glide > 0.f)];
		c.retrigger = c.gate;
		c.gate = true;
		c.target = pitch(h) + k / held;
		if (h.glide <= 0.f)
			c.pitch = c.target;
		c.glide = h.glide;
		c.velocity = h.velocity * (beat == 'H' ? 1.f : beat == 'M' ? 0.75f : 0.5f);
		c.live = h.live;
		c.off = now + period / 2;
		c.id = ++notes;
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
		mark(start);
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
			// Each trigger walks the ramps one degree of the key on; ramp down walks ramp up's degrees back from the top.
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
				int pitch = std::clamp(tonality.scale.shift(note.pitch, shift), 0, 127);
				double time = offset + (s.stepLength > 0 ? swung(at, s.swing) : at) + micro;
				queueNote({std::max(time, now), pitch, velocity(step, note.velocity), glide, noteLength, on(TONALITY) ? step.key(TONALITY) : -1, slotsPlayed});
			}
		}
	}

	/** Notes where the scheduler stands before it schedules the slot at `start`. */
	void mark(double start) {
		if (marked == MARKS)
			std::copy(marks + 1, marks + MARKS, marks);
		else
			marked++;
		Mark& m = marks[marked - 1];
		m.start = start;
		m.scheduled = scheduled;
		m.next = next;
		std::copy(slots, slots + 2, m.slots);
		m.position = position;
		m.slotsLeft = slotsLeft;
		m.holdSlots = holdSlots;
		m.jump = jump;
		m.componentsOn = componentsOn;
		m.notesOn = notesOn;
		m.locksOn = locksOn;
		m.slotsPlayed = slotsPlayed;
		m.passOrigin = passOrigin;
		std::copy(&ramps[0][0], &ramps[0][0] + 30, &m.ramps[0][0]);
		std::copy(sweeps, sweeps + 2, m.sweeps);
		m.seed = seed;
	}

	/** Takes back the slots scheduled to start at or after song time `at`, so the next advance schedules them again,
	from the pattern as it is then: their queued notes go, and those already started stop. A switch of pattern does
	this, so the step it is late for, or the one queued ahead, plays from the new one. False if no slot starts that
	late. */
	bool redo(double at) {
		int i = 0;
		while (i < marked && marks[i].start < at - 1e-9)
			i++;
		if (i == marked)
			return false;
		const Mark& m = marks[i];
		long cut = m.slotsPlayed;
		queued = int(std::remove_if(queue, queue + queued, [=](const Event& e) { return e.slot > cut; }) - queue);
		for (Channel& c : channels)
			if (!c.live && c.slot > cut)
				c.gate = false;
		scheduled = m.scheduled;
		next = m.next;
		std::copy(m.slots, m.slots + 2, slots);
		position = m.position;
		slotsLeft = m.slotsLeft;
		holdSlots = m.holdSlots;
		jump = m.jump;
		componentsOn = m.componentsOn;
		notesOn = m.notesOn;
		locksOn = m.locksOn;
		slotsPlayed = m.slotsPlayed;
		passOrigin = m.passOrigin;
		std::copy(&m.ramps[0][0], &m.ramps[0][0] + 30, &ramps[0][0]);
		std::copy(m.sweeps, m.sweeps + 2, sweeps);
		seed = m.seed;
		marked = i;
		return true;
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
			queueNote({now, note.pitch, note.velocity / 127.f, 0.f, ticks == DRONE ? INFINITY : Player::ticks(ticks, std::max(s.stepLength, 1)), -1});
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

	static int polyphony(const Settings& s) {
		return std::clamp(s.channels, 1, CHANNELS);
	}

	/** One of `n` channels for a new note: the newest one when it glides, so it slides from the last note; otherwise
	the one silent longest, else the one playing longest. */
	static int freeChannel(const Channel* cs, int n, bool glide) {
		int best = 0;
		for (int i = 1; i < n; i++) {
			const Channel& c = cs[i];
			const Channel& b = cs[best];
			if (glide ? c.id > b.id : c.gate == b.gate ? c.id < b.id : b.gate)
				best = i;
		}
		return best;
	}

	/** A channel for a new note. With the arp on, every channel can hold one for it to play, the first silent one
	first, which keeps the channels in use few. */
	int channelFor(bool glide, const Settings& s) const {
		if (arp.speed <= 0)
			return freeChannel(channels, polyphony(s), glide);
		for (int i = 0; i < CHANNELS; i++)
			if (!channels[i].gate)
				return i;
		return freeChannel(channels, CHANNELS, false);
	}

	/** Starts a note now, to end at `off` by now's clock, and returns its id. A sequenced note ends its length after its
	time, not after the sample it started in, so one a step long is over as the next step's notes start. Live notes
	pass INFINITY and end with release(). */
	uint32_t noteOn(float pitch, float velocity, double off, bool live, const Settings& s, float glide = 0.f, int midi = -1, int tonality = -1, long slot = 0) {
		int i = channelFor(glide > 0.f, s);
		used = std::max(used, i + 1);
		Channel& c = channels[i];
		c.retrigger = c.gate;
		c.gate = true;
		c.target = pitch;
		if (glide <= 0.f)
			c.pitch = pitch;
		c.glide = glide;
		c.velocity = velocity;
		c.live = live;
		c.off = off;
		c.id = ++notes;
		c.midi = midi;
		c.tonality = tonality;
		c.slot = slot;
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
