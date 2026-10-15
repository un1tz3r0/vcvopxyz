#pragma once
#include "Sequence.hpp"
#include <cstring>
#include <string>
#include <vector>

/*
A project: everything the Sequencer keeps in one of its ten project slots, as the OP-Z does, and the OP-Z's own
project file (projects/project01.opz to project10.opz on its disk), knowing nothing of Rack.

The file's layout comes from reverse engineering: lrk's z-po-project wiki (firmware 1.1.17) and kmorrill's
op-z-sysex (1.2.45 on, tested on the device). Both agree on the offsets. It is little-endian, uncompressed and
always 342844 bytes: a 4-byte header, a 568-byte block of project settings, then 16 patterns of 21392 bytes. Only
the first pattern of the chain playing is read and written, since the Sequencer has one, for the eight tracks and
the master track. Nobody has decoded the track settings' bytes, so those encodings are guesses, marked below. Everything else a file holds (the other
patterns, mutes, the drum tracks' plugs) is kept from the file a project was imported from, so a round trip changes
only what a project models.
*/
namespace opz {

constexpr int PROJECTS = 10, MUTE_GROUPS = 10;
enum Level { DRUM_LEVEL, SYNTH_LEVEL, PUNCH, MASTER_LEVEL, LEVELS_LEN };

struct Project {
	struct Part {
		Pattern pattern;
		int stepCount = STEPS;
		int stepLength = 0;             // the value key that set it: 0 to 7 for ×1 to ×8, 8 for ×16, 9 one per trigger
		int noteLength = 4;             // an index into NOTE_LENGTHS
		float quantize = 0.f;
		int channels = 1;
		bool hasSound = false;          // whether it holds an engine's dials
		float sound[SOUND_PARAMS] = {}; // the synth track's dials in its own param order, 0 to 1 over each one's range
	};
	Part parts[LANES];                  // the eight tracks', then the master track's, which has no sound
	int masterStyle = 0;                // the master track's note style: 0 latch, 1 free
	uint16_t transposed = 0xf0;         // the tracks it moves, a bit each: the synth tracks to begin with
	float tempo = 120.f, swing = 0.5f;
	float levels[LEVELS_LEN] = {1.f, 1.f, 0.f, 1.f};
	int metronomeSound = 0;
	float metronomeLevel = 0.f;
	uint16_t mutes[MUTE_GROUPS] = {}, audioMutes[MUTE_GROUPS] = {};
	int muteGroup = 0;
	bool used = false;                  // saved or imported at least once
	std::vector<uint8_t> opz;           // the file it came from, for everything it doesn't model
};


namespace opzfile {

constexpr size_t SIZE = 342844, PATTERNS = 572, PATTERN = 21392;
constexpr size_t LIVE_CHAIN = 484, LEVELS = 516, TEMPO = 520, SWING = 565, METRONOME_LEVEL = 566, METRONOME_SOUND = 567, CHAIN = 568;
constexpr size_t TRACK_CHUNKS = 0, NOTES = 192, STEP_CHUNKS = 7232, SOUNDS = 21056;
constexpr int SLOTS = 55, UNITS = 6200;      // note slots per step; note duration units per step
constexpr int HEADER = 0x49;
/** Each lane's track among a file's 16: the eight in order, then the master track, the OP-Z's 12th. */
constexpr int FILE_TRACK[LANES] = {0, 1, 2, 3, 4, 5, 6, 7, 11};
/** Where each lane's note slots start among a step's 55, and how many it has. */
constexpr int SLOT_OFFSET[LANES] = {0, 2, 4, 6, 8, 12, 16, 24, 31}, SLOT_COUNT[LANES] = {2, 2, 2, 2, 4, 4, 8, 4, 4};
/** The components of the step chunk's bits, from the lowest. */
constexpr int COMPONENT_BITS[COMPONENTS_LEN] = {PULSE, PARAMETER_SPARK, JUMP, VELOCITY, RAMP_UP, RAMP_DOWN, RANDOM,
	PORTAMENTO, MULTIPLY, SWEEP, PULSE_HOLD, TONALITY, TRIGGER_SPARK, COMPONENT_SPARK};
/** The file's 18 sound parameters, as a synth track's params: P1, P2, the envelope, the sends, the filter and
resonance, pan, level, portamento, the LFO's depth, speed, target and shape, and the note style. */
constexpr int FILE_SOUNDS = 18;
constexpr int SOUND_ORDER[FILE_SOUNDS] = {0, 1, 4, 5, 6, 7, 12, 13, 2, 3, 14, 15, 17, 8, 9, 10, 11, 16};
constexpr int LFO_DIALS = 8;  // the synth track's first LFO param

/** A file sound parameter's param on lane `t`. The arp track has no LFO: its file keeps the arp's dials there. */
inline int soundParam(int t, int i) {
	int p = SOUND_ORDER[i];
	return t == ARP_TRACK && p >= LFO_DIALS && p < LFO_DIALS + 4 ? p - LFO_DIALS + ARP_PARAMS : p;
}
/** The default engines' plugs on a new file's synth tracks (lrk's table), and the 0x05 nobody has explained. */
constexpr uint32_t PLUGS[TRACKS] = {0, 0, 0, 0, 26, 65, 26, 31};

inline uint32_t get(const std::vector<uint8_t>& f, size_t at, int bytes) {
	uint32_t v = 0;
	for (int i = bytes - 1; i >= 0; i--)
		v = v << 8 | f[at + i];
	return v;
}

inline void put(std::vector<uint8_t>& f, size_t at, int bytes, uint32_t v) {
	for (int i = 0; i < bytes; i++, v >>= 8)
		f[at + i] = v & 0xff;
}

inline uint8_t byte(float x) {
	return (uint8_t) std::lround(std::clamp(x, 0.f, 1.f) * 255.f);
}

/** The pattern a file is on: the first of the chain playing, or the first one. */
inline size_t pattern(const std::vector<uint8_t>& f) {
	return PATTERNS + PATTERN * (f[LIVE_CHAIN] < 16 ? f[LIVE_CHAIN] : 0);
}

/** A file with no notes, no components, default settings and the default engines. */
inline std::vector<uint8_t> blank() {
	std::vector<uint8_t> f(SIZE, 0);
	put(f, 0, 4, HEADER);
	std::fill(f.begin() + 4, f.begin() + LEVELS, 0xff);    // no chains
	f[LIVE_CHAIN] = 0;
	f[CHAIN] = 0xff;
	put(f, TEMPO, 2, 120);
	for (int l = 0; l < 4; l++)
		f[LEVELS + l] = l == PUNCH ? 0 : 0xff;
	for (int p = 0; p < 16; p++) {
		size_t at = PATTERNS + PATTERN * p;
		for (int s = 0; s < STEPS * SLOTS; s++)
			f[at + NOTES + 8 * s + 4] = 0xff;
		for (int t = 0; t < 16; t++) {
			size_t track = at + TRACK_CHUNKS + 12 * t;
			put(f, track, 4, t < TRACKS ? PLUGS[t] : 0);
			f[track + 4] = STEPS;
			f[track + 5] = 0x05;
			f[track + 6] = 1;
			std::fill(f.begin() + at + SOUNDS + 18 * t, f.begin() + at + SOUNDS + 18 * (t + 1), 0x80);
		}
	}
	return f;
}

/** Reads a file into a project, or says why it can't. */
inline bool read(const std::vector<uint8_t>& f, Project& p, std::string* error) {
	if (f.size() != SIZE) {
		*error = "Not an OP-Z project: it should be " + std::to_string(SIZE) + " bytes, and is " + std::to_string(f.size());
		return false;
	}
	size_t at = pattern(f);
	uint32_t tempo = get(f, TEMPO, 2);
	p.tempo = tempo >= 40 && tempo <= 200 ? tempo : 120.f;
	for (int l = 0; l < LEVELS_LEN; l++)
		p.levels[l] = f[LEVELS + l] / 255.f;
	p.swing = 0.5f + 0.25f * f[SWING] / 255.f;             // a guess: 0 straight, 255 the most
	p.metronomeLevel = f[METRONOME_LEVEL] / 255.f;
	p.metronomeSound = f[METRONOME_SOUND] * 6 / 256;
	for (int t = 0; t < LANES; t++) {
		Project::Part& part = p.parts[t];
		int ft = FILE_TRACK[t];
		const uint8_t* track = &f[at + TRACK_CHUNKS + 12 * ft];
		// Guesses: the step count as is, the step length as its value key 1 to 0 (10), quantize and the note length
		// as their dials' positions.
		part.stepCount = track[4] >= 1 && track[4] <= STEPS ? track[4] : STEPS;
		part.stepLength = track[6] >= 1 && track[6] <= 10 ? track[6] - 1 : 0;
		part.quantize = track[7] / 255.f;
		part.noteLength = std::min<int>(track[9], std::size(NOTE_LENGTHS) - 1);
		part.hasSound = t < TRACKS;
		std::fill(part.sound + LFO_DIALS, part.sound + LFO_DIALS + 4, 0x80 / 255.f); // the arp track's LFO, as a blank file's
		for (int i = 0; part.hasSound && i < FILE_SOUNDS; i++)
			part.sound[soundParam(t, i)] = f[at + SOUNDS + 18 * ft + i] / 255.f;

		part.pattern.clear();
		for (int s = 0; s < STEPS; s++) {
			Step& step = part.pattern.steps[s];
			for (int k = 0; k < std::min(SLOT_COUNT[t], STEP_NOTES); k++) {
				size_t note = at + NOTES + 8 * (SLOTS * s + SLOT_OFFSET[t] + k);
				if (f[note + 4] > 127)
					continue;
				int32_t units = (int32_t) get(f, note, 4);
				int ticks = units > STEPS * UNITS ? DRONE : std::max((int) std::lround(units * double(TICKS) / UNITS), 0);
				Note n;
				n.pitch = f[note + 4];
				n.velocity = std::clamp<int>(f[note + 5], 1, 127);
				n.offset = std::clamp<int>(int8_t(f[note + 6]) / 8, MIN_OFFSET, MAX_OFFSET);
				n.length = ticks == NOTE_LENGTHS[part.noteLength] ? 0 : ticks;
				step.add(n);
			}
			const uint8_t* chunk = &f[at + STEP_CHUNKS + 54 * (16 * ft + s)];
			uint16_t mask = chunk[0] | chunk[1] << 8;
			for (int b = 0; b < COMPONENTS_LEN; b++) {
				if (!(mask >> b & 1))
					continue;
				int c = COMPONENT_BITS[b], raw = chunk[2 + b];
				int key = std::clamp((raw & 15) - 1, 0, 9);
				step.set(c, c >= PARAMETER_SPARK ? key | (raw >> 4 & 3) << 4 : key);
			}
			for (int i = 0; i < FILE_SOUNDS; i++)
				if (chunk[36 + i])
					step.lock(soundParam(t, i), chunk[18 + i] / 255.f);
		}
	}
	p.opz = f;
	p.used = true;
	return true;
}

/** A project as a file: the one it was imported from with what it models written over, or a blank one. */
inline std::vector<uint8_t> write(const Project& p) {
	std::vector<uint8_t> f = p.opz.size() == SIZE ? p.opz : blank();
	size_t at = pattern(f);
	put(f, TEMPO, 2, (uint32_t) std::lround(p.tempo));
	for (int l = 0; l < LEVELS_LEN; l++)
		f[LEVELS + l] = byte(p.levels[l]);
	f[SWING] = byte((p.swing - 0.5f) / 0.25f);
	f[METRONOME_LEVEL] = byte(p.metronomeLevel);
	f[METRONOME_SOUND] = (uint8_t) std::clamp(p.metronomeSound * 256 / 6 + 21, 0, 255);
	for (int t = 0; t < LANES; t++) {
		const Project::Part& part = p.parts[t];
		int ft = FILE_TRACK[t];
		uint8_t* track = &f[at + TRACK_CHUNKS + 12 * ft];
		track[4] = (uint8_t) std::clamp(part.stepCount, 1, STEPS);
		track[6] = (uint8_t) (part.stepLength + 1);
		track[7] = byte(part.quantize);
		track[9] = (uint8_t) part.noteLength;
		if (part.hasSound)
			for (int i = 0; i < FILE_SOUNDS; i++)
				f[at + SOUNDS + 18 * ft + i] = byte(part.sound[soundParam(t, i)]);

		for (int s = 0; s < STEPS; s++) {
			const Step& step = part.pattern.steps[s];
			for (int k = 0; k < SLOT_COUNT[t]; k++) {
				size_t note = at + NOTES + 8 * (SLOTS * s + SLOT_OFFSET[t] + k);
				if (k >= step.count) {
					f[note + 4] = 0xff;
					continue;
				}
				const Note& n = step.notes[k];
				int ticks = n.length ? n.length : NOTE_LENGTHS[part.noteLength];
				put(f, note, 4, ticks == DRONE ? 0x7fffffff : (uint32_t) std::lround(ticks * double(UNITS) / TICKS));
				f[note + 4] = (uint8_t) n.pitch;
				f[note + 5] = n.velocity;
				f[note + 6] = (uint8_t) int8_t(8 * std::clamp<int>(n.offset, -12, 11));
				f[note + 7] = 0;
			}
			uint8_t* chunk = &f[at + STEP_CHUNKS + 54 * (16 * ft + s)];
			uint16_t mask = 0;
			for (int b = 0; b < COMPONENTS_LEN; b++) {
				int c = COMPONENT_BITS[b];
				chunk[2 + b] = step.has(c) ? (uint8_t) ((step.values[c] & 15) + 1 + (step.values[c] & 0x30)) : 0;
				mask |= step.has(c) << b;
			}
			chunk[0] = mask & 0xff;
			chunk[1] = mask >> 8;
			// A lock let go of keeps its old value, as the OP-Z leaves it.
			for (int i = 0; i < FILE_SOUNDS; i++) {
				int param = soundParam(t, i);
				if (step.locked(param))
					chunk[18 + i] = byte(step.lockValues[param]);
				chunk[36 + i] = step.locked(param) ? 0xff : 0;
			}
		}
	}
	return f;
}

} // namespace opzfile
} // namespace opz
