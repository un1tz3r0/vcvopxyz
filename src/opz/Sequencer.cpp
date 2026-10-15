/*
Sequencer: the OP-Z's front panel, and the master of the OP-Z modules to its right. It keeps the tempo and the
transport, and sequences the first eight tracks: four drum tracks (kick, snare, perc and sample), then bass, lead,
arp and chord. Each track has a lane of sixteen steps of up to four notes, filled from the keys or from poly V/OCT,
GATE and VEL inputs.

Each lane plays its track's engine over the bus (Bus.hpp), and the engines' sound comes back to the L and R
outputs; a Breakout engine sends a synth track's notes out as CV. The four dials are the selected track's: whichever engine
plays it, they turn its own parameters.

Its keys are laid out as on the OP-Z: TRACK, REC and −, PLAY and +, STOP and SHIFT, then the 16 steps along the top,
the numbered black keys and the white keys from F to E. Rack can't hold one button down while clicking another, so
a long press or a Ctrl+click picks a step up, and it stays held (blinking) until it's clicked again; SHIFT latches
the same way, and a click on it turns the page. TRACK latches, and so do the first three of the OP-Z's four index
keys over the display: PROJECT (ten project slots on the value keys, and each project's 16 patterns on the steps),
MIXER (group levels, punch, master, and mutes on the steps) and TEMPO (tempo, swing and the metronome). The fourth,
SCREEN, shows the battery while it's held.

With SHIFT latched, the steps are picked for step components (blinking green), the white keys put a component on
them, as the icons above the keys say, and the black keys set its value. While a step is held, the dials lock the
engine's parameters on it.

After the eight comes the master track, the OP-Z's 12th: a lane with no engine whose notes move the tracks it
transposes, in key, for chord progressions. The key and mode come from the notes of the bass, lead, arp and chord
tracks, and the ramps, random walks and tonality component play in them too.

Then the two FX tracks, the OP-Z's 9th and 10th, which play the FX modules: the send effects every engine's FX 1 and
FX 2 dials send to. Their steps lock the effects' dials, and their notes set the effects' time.

A project has 16 patterns, each every track's steps, settings and dials. The lanes, the params and the engines hold
the one playing; the others wait in scenes. A chain plays patterns one after another, each starting over from its
first step as the last ends.
*/
#include "Track.hpp"
#include "Sequence.hpp"
#include "Project.hpp"
#include <mutex>
#include <osdialog.h>

namespace opz {

/** The keyboard: two octaves, F to E. */
constexpr int KEYS = 24;
/** The black keys, in semitones above F, double as the value keys: 1 to 9 then 0 from the left. */
constexpr int BLACK_KEYS[5] = {1, 3, 5, 8, 10};
inline int keyValue(int key) {
	const int* k = std::find(std::begin(BLACK_KEYS), std::end(BLACK_KEYS), key % 12);
	return k == std::end(BLACK_KEYS) ? -1 : 5 * (key / 12) + int(k - BLACK_KEYS);
}
inline int valueKey(int value) {
	return 12 * (value / 5) + BLACK_KEYS[value % 5];
}

/** The white keys, in semitones above F, pick the step components: pulse on the left to trigger spark on the right. */
constexpr int WHITE_KEYS[7] = {0, 2, 4, 6, 7, 9, 11};
inline int keyComponent(int key) {
	const int* k = std::find(std::begin(WHITE_KEYS), std::end(WHITE_KEYS), key % 12);
	return k == std::end(WHITE_KEYS) ? -1 : 7 * (key / 12) + int(k - WHITE_KEYS);
}
inline int componentKey(int c) {
	return 12 * (c / 7) + WHITE_KEYS[c % 7];
}

static const char* const COMPONENT_NAMES[COMPONENTS_LEN] = {"Pulse", "Pulse hold", "Multiply", "Velocity", "Ramp up",
	"Ramp down", "Random", "Portamento", "Sweep", "Tonality", "Jump", "Parameter spark", "Component spark", "Trigger spark"};
/** The value key a component starts on: one that does something. */
static const int COMPONENT_DEFAULTS[COMPONENTS_LEN] = {1, 1, 1, 4, 2, 2, 2, 3, 0, 0, 0, 1, 1, 1};

inline bool isSpark(int c) {
	return c >= PARAMETER_SPARK;
}

/** What a component does at a value, its value key 0 to 9 plus, for the sparks, its mode times 16. */
inline std::string componentValue(int c, int value) {
	int key = value & 15, mode = value >> 4;
	auto notes = [](int k) { return string::f("%d notes over %s", k % 5 + 2, k < 5 ? "1 octave" : "3 octaves"); };
	switch (c) {
		case PULSE:
		case PULSE_HOLD: return key == 9 ? "random" : string::f("%d", key + 1);
		case MULTIPLY: return key == 8 ? "broken chord" : key == 9 ? "quantize" : string::f("×%d", key + 1);
		case VELOCITY: return key == 8 ? "mute" : key == 9 ? "random" : key == 4 ? "default" : string::f("%+d", key - 4);
		case RAMP_UP:
		case RAMP_DOWN:
		case RANDOM: return notes(key);
		case PORTAMENTO: return key == 8 ? "direct" : key == 9 ? "random" : string::f("glide %d", key + 1);
		case SWEEP: {
			static const char* const v[] = {"filter up", "filter down", "synth up", "synth down", "pan", "filter up, long",
				"filter down, long", "synth up, long", "synth down, long", "random"};
			return v[key];
		}
		case TONALITY: {
			static const char* const v[] = {"ignore chords", "transpose only", "+octave", "+fifth", "+third", "chromatic up",
				"chromatic down", "quantize 1", "quantize 2", "quantize 3"};
			return v[key];
		}
		case JUMP: {
			static const char* const v[] = {"to start", "to 2/4", "to 3/4", "to 4/4", "forward", "back", "random", "stay",
				"align", "gate step"};
			return v[key];
		}
		default: {
			if (key == 8)
				return "random";
			if (key == 9)
				return "reset counter";
			int n = key + 1;
			static const char* const modes[] = {"pass %d of %d", "pass 1 of %d", "all but pass %d of %d", "all but pass 1 of %d"};
			return mode == 0 || mode == 2 ? string::f(modes[mode], n, n) : string::f(modes[mode], n);
		}
	}
}

inline std::string noteName(int pitch) {
	static const char* const names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
	return string::f("%s%d", names[pitch % 12], pitch / 12 - 1);
}

/** The keys' LED colours, besides the notes' orange. */
namespace led {
const NVGcolor WHITE = nvgRGBf(1.f, 1.f, 1.f), RED = nvgRGBf(1.f, 0.1f, 0.06f), GREEN = nvgRGBf(0.15f, 1.f, 0.3f), BLUE = nvgRGBf(0.2f, 0.4f, 1.f);
}

static const char* const TRACK_NAMES[LANES] = {"Kick", "Snare", "Perc", "Sample", "Bass", "Lead", "Arp", "Chord", "Master", "FX 1", "FX 2"};
/** Each track's polyphony to begin with, as the OP-Z's: two notes a step for the drums, one for the bass and arp,
three for the lead, four for the chords and the master track, and one for the FX tracks. */
static const int TRACK_CHANNELS[LANES] = {2, 2, 2, 2, 1, 3, 1, 4, 4, 1, 1};

/** The track a step stands for in track and mixer mode, or -1: the one the OP-Z numbers with it. */
inline int stepLane(int i) {
	for (int t = 0; t < LANES; t++)
		if (trackNumber(t) == i + 1)
			return t;
	return -1;
}

inline std::string trackName(int t) {
	return string::f("Track %d, %s", trackNumber(t), TRACK_NAMES[t]);
}

/** A chain's patterns, numbered as the steps are. */
inline std::string chainName(const Project::Chain& c) {
	std::string name;
	for (int i = 0; i < c.length; i++)
		name += (i ? " " : "") + std::to_string(c.patterns[i] + 1);
	return c.length ? name : "empty";
}

static const char* const MODE_NAMES[7] = {"major", "dorian", "phrygian", "lydian", "mixolydian", "minor", "locrian"};

/** A key and mode in words, flat keys flat: "B♭ minor". */
inline std::string scaleName(Scale s) {
	static const char* const tonics[12] = {"C", "D♭", "D", "E♭", "E", "F", "F♯", "G", "A♭", "A", "B♭", "B"};
	return string::f("%s %s", tonics[s.tonic()], MODE_NAMES[s.mode]);
}

/** A key and mode on the display: the tonic, with b for flat (and b for B, which a seven-segment display can't make
apart from 8), then the mode, 1 for major to 7. */
inline std::string scaleDisplay(Scale s) {
	static const char* const tonics[12] = {"C", "db", "d", "Eb", "E", "F", "Gb", "G", "Ab", "A", "bb", "b"};
	return string::f("%2s%d", tonics[s.tonic()], s.mode + 1);
}

constexpr int LONGEST = STEPS * TICKS; // the longest note a step's menu sets, short of a drone


/** The metronome's clicks. The OP-Z counts in voices (Swedish, English, German...), which these six synthesized
sounds stand in for; the first beat of each bar is higher. */
struct Metronome {
	static constexpr int SOUNDS = 6;
	int sound = 0;
	float phase = 0.f, env = 0.f, freq = 0.f;
	uint32_t noise = 0x2545F491u;

	void trigger(int s, bool accent) {
		static const float pitches[SOUNDS] = {2000.f, 1300.f, 880.f, 540.f, 0.f, 3000.f};
		sound = s;
		freq = pitches[s] * (accent ? 1.5f : 1.f);
		phase = 0.f;
		env = 1.f;
	}

	float process(float dt) {
		if (env < 1e-4f)
			return 0.f;
		static const float decays[SOUNDS] = {0.006f, 0.025f, 0.06f, 0.12f, 0.025f, 0.012f};
		env *= std::exp(-dt / decays[sound]);
		noise ^= noise << 13;
		noise ^= noise >> 17;
		noise ^= noise << 5;
		float white = int32_t(noise) * (1.f / 2147483648.f);
		float f = sound == 5 ? freq * (0.3f + 0.7f * env) : freq;    // the blip falls as it fades
		phase = std::fmod(phase + f * dt, 1.f);
		float sine = std::sin(2.f * float(M_PI) * phase), square = phase < 0.5f ? 1.f : -1.f;
		switch (sound) {
			case 2: return env * 0.6f * square;
			case 3: return env * 0.5f * (square + (std::fmod(phase * 1.48f, 1.f) < 0.5f ? 1.f : -1.f));
			case 4: return env * white;
			default: return env * sine;
		}
	}
};

/** The mixer's punch: a compressor on the mix that pulls loud passages down and the whole back up. */
struct Punch {
	float env = 0.f;

	void process(float& left, float& right, float amount, float dt) {
		if (amount <= 0.f)
			return;
		float level = std::fmax(std::fabs(left), std::fabs(right)) / 5.f;
		env += (level - env) * (1.f - std::exp(-dt / (level > env ? 0.005f : 0.12f)));
		float threshold = 1.f - 0.75f * amount, ratio = 1.f + 7.f * amount;
		float gain = (env > threshold ? std::pow(env / threshold, 1.f / ratio - 1.f) : 1.f) * (1.f + 1.5f * amount);
		left *= gain;
		right *= gain;
	}
};


struct Sequencer : Module, BusNode {
	enum ParamId {
		TEMPO_PARAM, SWING_PARAM,
		ENUMS(LENGTH_PARAM, TRACKS), ENUMS(QUANTIZE_PARAM, TRACKS), ENUMS(STEP_COUNT_PARAM, TRACKS), ENUMS(STEP_LENGTH_PARAM, TRACKS),
		ENUMS(LEVEL_PARAM, LEVELS_LEN), METRONOME_SOUND_PARAM, METRONOME_LEVEL_PARAM,
		// The keys, each with an RGB light: three light ids from KEY_LIGHT, in the same order.
		TRACK_PARAM, REC_PARAM, PLAY_PARAM, STOP_PARAM, OCTAVE_DOWN_PARAM, OCTAVE_UP_PARAM, SHIFT_PARAM,
		PROJECT_PARAM, MIXER_PARAM, METRONOME_PARAM, SCREEN_PARAM,
		ENUMS(STEP_PARAM, STEPS), ENUMS(KEY_PARAM, KEYS), ENUMS(SELECT_PARAM, TRACKS), ENUMS(PAGE_PARAM, PAGES),
		// What the dials turn while a step is held: its locks. Last, so older patches keep their params.
		ENUMS(LOCK_PARAM, 4),
		// The master track's key, its settings in the tracks' order, and its note style, after them for the same reason.
		MASTER_PARAM, MASTER_LENGTH_PARAM, MASTER_QUANTIZE_PARAM, MASTER_STEP_COUNT_PARAM, MASTER_STEP_LENGTH_PARAM, MASTER_STYLE_PARAM,
		// The FX tracks' settings, after those again.
		ENUMS(FX_LENGTH_PARAM, FX_TRACKS), ENUMS(FX_QUANTIZE_PARAM, FX_TRACKS), ENUMS(FX_STEP_COUNT_PARAM, FX_TRACKS), ENUMS(FX_STEP_LENGTH_PARAM, FX_TRACKS),
		PARAMS_LEN
	};
	enum InputId { CLOCK_INPUT, RUN_INPUT, RESET_INPUT, CONT_INPUT, ADVANCE_INPUT, VOCT_INPUT, GATE_INPUT, VELOCITY_INPUT, INPUTS_LEN };
	enum OutputId {
		CLOCK_OUTPUT, RUN_OUTPUT, RESET_OUTPUT, CONT_OUTPUT, LEFT_OUTPUT, RIGHT_OUTPUT,
		// The tracks' V/OCT and GATE until 2.0.9. Rack adds a patch's cables before it looks for their jacks, so an
		// older patch's cables to them need ports to land on; with no jacks on the panel, Rack then drops them.
		ENUMS(OLD_TRACK_OUTPUT, 2 * TRACKS),
		OUTPUTS_LEN
	};
	static constexpr int JACKS = OLD_TRACK_OUTPUT; // the outputs on the panel
	enum LightId {
		ENUMS(KEY_LIGHT, 3 * (LOCK_PARAM - TRACK_PARAM + 1)), ENUMS(DIAL_LIGHT, 3 * 4),
		ENUMS(CHANNEL_LIGHT, CHANNELS), INTERNAL_LIGHT, EXTERNAL_LIGHT,
		ENUMS(INPUT_LIGHT, 2 * INPUTS_LEN), ENUMS(OUTPUT_LIGHT, 2 * JACKS),
		LIGHTS_LEN
	};
	static constexpr float LONG_PRESS = 0.4f, CLEAR_HOLD = 1.f; // seconds
	static constexpr int SOURCES = KEYS + CHANNELS;             // live notes: the keys, then the GATE input's channels
	static constexpr int ALL_KEYS = LOCK_PARAM - TRACK_PARAM + 1; // from TRACK_PARAM on, then MASTER_PARAM
	enum { LATCH, FREE };                                         // the master track's note styles

	/** A key's place among them all, for its gesture and its light, and back. */
	static int keyIndex(int param) {
		return param == MASTER_PARAM ? ALL_KEYS - 1 : param - TRACK_PARAM;
	}
	static int keyParam(int index) {
		return index == ALL_KEYS - 1 ? MASTER_PARAM : TRACK_PARAM + index;
	}
	static int keyLight(int param) {
		return KEY_LIGHT + 3 * keyIndex(param);
	}

	/** A lane's param for one of the track settings, given the first track's: LENGTH_PARAM, QUANTIZE_PARAM,
	STEP_COUNT_PARAM or STEP_LENGTH_PARAM. The master track's come after the rest, then the FX tracks', in the same
	order. */
	static int laneParam(int first, int t) {
		int setting = (first - LENGTH_PARAM) / TRACKS;
		return t < TRACKS ? first + t : t == MASTER ? MASTER_LENGTH_PARAM + setting : FX_LENGTH_PARAM + FX_TRACKS * setting + t - FX1;
	}

	/** A track's sequence. */
	struct Lane {
		Pattern pattern;
		Pattern recorded;        // the pattern as the undo history last saw it; the UI thread's
		Settings settings;
		Player player;
	};

	/** A note played live, from a key or a GATE channel, on the lane that was selected when it started. */
	struct Live {
		bool on = false;
		int lane = 0;
		uint32_t id = 0;        // the player's id for it
		int pitch = 60;
		int recorded = -1;      // the step it was recorded into while playing, which gets its length on release
		double start = 0.0, slot = 1.0;
	};

	struct StepQuantity : SwitchQuantity {
		std::string getLabel() override {
			Sequencer* m = static_cast<Sequencer*>(module);
			int i = paramId - STEP_PARAM;
			int t = stepLane(i);
			if (m->mixerMode)
				return t >= 0 ? trackName(t) + (m->muted(t) ? ": muted" : m->audioMuted(t) ? ": audio muted" : ": on") : "";
			if (m->projectMode) {
				std::string label = string::f("Pattern %d", i + 1);
				label += i == m->pattern ? " (playing)" : m->scenes[i].empty() ? " (empty)" : "";
				return label + (m->copying ? ": click to paste" : m->chaining ? ": click to chain" : "");
			}
			if (m->trackMode)
				return m->shifted ? string::f("Step count %d", i + 1) : t >= 0 ? trackName(t) : "";
			std::string label = string::f("Step %d", i + 1);
			const Step& s = m->lane().pattern.steps[i];
			for (int n = 0; n < s.count; n++)
				label += (n ? " " : ": ") + noteName(s.notes[n].pitch);
			for (int c = 0, n = 0; c < COMPONENTS_LEN; c++)
				if (s.has(c))
					label += std::string(n++ ? ", " : "; ") + COMPONENT_NAMES[c] + " " + componentValue(c, s.values[c]);
			if (int n = __builtin_popcount(s.locks))
				label += string::f("; %d lock%s", n, n > 1 ? "s" : "");
			if (m->editingComponents())
				label += m->selection >> i & 1 ? " (picked)" : " (click to pick)";
			return label;
		}
	};

	struct KeyQuantity : SwitchQuantity {
		std::string getLabel() override {
			static const char* const values[] = {"×1", "×2", "×3", "×4", "×5", "×6", "×7", "×8", "×16", "one per ADV trigger"};
			Sequencer* m = static_cast<Sequencer*>(module);
			int key = paramId - KEY_PARAM, value = keyValue(key);
			if (m->trackMode && value >= 0)
				return string::f("Step length %s", values[value]);
			if (m->tempoMode)
				return value >= 0 ? string::f("Tempo digit %d", (value + 1) % 10) : "Tap tempo";
			if (m->projectMode)
				return value >= 0 ? string::f("Project %d%s", value + 1, m->projects[value].used ? "" : " (empty)")
					: string::f("Chain %d: %s", keyComponent(key) + 1, chainName(m->chains[keyComponent(key)]).c_str());
			if (m->mixerMode)
				return value >= 0 ? string::f("Mute group %d", value + 1) : "";
			if (m->editingComponents()) {
				if (value < 0)
					return COMPONENT_NAMES[keyComponent(key)];
				return m->component >= 0 ? string::f("%s %s", COMPONENT_NAMES[m->component], componentValue(m->component, value).c_str())
					: string::f("Value %d", (value + 1) % 10);
			}
			int pitch = m->keyPitch(key);
			if (m->track == MASTER) {
				Tonality t = masterMove(m->scale, pitch);
				std::string move = t.degrees ? string::f("%+d degree%s", t.degrees, std::abs(t.degrees) > 1 ? "s" : "")
					: t.semitones ? string::f("%+d semitone%s", t.semitones, std::abs(t.semitones) > 1 ? "s" : "") : "no move";
				return noteName(pitch) + ": " + move + ", to " + scaleName(t.moved());
			}
			Track* e = m->engine(m->track);
			std::string sound = e ? e->soundName(pitch) : "";
			return noteName(pitch) + (sound.empty() ? "" : ": " + sound);
		}
	};

	/** A track key: on the master track with SHIFT latched, it says whether the master track moves its track. */
	struct TrackKeyQuantity : SwitchQuantity {
		std::string getLabel() override {
			Sequencer* m = static_cast<Sequencer*>(module);
			int t = paramId - SELECT_PARAM;
			if (m->choosingTransposed())
				return trackName(t) + (m->transposed >> t & 1 ? ": transposed" : ": not transposed");
			return trackName(t);
		}
	};

	/** A dial while a step is held: the step's lock on the engine's dial under it, or the dial itself where the step
	has none. Turning it locks the dial on the step. It runs on the UI thread, as the step menu's edits do, and the
	pattern's own undo covers it. */
	struct LockQuantity : ParamQuantity {
		Sequencer* sequencer() {
			return static_cast<Sequencer*>(module);
		}

		ParamQuantity* target() {
			std::pair<Track*, int> t = sequencer()->lockTarget(paramId - LOCK_PARAM);
			return t.first ? t.first->paramQuantities[t.second] : nullptr;
		}

		Step& step() {
			return sequencer()->lane().pattern.steps[std::max(sequencer()->held, 0)];
		}

		bool locked() {
			ParamQuantity* q = target();
			return q && step().locked(q->paramId);
		}

		float getValue() override {
			ParamQuantity* q = target();
			return !q ? 0.f : step().locked(q->paramId) ? step().lockValues[q->paramId] : q->getScaledValue();
		}

		void setValue(float value) override {
			if (ParamQuantity* q = target())
				step().lock(q->paramId, value);
		}

		void unlock() {
			if (ParamQuantity* q = target())
				step().unlock(q->paramId);
		}

		std::string getLabel() override {
			ParamQuantity* q = target();
			return q ? string::f("Step %d, %s%s", sequencer()->held + 1, q->getLabel().c_str(), locked() ? " (locked)" : "") : "Lock";
		}

		std::string getDescription() override {
			return locked() ? "Double-click to let go of the lock" : "Turn to lock it on the held step";
		}

		/** A lock is a share of the dial's travel; one with steps shows the step it lands on. */
		std::string getDisplayValueString() override {
			ParamQuantity* q = target();
			if (!q || !q->snapEnabled)
				return ParamQuantity::getDisplayValueString();
			int i = (int) std::round(math::rescale(getValue(), 0.f, 1.f, q->getMinValue(), q->getMaxValue()));
			if (SwitchQuantity* labels = dynamic_cast<SwitchQuantity*>(q))
				return labels->labels[clamp(i - (int) q->getMinValue(), 0, (int) labels->labels.size() - 1)];
			return std::to_string(i);
		}

		std::string getUnit() override {
			ParamQuantity* q = target();
			return q && q->snapEnabled ? "" : unit;
		}
	};

	Lane lanes[LANES];           // the eight tracks', then the master track's
	int track = DRUM_TRACKS;     // the selected track, from 0: bass
	Step memory;                 // what a click on an empty step pastes: the last notes played, or the step last held
	Live live[SOURCES];
	bool playing = false;        // the transport, as PLAY shows it
	bool armed = false;          // playing, but waiting for the clock's next pulse to start
	bool resuming = false;       // and when it comes, carrying on rather than starting over
	bool recording = false, trackMode = false, tempoMode = false, projectMode = false, mixerMode = false, shifted = false;
	bool erasing = false;        // removing the held notes from the steps they pass, while recording
	bool chord = false;          // notes are down for the step being step-recorded
	int held = -1, cursor = 0, octave = 0, ppqn = 4;
	uint16_t selection = 0;      // the steps picked for step components, with SHIFT latched
	int component = -1;          // the component the white keys picked last for them
	int page = NEXT_PAGE;        // the page last asked of the selected track's engine, and how many times one was
	uint32_t pageRequests = 0;
	int typed = 0, digits = 0;   // a tempo being typed on the value keys
	float sinceTap = -1.f, tapPeriod = 0.f; // tap tempo, in seconds
	bool longPressHolds = true, ctrlClickHolds = true;
	bool turningLock = false;    // a dial is being dragged on a lock, so the undo history waits to take the whole turn
	std::atomic<float> shownTempo{120.f}; // -1 while an external clock is still being measured

	Scale scale;                 // the key and mode of the synth tracks' notes
	uint16_t transposed = 0xf0;  // the tracks the master track moves, a bit each: the synth tracks to begin with
	int masterNote = -1;         // the note on the master track moving them, or -1
	int masterStep = -1;         // the step its playhead was on
	uint32_t masterHeard = 0;    // the id of its last note followed
	Tonality transposition;      // the master track's move now, for the tracks it transposes
	int movedBy = -1;            // the note it's from, or -1
	std::atomic<int> shownScale{0}; // the key and mode the tracks play in now, key times 8 plus mode, for the display

	Project projects[PROJECTS];
	int project = 0;             // the slot the live project came from, and goes back to
	bool autoSave = true;        // as the OP-Z: switching projects saves the one left
	const Project blank = blankProject(); // a new project, for an empty slot: kept, as it's too big for the audio thread's stack
	// The live project's 16 patterns: the one playing lives in the lanes, the params and the engines' dials, the rest
	// here. The audio thread alone changes them and the chains, and the UI thread reads them, under the slots' lock.
	Project::Scene scenes[PATTERN_COUNT];
	int pattern = 0;                // the pattern playing
	Project::Chain chain = Project::Chain::of(0); // the chain playing: the pattern, or patterns played in turn
	Project::Chain chains[SAVED_CHAINS];          // the saved chains, on the white keys in project mode
	int chainAt = 0;                // where in the chain the pattern playing is
	double patternStart = 0.0;      // the song time the pattern playing started at, for the chain to move on from
	bool chaining = false;          // PLAY in project mode: the steps build a new chain
	int chained = 0;                // patterns picked for it so far
	bool copying = false;           // SHIFT in project mode copied the pattern: the steps paste it
	Project::Scene clipboard;
	// What the project mode keys asked for, done when the slots' lock is free: patterns picked, a white key's chain
	// to play or save over, pastes, going back to the chain's first pattern, and clearing every pattern.
	int picks[CHAIN_LENGTH] = {}, picked = 0;
	int chainKey = -1;
	bool chainKeySave = false, rewound = false, clearAll = false;
	uint16_t pastes = 0;
	// The slots, and the projects and recall below, belong to the audio and UI threads both, under this lock. The UI
	// thread can hold it a while, writing files, so the audio thread only tries it, and project work waits till it's free.
	std::mutex projectsMutex;
	std::atomic<int> reload{-1};    // a slot the UI thread changed, to load as the live project
	std::atomic<int> restoring{-1}; // the slot of a project the undo history puts back: restored
	int keyed = -1;                 // a slot a project key picked, to switch to, or with keyedSave to save to
	bool keyedSave = false;
	Project restored;
	Project left, entered;          // the live project either side of the last switch, for the undo history
	int leftSlot = 0;
	// A change of live project or pattern the widget has yet to see; only a project switch is a step of the undo history.
	enum { SETTLED, PATTERNED, RESTORED, SWITCHED } recall = SETTLED;
	uint16_t mutes[MUTE_GROUPS] = {}, audioMutes[MUTE_GROUPS] = {};
	int muteGroup = 0;
	float battery = 0.87f, batteryShown = 0.f; // a pretend charge, and how long SCREEN shows it after a press
	Metronome metronome;
	Punch punch;

	KeyGesture gestures[ALL_KEYS]; // the keys', from TRACK_PARAM on
	float clearing = -1.f;       // how long STOP has been held in track mode, or -1
	dsp::SchmittTrigger gates[CHANNELS], clockIn, runIn, resetIn, contIn, advanceIn;
	dsp::PulseGenerator clockOut, resetOut, contOut;
	bool external = false;
	long pulses = 0, lastTick = -1, lastBeat = -1;
	float sincePulse = 0.f, pulsePeriod = 0.f; // seconds; a period of 0 is unknown
	Upstream engines;            // the last the engines sent back
	dsp::ClockDivider controlDivider, lightDivider, scaleDivider;
	Meter inputMeters[INPUTS_LEN], outputMeters[JACKS];
	float blink = 0.f;

	Sequencer() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		attach(this);
		ParamQuantity* q = configParam(TEMPO_PARAM, 40.f, 200.f, 120.f, "Tempo", " BPM");
		q->snapEnabled = true;
		q->randomizeEnabled = false;
		q->description = "A patched CLOCK overrides it";
		configParam(SWING_PARAM, 0.5f, 0.75f, 0.5f, "Swing", "%", 0.f, 100.f)->description = "Delays every second sixteenth; 50% is straight";
		configParam(LEVEL_PARAM + DRUM_LEVEL, 0.f, 1.f, 1.f, "Drum level", "%", 0.f, 100.f)->description = "Tracks 1 to 4";
		configParam(LEVEL_PARAM + SYNTH_LEVEL, 0.f, 1.f, 1.f, "Synth level", "%", 0.f, 100.f)->description = "Tracks 5 to 8";
		configParam(LEVEL_PARAM + PUNCH, 0.f, 1.f, 0.f, "Punch", "%", 0.f, 100.f)->description = "Compresses the mix at L and R";
		configParam(LEVEL_PARAM + MASTER_LEVEL, 0.f, 1.f, 1.f, "Master level", "%", 0.f, 100.f);
		configSwitch(METRONOME_SOUND_PARAM, 0.f, Metronome::SOUNDS - 1, 0.f, "Metronome sound", {"Click", "Wood", "Beep", "Cowbell", "Hat", "Blip"});
		configParam(METRONOME_LEVEL_PARAM, 0.f, 1.f, 0.f, "Metronome level", "%", 0.f, 100.f)->description = "Off fully left. Into L and R while playing";
		for (int t = 0; t < LANES; t++) {
			std::string name = t == MASTER ? "Master track " : string::f("Track %d ", trackNumber(t));
			configSwitch(laneParam(LENGTH_PARAM, t), 0.f, std::size(NOTE_LENGTHS) - 1, 4.f, name + "note length",
				{"1/64", "1/64.", "1/32", "1/32.", "1/16", "1/16.", "1/8", "1/8.", "1/4", "1/4.", "1/2", "1/2.", "1 bar", "Drone"})
				->description = t == MASTER ? "For notes left at the default length, in free style" : "For notes left at the default length";
			configParam(laneParam(QUANTIZE_PARAM, t), 0.f, 1.f, 0.f, name + "quantize", "%", 0.f, 100.f)->description = "Pulls micro timing towards the grid as it plays";
			configParam(laneParam(STEP_COUNT_PARAM, t), 1.f, STEPS, STEPS, name + "step count")->snapEnabled = true;
			configSwitch(laneParam(STEP_LENGTH_PARAM, t), 0.f, 9.f, 0.f, name + "step length", {"×1", "×2", "×3", "×4", "×5", "×6", "×7", "×8", "×16", "Per ADV trigger"});
			getParamQuantity(laneParam(STEP_COUNT_PARAM, t))->randomizeEnabled = false;
			getParamQuantity(laneParam(STEP_LENGTH_PARAM, t))->randomizeEnabled = false;
			lanes[t].settings.channels = TRACK_CHANNELS[t];
		}
		for (int t = 0; t < TRACKS; t++)
			configKey<TrackKeyQuantity>(this, SELECT_PARAM + t)->description = "Selects it for the keys, the steps and the dials. "
				"On the master track with SHIFT latched, switches the master track's transposing of it";
		configKey(this, MASTER_PARAM, trackName(MASTER))->description = "Selects the master track, whose notes move the tracks it "
			"transposes, in key. With it selected and SHIFT latched, the track keys pick which those are";
		configSwitch(MASTER_STYLE_PARAM, LATCH, FREE, LATCH, "Master track note style", {"Latch", "Free"})->description =
			"Latch plays its notes on the steps and holds each until the next, or the start of the pattern; free plays them as they are";
		configKey(this, TRACK_PARAM, "Track")->description = "On and off: steps 1 to 8, 9 and 10 for the FX tracks and 12 for the master track select a track, and the dials set its note length, style, "
			"quantize and portamento. With SHIFT, the steps set the step count; the black keys set the step length. − and + shift the pattern, "
			"and holding STOP clears it";
		configKey(this, REC_PARAM, "Record")->description = "On and off: records the keys and inputs live while playing, or a step at a time while stopped";
		configKey(this, PLAY_PARAM, "Play")->description = "Starts from the first step, or the clock's next pulse";
		configKey(this, STOP_PARAM, "Stop")->description = "Stopped already: lets go of every note";
		configKey(this, OCTAVE_DOWN_PARAM, "Octave down")->description = "Nudges a held step a tick earlier; with notes held while recording, erases them";
		configKey(this, OCTAVE_UP_PARAM, "Octave up")->description = "Nudges a held step a tick later";
		configKey(this, SHIFT_PARAM, "Shift")->description = "Click: the next page of dials. Long-press or " RACK_MOD_CTRL_NAME "+click: latch it, "
			"to pick steps for step components; then a white key puts one on them, again takes it off, and the black keys set its value. "
			"STOP clears every component on the track";
		configKey(this, PROJECT_PARAM, "Project")->description = "On and off: the steps play patterns 1 to 16, and the black keys load projects 1 "
			"to 10, saving the one left; a long press or " RACK_MOD_CTRL_NAME "+click saves to one. PLAY builds a chain of patterns on the steps, "
			"and the white keys play saved chains, a long press saving one. SHIFT copies the pattern for the steps to paste. Holding STOP clears "
			"the pattern, or with SHIFT latched, the whole project";
		configKey(this, MIXER_PARAM, "Mixer")->description = "On and off: the dials set the drum and synth levels, punch and the master level; "
			"steps 1 to 10 and 12 mute tracks (with SHIFT latched, their audio only), and the black keys pick one of 10 mute groups";
		configKey(this, METRONOME_PARAM, "Tempo")->description = "On and off: the dials set the tempo, swing, metronome sound and metronome level, "
			"the black keys type a tempo, the white keys tap one, and − and + nudge it";
		configKey(this, SCREEN_PARAM, "Screen")->description = "Hold: the battery level, on the steps and the display";
		for (int i = 0; i < STEPS; i++)
			configKey<StepQuantity>(this, STEP_PARAM + i)->description = "Click: paste the last notes played, or clear. Long-press or " RACK_MOD_CTRL_NAME "+click: hold, "
				"to edit its notes and lock the dials on it";
		for (int i = 0; i < KEYS; i++)
			configKey<KeyQuantity>(this, KEY_PARAM + i);
		static const char* const pages[PAGES] = {"Sound page", "Envelope page", "LFO page (the arp's, on the arp track)", "Mix page"};
		for (int i = 0; i < PAGES; i++)
			configKey(this, PAGE_PARAM + i, pages[i])->description = "Of the selected track's engine";
		for (int i = 0; i < CHANNELS; i++)
			configLight(CHANNEL_LIGHT + i, string::f("Channel %d", i + 1));
		for (int d = 0; d < 4; d++) {
			ParamQuantity* q = configParam<LockQuantity>(LOCK_PARAM + d, 0.f, 1.f, 0.f, "Lock", "%", 0.f, 100.f);
			q->resetEnabled = q->randomizeEnabled = false;
		}

		configInput(CLOCK_INPUT, "Clock; overrides the tempo");
		configInput(RUN_INPUT, "Run: plays from the start while high, stops when low");
		configInput(RESET_INPUT, "Reset");
		configInput(CONT_INPUT, "Continue: plays on from where it stopped");
		configInput(ADVANCE_INPUT, "Advance, one step per trigger at step length 0");
		configInput(VOCT_INPUT, "1V/octave pitch, to play and record on the selected track");
		configInput(GATE_INPUT, "Gate, to play and record on the selected track");
		configInput(VELOCITY_INPUT, "Velocity (0-10V)");
		configOutput(CLOCK_OUTPUT, "Clock, while playing");
		configOutput(RUN_OUTPUT, "Run, high while playing");
		configOutput(RESET_OUTPUT, "Reset, as it starts");
		configOutput(CONT_OUTPUT, "Continue, as it plays on");
		configOutput(LEFT_OUTPUT, "Left, the engines' mix");
		configOutput(RIGHT_OUTPUT, "Right, the engines' mix");
		memory.add(Note());
		controlDivider.setDivision(16);
		lightDivider.setDivision(64);
		scaleDivider.setDivision(256); // the key changes only as the patterns do
	}

	Lane& lane() {
		return lanes[track];
	}

	/** The keys run from F3 to E5, an octave either side of middle C, before − and + move them. */
	int keyPitch(int key) const {
		return clamp(53 + 12 * octave + key, 0, 127);
	}

	/** Whether TRACK or an index key has taken over the dials and keys. */
	bool modal() const {
		return trackMode || tempoMode || projectMode || mixerMode;
	}

	void toggleMode(bool Sequencer::*mode) {
		bool on = !(this->*mode);
		trackMode = tempoMode = projectMode = mixerMode = false;
		this->*mode = on;
		typed = digits = 0;
		chaining = copying = false;
	}

	bool editingComponents() const {
		return shifted && !modal();
	}

	bool muted(int t) const {
		return mutes[muteGroup] >> t & 1;
	}

	bool audioMuted(int t) const {
		return audioMutes[muteGroup] >> t & 1;
	}

	bool anyHeld() const {
		return std::any_of(std::begin(live), std::end(live), [](const Live& n) { return n.on; });
	}

	int stepCount() {
		return clamp(lane().settings.stepCount, 1, STEPS);
	}

	float bpm() {
		return external && pulsePeriod > 0.f ? 60.f / (pulsePeriod * ppqn) : params[TEMPO_PARAM].getValue();
	}

	/** The engine playing a track, found as the bus finds it; for the UI thread. None plays the master track. */
	Track* engine(int t) {
		static const int first[3] = {0, DRUM_TRACKS, FX1}, kinds[3] = {DRUM_TRACKS, SYNTH_TRACKS, FX_TRACKS};
		int taken[3] = {};
		for (Module* m = rightExpander.module; onBus(m); m = m->rightExpander.module)
			if (Track* e = dynamic_cast<Track*>(m)) {
				int k = e->effect ? 2 : e->drum ? 0 : 1;
				if (taken[k] < kinds[k] && first[k] + taken[k]++ == t)
					return e;
			}
		return nullptr;
	}

	/** The engine dial `d` locks on the held step: the selected track's, on its page. */
	std::pair<Track*, int> lockTarget(int d) {
		Track* e = held >= 0 && !modal() ? engine(track) : nullptr;
		int p = e ? e->dial(e->shownPage(), d) : -1;
		return {p >= 0 ? e : nullptr, p};
	}

	/** The module and param a dial turns: the selected track's engine's, or its lock on the held step, or in track and
	tempo mode, its settings. */
	std::pair<Module*, int> dialTarget(int d) {
		static const int tempoDials[4] = {TEMPO_PARAM, SWING_PARAM, METRONOME_SOUND_PARAM, METRONOME_LEVEL_PARAM};
		if (tempoMode)
			return {this, tempoDials[d]};
		if (mixerMode)
			return {this, LEVEL_PARAM + d};
		if (projectMode)
			return {nullptr, -1};
		if (trackMode && d % 2 == 0)
			return {this, laneParam(d == 0 ? LENGTH_PARAM : QUANTIZE_PARAM, track)};
		if (track == MASTER) {
			// No engine: only the blue dial, in track mode, for its note style.
			if (trackMode && d == 1)
				return {this, MASTER_STYLE_PARAM};
			return {nullptr, -1};
		}
		if (lockTarget(d).first)
			return {this, LOCK_PARAM + d};
		Track* e = engine(track);
		int p = e ? e->dial(trackMode ? PAGE_TRACK : e->shownPage(), d) : -1;
		return p < 0 ? std::make_pair<Module*, int>(nullptr, -1) : std::make_pair<Module*, int>(e, int(p));
	}

	void restart() {
		scale = findScale();
		for (Lane& l : lanes)
			l.player.start();
		// From the top: a chain starts over from its first pattern.
		patternStart = 0.0;
		if (chain.length > 1) {
			chainAt = 0;
			if (projectsMutex.try_lock()) {
				std::lock_guard<std::mutex> lock(projectsMutex, std::adopt_lock);
				switchPattern(chain.patterns[0]);
			}
			else
				rewound = true;
		}
		lastTick = lastBeat = -1;
		pulses = 0;
		resetOut.trigger(1e-3f);
	}

	void resume() {
		scale = findScale();
		for (Lane& l : lanes)
			l.player.resume();
		pulses = (long) std::floor(lanes[0].player.song() * ppqn / 4.0 + 1e-9);
		contOut.trigger(1e-3f);
	}

	/** Starts now, or with an external clock, on its next pulse. */
	void play(bool carryOn = false) {
		playing = true;
		armed = external;
		resuming = carryOn;
		if (armed)
			for (Lane& l : lanes)
				l.player.stop();
		else if (carryOn)
			resume();
		else
			restart();
	}

	void halt() {
		playing = armed = erasing = false;
		for (Lane& l : lanes)
			l.player.stop();
	}

	void stop() {
		if (trackMode)
			lane().player.silence();
		else if (playing)
			halt();
		else
			for (Lane& l : lanes)
				l.player.panic();
	}

	void select(int t) {
		if (t == track)
			return;
		for (int s = 0; s < SOURCES; s++)
			noteUp(s);
		track = t;
		held = -1;
		cursor = 0;
		selection = 0;
		component = -1;
		chord = erasing = false;
	}

	/** On the master track with SHIFT latched, the track keys pick the tracks it moves. */
	bool choosingTransposed() const {
		return track == MASTER && editingComponents();
	}

	void trackKey(int t) {
		if (choosingTransposed() && t < TRACKS)
			transposed ^= 1 << t;
		else
			select(t);
	}

	void requestPage(int p) {
		page = p;
		pageRequests++;
	}

	/** A pattern as a new project has it: no notes, the default settings, each track's own polyphony, and no dials, so
	the engines keep theirs until it's played. */
	static Project::Scene blankScene() {
		Project::Scene scene;
		for (int t = 0; t < LANES; t++)
			scene.parts[t].channels = TRACK_CHANNELS[t];
		return scene;
	}

	static Project blankProject() {
		Project p;
		for (Project::Scene& scene : p.scenes)
			scene = blankScene();
		return p;
	}

	/** The pattern playing into a scene: each track's steps and settings, the dials of the engine playing it, and the
	mute group. */
	void storeScene(Project::Scene& scene) {
		for (int t = 0; t < LANES; t++) {
			Project::Part& part = scene.parts[t];
			part.pattern = lanes[t].pattern;
			part.stepCount = (int) params[laneParam(STEP_COUNT_PARAM, t)].getValue();
			part.stepLength = (int) params[laneParam(STEP_LENGTH_PARAM, t)].getValue();
			part.noteLength = (int) params[laneParam(LENGTH_PARAM, t)].getValue();
			part.quantize = params[laneParam(QUANTIZE_PARAM, t)].getValue();
			part.channels = lanes[t].settings.channels;
			Track* e = engine(t);
			part.hasSound = e;
			for (int i = 0; e && i < SOUND_PARAMS; i++)
				part.sound[i] = e->paramQuantities[i]->getScaledValue();
		}
		scene.muteGroup = muteGroup;
		scene.used = true;
	}

	/** A scene into the pattern playing, leaving the notes sounding to ring. Engines take the dials it saved; a track
	saved without an engine leaves its engine's dials as they are. A scene's values are always in range, since
	projectFromJson and opzfile::read clamp what they read. */
	void recallScene(const Project::Scene& scene) {
		for (int t = 0; t < LANES; t++) {
			const Project::Part& part = scene.parts[t];
			lanes[t].pattern = part.pattern;
			lanes[t].settings.channels = part.channels;
			params[laneParam(STEP_COUNT_PARAM, t)].setValue(part.stepCount);
			params[laneParam(STEP_LENGTH_PARAM, t)].setValue(part.stepLength);
			params[laneParam(LENGTH_PARAM, t)].setValue(part.noteLength);
			params[laneParam(QUANTIZE_PARAM, t)].setValue(part.quantize);
			Track* e = engine(t);
			for (int i = 0; e && part.hasSound && i < SOUND_PARAMS; i++) {
				ParamQuantity* q = e->paramQuantities[i];
				float v = math::rescale(part.sound[i], 0.f, 1.f, q->getMinValue(), q->getMaxValue());
				e->params[i].setValue(q->snapEnabled ? std::round(v) : v);
			}
		}
		muteGroup = scene.muteGroup;
		readSettings();
		held = -1;
		selection = 0;
		component = -1;
	}

	/** The live project into a slot: its patterns and chains, its settings and the mixer. */
	void storeProject(Project& p) {
		std::copy(std::begin(scenes), std::end(scenes), p.scenes);
		storeScene(p.scenes[pattern]);
		p.pattern = pattern;
		p.chain = chain;
		std::copy(std::begin(chains), std::end(chains), p.chains);
		p.masterStyle = (int) params[MASTER_STYLE_PARAM].getValue();
		p.transposed = transposed;
		p.tempo = params[TEMPO_PARAM].getValue();
		p.swing = params[SWING_PARAM].getValue();
		for (int l = 0; l < LEVELS_LEN; l++)
			p.levels[l] = params[LEVEL_PARAM + l].getValue();
		p.metronomeSound = (int) params[METRONOME_SOUND_PARAM].getValue();
		p.metronomeLevel = params[METRONOME_LEVEL_PARAM].getValue();
		std::copy(std::begin(mutes), std::end(mutes), p.mutes);
		std::copy(std::begin(audioMutes), std::end(audioMutes), p.audioMutes);
		p.used = true;
	}

	/** A slot into the live project, on the pattern it was playing, its notes let go of. */
	void recallProject(const Project& p) {
		std::copy(std::begin(p.scenes), std::end(p.scenes), scenes);
		pattern = clamp(p.pattern, 0, PATTERN_COUNT - 1);
		chain = p.chain.length ? p.chain : Project::Chain::of(pattern);
		std::copy(std::begin(p.chains), std::end(p.chains), chains);
		const uint8_t* at = std::find(chain.patterns, chain.patterns + chain.length, pattern);
		chainAt = at < chain.patterns + chain.length ? int(at - chain.patterns) : 0;
		chaining = copying = false;
		recallScene(scenes[pattern]);
		for (Lane& l : lanes)
			l.player.silence();
		params[MASTER_STYLE_PARAM].setValue(p.masterStyle);
		transposed = p.transposed;
		masterNote = -1;
		params[TEMPO_PARAM].setValue(std::round(p.tempo));
		params[SWING_PARAM].setValue(p.swing);
		for (int l = 0; l < LEVELS_LEN; l++)
			params[LEVEL_PARAM + l].setValue(p.levels[l]);
		params[METRONOME_SOUND_PARAM].setValue(p.metronomeSound);
		params[METRONOME_LEVEL_PARAM].setValue(p.metronomeLevel);
		std::copy(std::begin(p.mutes), std::end(p.mutes), mutes);
		std::copy(std::begin(p.audioMutes), std::end(p.audioMutes), audioMutes);
	}

	/** Plays another of the live project's patterns, the tracks carrying on from where they are. With the slots
	locked. */
	void switchPattern(int q) {
		if (q == pattern)
			return;
		storeScene(scenes[pattern]);
		pattern = q;
		recallScene(scenes[q]);
		recall = std::max(recall, PATTERNED);
	}

	/** A pattern picked on the steps in project mode: a chain of its own, played now, or while chaining, the next in
	a new chain, its first played now. With the slots locked. */
	void pickPattern(int q) {
		if (chaining && chained++ > 0) {
			if (chain.length < CHAIN_LENGTH)
				chain.patterns[chain.length++] = (uint8_t) q;
			return;
		}
		chain = Project::Chain::of(q);
		chainAt = 0;
		switchPattern(q);
	}

	/** What the project mode keys asked for. With the slots locked. */
	void handlePatternRequests() {
		for (int i = 0; i < picked; i++)
			pickPattern(picks[i]);
		picked = 0;
		if (chainKey >= 0 && chainKeySave)
			chains[chainKey] = chain;
		else if (chainKey >= 0 && chains[chainKey].length) {
			chain = chains[chainKey];
			chainAt = 0;
			switchPattern(chain.patterns[0]);
		}
		chainKey = -1;
		if (rewound)
			switchPattern(chain.patterns[chainAt]);
		rewound = false;
		// A paste over the pattern playing is an edit of each track, for the undo history; over another, it isn't.
		for (int i = 0; i < PATTERN_COUNT; i++)
			if (pastes >> i & 1)
				i == pattern ? recallScene(clipboard) : (void) (scenes[i] = clipboard);
		pastes = 0;
		if (clearAll) {
			for (int i = 0; i < PATTERN_COUNT; i++)
				if (i != pattern)
					scenes[i] = blankScene();
			chain = Project::Chain::of(pattern);
			chainAt = 0;
			for (Project::Chain& c : chains)
				c = Project::Chain();
		}
		clearAll = false;
	}

	bool patternRequested() const {
		return picked || chainKey >= 0 || rewound || pastes || clearAll;
	}

	/** How long the pattern playing lasts, in sixteenths: its longest track, of those that step in time, or a bar. */
	double patternLength() const {
		int longest = 0;
		for (const Lane& l : lanes)
			longest = std::max(longest, clamp(l.settings.stepCount, 1, STEPS) * l.settings.stepLength);
		return longest > 0 ? longest : STEPS;
	}

	/** Moves the chain on as the pattern playing ends, `song` being the song time this sample runs to. Every track
	holds back its steps from the end on until the next pattern is in, then they start it together from its first
	step. If the UI thread has the slots locked, the next pattern comes in a moment late rather than the audio thread
	waiting; if the end has passed by more than a sixteenth, as when a track is shortened, it comes in on the next
	sixteenth. A pattern chained to nothing else just loops, each track running on at its own length. */
	void followChain(double song) {
		double length = patternLength(), end = patternStart + length;
		bool moving = chain.length > 1;
		for (Lane& l : lanes)
			l.player.hold = moving ? end : INFINITY;
		if (song < end)
			return;
		if (!moving) {
			patternStart = end + std::floor((song - end) / length) * length;
			return;
		}
		if (!projectsMutex.try_lock())
			return;
		std::lock_guard<std::mutex> lock(projectsMutex, std::adopt_lock);
		double from = lanes[0].player.song(), at = from < end + 1.0 ? end : std::ceil(from);
		chainAt = (chainAt + 1) % chain.length;
		switchPattern(chain.patterns[chainAt]);
		for (Lane& l : lanes)
			l.player.restartPattern(at);
		patternStart = at;
	}

	/** Loads a slot as one step of the undo history. An empty slot is a new project. With the slots locked. */
	void enterProject(int n) {
		storeProject(left);
		leftSlot = project;
		project = n;
		recallProject(projects[n].used ? projects[n] : blank);
		storeProject(entered);
		recall = SWITCHED;
	}

	/** Loads a slot, saving the project left first unless saving is manual. With the slots locked. */
	void switchProject(int n) {
		if (autoSave)
			storeProject(projects[project]);
		enterProject(n);
	}

	/** Saves the live project to a slot, which it then belongs to. With the slots locked. */
	void saveProject(int n) {
		if (n != project)
			projects[n].opz = projects[project].opz;
		storeProject(projects[n]);
		project = n;
	}

	void toggleHold(int i) {
		held = held == i ? -1 : i;
		const Step& s = lane().pattern.steps[i];
		if (held >= 0 && !s.empty()) {
			memory = s;
			if (!playing)
				lane().player.preview(s, lane().settings);
		}
	}

	/** A click on a step that doesn't pick it up. */
	void clickStep(int i) {
		Pattern& pattern = lane().pattern;
		Step& s = pattern.steps[i];
		if (held == i)
			held = -1;
		else if (held >= 0) {
			// The held step's notes last until the end of this one, or go back to the default if they already do.
			Step& h = pattern.steps[held];
			int loop = held < stepCount() ? stepCount() : STEPS, span = i - held;
			if (span < 0)
				span += loop;
			int ticks = (span + 1) * TICKS;
			bool same = std::all_of(h.notes, h.notes + h.count, [=](const Note& n) { return n.length == ticks; });
			for (int n = 0; n < h.count; n++)
				h.notes[n].length = same ? 0 : ticks;
		}
		else if (recording && !playing)
			cursor = i;
		else if (s.empty())
			s.paste(memory);
		else
			s.clear();
	}

	void shift(int direction) {
		if (tempoMode)
			params[TEMPO_PARAM].setValue(clamp(params[TEMPO_PARAM].getValue() + direction, 40.f, 200.f));
		else if (trackMode)
			lane().pattern.rotate(direction, stepCount());
		else if (held >= 0)
			lane().pattern.steps[held].nudge(direction);
		else if (direction < 0 && recording && playing && anyHeld())
			erasing = true;
		else
			octave = clamp(octave + direction, -3, 3);
	}

	/** A live note starts: it plays, and goes into the held step, the record, or the notes a click pastes. */
	void noteDown(int source, int pitch, float volts, int velocity) {
		bool first = !anyHeld();
		Lane& l = lane();
		Live& n = live[source];
		n = Live();
		n.on = true;
		n.lane = track;
		n.pitch = pitch = clamp(pitch, 0, 127);
		n.id = l.player.noteOn(volts, velocity / 127.f, INFINITY, true, l.settings);
		Note note;
		note.pitch = pitch;
		note.velocity = clamp(velocity, 1, 127);
		if (held >= 0) {
			Step& s = l.pattern.steps[held];
			if (s.indexOf(pitch) >= 0)
				s.remove(pitch);
			else
				s.add(note);
			if (!s.empty())
				memory = s;
			return;
		}
		if (first)
			memory = Step();
		memory.add(note);
		if (!recording || erasing)
			return;
		if (playing) {
			int step, ticks;
			if (l.player.locate(&step, &ticks)) {
				note.offset = ticks;
				if (l.pattern.steps[step].add(note)) {
					n.recorded = step;
					n.start = l.player.now;
					n.slot = l.player.stepLength();
				}
			}
		}
		else {
			if (!chord)
				l.pattern.steps[cursor].clear();
			chord = true;
			l.pattern.steps[cursor].add(note);
		}
	}

	void noteUp(int source) {
		Live& n = live[source];
		if (!n.on)
			return;
		n.on = false;
		Lane& l = lanes[n.lane];
		l.player.release(n.id);
		if (n.recorded >= 0) {
			Step& s = l.pattern.steps[n.recorded];
			int i = s.indexOf(n.pitch);
			if (i >= 0)
				s.notes[i].length = (int16_t) std::clamp(std::lround((l.player.now - n.start) / n.slot * TICKS), 1L, long(DRONE - 1));
		}
		if (chord && !anyHeld()) {
			chord = false;
			cursor = (cursor + 1) % stepCount();
		}
	}

	/** In tempo mode the value keys type a tempo, three digits as on the OP-Z, and the white keys tap one. */
	void tempoKey(int key) {
		int value = keyValue(key);
		if (value >= 0) {
			typed = 10 * typed + (value + 1) % 10;
			if (++digits == 3) {
				params[TEMPO_PARAM].setValue(clamp(typed, 40, 200));
				typed = digits = 0;
			}
			return;
		}
		if (sinceTap > 0.2f && sinceTap < 1.5f) {
			tapPeriod = tapPeriod > 0.f ? 0.5f * (tapPeriod + sinceTap) : sinceTap;
			params[TEMPO_PARAM].setValue(clamp(std::round(60.f / tapPeriod), 40.f, 200.f));
		}
		else
			tapPeriod = 0.f;
		sinceTap = 0.f;
	}

	/** With SHIFT latched, a white key puts its component on the picked steps, at a value that does something, or
	pressed again once they all have it, takes it off. A black key sets the value of the component picked last;
	pressing a spark's value again cycles which of its passes fire. */
	void componentKeyDown(int key) {
		Pattern& pattern = lane().pattern;
		auto picked = [&](auto f) {
			for (int i = 0; i < STEPS; i++)
				if (selection >> i & 1)
					f(pattern.steps[i]);
		};
		int c = keyComponent(key), value = keyValue(key);
		if (c >= 0) {
			bool all = selection != 0;
			picked([&](Step& s) { all = all && s.has(c); });
			if (all && component == c) {
				picked([&](Step& s) { s.unset(c); });
				component = -1;
			}
			else {
				picked([&](Step& s) {
					if (!s.has(c))
						s.set(c, COMPONENT_DEFAULTS[c]);
				});
				component = c;
			}
		}
		else if (component >= 0) {
			picked([&](Step& s) {
				bool again = isSpark(component) && s.has(component) && s.key(component) == value && value < 8;
				s.set(component, again ? (s.values[component] + 16) % 64 : value);
			});
		}
	}

	void keyDown(int key) {
		if (editingComponents())
			componentKeyDown(key);
		else if (tempoMode)
			tempoKey(key);
		else if (mixerMode) {
			if (keyValue(key) >= 0)
				muteGroup = keyValue(key);
		}
		else if (!trackMode)
			noteDown(key, keyPitch(key), (keyPitch(key) - 60) / 12.f, 100);
		else if (keyValue(key) >= 0)
			params[laneParam(STEP_LENGTH_PARAM, track)].setValue(keyValue(key));
	}

	/** The tracks' settings from their params. */
	void readSettings() {
		for (int t = 0; t < LANES; t++) {
			Settings& s = lanes[t].settings;
			s.stepCount = (int) params[laneParam(STEP_COUNT_PARAM, t)].getValue();
			s.stepLength = stepLengthOfKey((int) params[laneParam(STEP_LENGTH_PARAM, t)].getValue());
			s.noteLength = NOTE_LENGTHS[(int) params[laneParam(LENGTH_PARAM, t)].getValue()];
			s.quantize = params[laneParam(QUANTIZE_PARAM, t)].getValue();
			s.swing = params[SWING_PARAM].getValue();
		}
	}

	void handleControls(float dt) {
		readSettings();
		// The arp track's engine sends its arp dials back; without one, the arp is off.
		Arp& arp = lanes[ARP_TRACK].settings.arp;
		arp.speed = clamp((int) engines.arp[0], 0, ARP_SPEEDS);
		arp.pattern = clamp((int) engines.arp[1], 0, ARP_PATTERNS - 1);
		arp.style = clamp((int) engines.arp[2], 0, ARP_STYLES - 1);
		arp.range = clamp(engines.arp[3], 0.f, 1.f);
		// Latch plays the master track's notes on the steps, as the OP-Z does.
		if (params[MASTER_STYLE_PARAM].getValue() == LATCH)
			lanes[MASTER].settings.quantize = 1.f;
		if (sinceTap >= 0.f)
			sinceTap += dt;
		if ((reload >= 0 || restoring >= 0 || keyed >= 0 || patternRequested()) && projectsMutex.try_lock()) {
			std::lock_guard<std::mutex> lock(projectsMutex, std::adopt_lock);
			if (int r = reload.exchange(-1); r >= 0)
				enterProject(r);
			if (int r = restoring.exchange(-1); r >= 0) {
				project = r;
				recallProject(restored);
				recall = std::max(recall, RESTORED);
			}
			if (keyed >= 0)
				keyedSave ? saveProject(keyed) : switchProject(keyed);
			keyed = -1;
			handlePatternRequests();
		}

		for (int k = 0; k < ALL_KEYS; k++) {
			KeyGesture& g = gestures[k];
			if (KeyGesture::Event e = g.process(params[keyParam(k)].getValue(), dt, LONG_PRESS))
				onKey(keyParam(k), e, g);
		}
		if (clearing >= 0.f) {
			const KeyGesture& stop = gestures[keyIndex(STOP_PARAM)];
			if (!stop.down() || !(trackMode || projectMode))
				clearing = -1.f;
			else if ((clearing = stop.held) >= CLEAR_HOLD) {
				for (int t = 0; t < LANES; t++)
					if (projectMode || t == track)
						lanes[t].pattern.clear();
				clearAll = projectMode && shifted;
				clearing = -1.f;
			}
		}

		if (erasing) {
			int step, ticks;
			if (!recording || !playing || !anyHeld())
				erasing = false;
			else if (lane().player.locate(&step, &ticks))
				for (const Live& n : live)
					if (n.on)
						lane().pattern.steps[step].remove(n.pitch);
		}
	}

	/** Whether this press picks a key up: a long press, or a Ctrl+click, as the context menu allows. */
	bool picksUp(KeyGesture::Event e, const KeyGesture& g) const {
		return !g.handled && (e == KeyGesture::PRESS ? g.press == KEY_CTRL && ctrlClickHolds : e == KeyGesture::LONG_PRESS && longPressHolds);
	}

	/** A step acts when it's let go, so the module can tell a click from a long press, which picks it up as soon as it's
	long enough; a Ctrl+click picks it up straight away. SHIFT does the same. The other keys act as they go down. */
	void onKey(int p, KeyGesture::Event e, KeyGesture& g) {
		if (p == MASTER_PARAM) {
			if (e == KeyGesture::PRESS)
				select(MASTER);
		}
		else if (p >= PAGE_PARAM) {
			if (e == KeyGesture::PRESS)
				requestPage(p - PAGE_PARAM);
		}
		else if (p >= SELECT_PARAM) {
			if (e == KeyGesture::PRESS)
				trackKey(p - SELECT_PARAM);
		}
		else if (p >= KEY_PARAM) {
			int key = p - KEY_PARAM;
			if (projectMode) {
				// A click loads a project, or on a white key plays its chain; a long press or a Ctrl+click saves.
				if (keyComponent(key) >= 0) {
					if (picksUp(e, g) || e == KeyGesture::CLICK) {
						chainKey = keyComponent(key);
						chainKeySave = picksUp(e, g);
						g.handled = true;
					}
					return;
				}
				if (keyValue(key) < 0)
					return;
				if (picksUp(e, g)) {
					keyed = keyValue(key);
					keyedSave = true;
					g.handled = true;
				}
				else if (e == KeyGesture::CLICK) {
					keyed = keyValue(key);
					keyedSave = false;
				}
			}
			else if (e == KeyGesture::PRESS)
				keyDown(key);
			else if (e == KeyGesture::CLICK || e == KeyGesture::RELEASE)
				noteUp(key);
		}
		else if (p >= STEP_PARAM) {
			int i = p - STEP_PARAM, t = stepLane(i);
			if (mixerMode) {
				// Steps 1 to 8 mute their tracks in the mute group, or with SHIFT latched, only their sound. Step 12
				// mutes the master track, which stops its transposing.
				if (e == KeyGesture::PRESS && t >= 0)
					(shifted && t < TRACKS ? audioMutes : mutes)[muteGroup] ^= 1 << t;
			}
			else if (projectMode) {
				// A step plays its pattern, adds it to the chain being built, or takes the pattern copied.
				if (e != KeyGesture::PRESS)
					return;
				if (copying)
					pastes |= 1 << i;
				else if (picked < CHAIN_LENGTH)
					picks[picked++] = i;
			}
			else if (trackMode) {
				if (e != KeyGesture::PRESS)
					return;
				if (shifted)
					params[laneParam(STEP_COUNT_PARAM, track)].setValue(i + 1);
				else if (t >= 0)
					select(t);
			}
			else if (editingComponents()) {
				if (e == KeyGesture::PRESS) {
					selection ^= 1 << i;
					component = -1;
				}
			}
			else if (picksUp(e, g)) {
				toggleHold(i);
				g.handled = true;
			}
			else if (e == KeyGesture::CLICK)
				clickStep(i);
		}
		else if (p == SHIFT_PARAM) {
			if (picksUp(e, g)) {
				shifted = !shifted;
				g.handled = true;
			}
			else if (e == KeyGesture::CLICK) {
				if (shifted)
					shifted = false;
				else if (projectMode) {
					// Copies the pattern playing, for the steps to paste, or puts the copy away.
					copying = !copying;
					if (copying)
						storeScene(clipboard);
				}
				else
					requestPage(NEXT_PAGE);
			}
			if (!shifted) {
				selection = 0;
				component = -1;
			}
		}
		else if (e == KeyGesture::PRESS) {
			switch (p) {
				case TRACK_PARAM: toggleMode(&Sequencer::trackMode); break;
				case PROJECT_PARAM: toggleMode(&Sequencer::projectMode); break;
				case MIXER_PARAM: toggleMode(&Sequencer::mixerMode); break;
				case METRONOME_PARAM: toggleMode(&Sequencer::tempoMode); break;
				case REC_PARAM: recording = !recording; cursor = 0; break;
				case PLAY_PARAM:
					if (projectMode) {
						chaining = !chaining;
						chained = 0;
					}
					else
						play();
					break;
				case STOP_PARAM:
					if (editingComponents()) {
						for (Step& s : lane().pattern.steps)
							s.clearComponents();
						break;
					}
					if (!projectMode)
						stop();
					clearing = trackMode || projectMode ? 0.f : -1.f;
					break;
				case OCTAVE_DOWN_PARAM: shift(-1); break;
				case OCTAVE_UP_PARAM: shift(1); break;
			}
		}
	}

	/** The GATE input's channels play like keys, and while held their notes follow V/OCT. */
	void handleInputs() {
		int channels = inputs[GATE_INPUT].getChannels();
		for (int c = 0; c < CHANNELS; c++) {
			Live& n = live[KEYS + c];
			float volts = inputs[VOCT_INPUT].getPolyVoltage(c);
			switch (gates[c].processEvent(c < channels ? inputs[GATE_INPUT].getVoltage(c) : 0.f, 0.1f, 1.f)) {
				case dsp::SchmittTrigger::TRIGGERED: {
					int velocity = inputs[VELOCITY_INPUT].isConnected() ? (int) std::round(inputs[VELOCITY_INPUT].getPolyVoltage(c) * 12.7f) : 100;
					noteDown(KEYS + c, 60 + (int) std::round(volts * 12.f), volts, velocity);
					break;
				}
				case dsp::SchmittTrigger::UNTRIGGERED:
					noteUp(KEYS + c);
					break;
				default:
					if (n.on)
						if (Player::Channel* channel = lanes[n.lane].player.find(n.id))
							channel->pitch = channel->target = volts;
			}
		}
	}

	/** RUN plays from the start while high and stops when it falls, CONT plays on, and RESET starts over. */
	void handleTransport() {
		switch (runIn.processEvent(inputs[RUN_INPUT].getVoltage(), 0.1f, 1.f)) {
			case dsp::SchmittTrigger::TRIGGERED: play(); break;
			case dsp::SchmittTrigger::UNTRIGGERED: if (playing) halt(); break;
			default: break;
		}
		if (contIn.process(inputs[CONT_INPUT].getVoltage(), 0.1f, 1.f) && !playing)
			play(true);
		if (resetIn.process(inputs[RESET_INPUT].getVoltage(), 0.1f, 1.f) && playing)
			play();
		if (advanceIn.process(inputs[ADVANCE_INPUT].getVoltage(), 0.1f, 1.f))
			advanceWaiting();
	}

	/** Steps on every track that waits for triggers. */
	void advanceWaiting() {
		for (Lane& l : lanes)
			if (l.settings.stepLength == 0)
				l.player.trigger(l.pattern, l.settings);
	}

	/** The key and mode of the bass, lead, arp and chord tracks, as the OP-Z hears them: every note on the steps that
	play counts towards the scale, and each step's lowest note towards the tonic, the bass's twice. */
	Scale findScale() const {
		int notes[12] = {}, tonics[12] = {};
		for (int t = DRUM_TRACKS; t < TRACKS; t++) {
			const Lane& l = lanes[t];
			int weight = t == DRUM_TRACKS ? 2 : 1;
			for (int i = 0; i < clamp(l.settings.stepCount, 1, STEPS); i++) {
				const Step& s = l.pattern.steps[i];
				int lowest = 127;
				for (int n = 0; n < s.count; n++) {
					notes[s.notes[n].pitch % 12] += weight;
					lowest = std::min(lowest, (int) s.notes[n].pitch);
				}
				if (s.count)
					tonics[lowest % 12] += weight;
			}
		}
		return detect(notes, tonics);
	}

	/** Follows the master track's notes. In latch style, the lowest of the notes that started together last moves the
	tracks until the next ones, or until the start of its pattern or a stop, unless it's a key still held; in free
	style, the lowest note sounding does. */
	void followMaster() {
		const Player& p = lanes[MASTER].player;
		bool free = params[MASTER_STYLE_PARAM].getValue() == FREE;
		int step = p.running ? p.playing() : -1, newest = 128, lowest = 128, keys = 128;
		if (!free && p.notes == masterHeard && step == masterStep)
			return;
		for (const Player::Channel& c : p.channels) {
			if (!c.gate)
				continue;
			int pitch = 60 + (int) std::round(c.target * 12.f);
			lowest = std::min(lowest, pitch);
			if (c.live)
				keys = std::min(keys, pitch);
			if (c.id > masterHeard)
				newest = std::min(newest, pitch);
		}
		masterHeard = p.notes;
		if (step != masterStep && step <= 0)
			masterNote = keys < 128 ? keys : -1;
		masterStep = step;
		if (free)
			masterNote = lowest < 128 ? lowest : -1;
		else if (newest < 128)
			masterNote = newest;
	}

	/** Runs the players at the tempo, or follows the clock: between pulses at the rate they last came, but never past
	where the next one is due, and on a pulse straight to it. Every lane keeps the same time, so the first one's is
	everyone's. */
	void runClock(float dt) {
		bool patched = inputs[CLOCK_INPUT].isConnected();
		if (patched != external) {
			external = patched;
			pulsePeriod = 0.f;
			if (playing)
				play();
		}
		const Player& time = lanes[0].player;
		double perPulse = 4.0 / ppqn;
		double rate = external && pulsePeriod > 0.f ? perPulse / pulsePeriod : params[TEMPO_PARAM].getValue() / 15.0;
		double d = rate * dt;
		if (external) {
			sincePulse += dt;
			if (clockIn.process(inputs[CLOCK_INPUT].getVoltage(), 0.1f, 1.f)) {
				// The first pulse after a long gap only starts the count again.
				pulsePeriod = sincePulse * ppqn < 4.f ? sincePulse : 0.f;
				sincePulse = 0.f;
				if (armed) {
					armed = false;
					resuming ? resume() : restart();
				}
				else if (time.running)
					pulses++;
			}
			if (time.running) {
				double due = pulses * perPulse, song = time.song();
				d = song < due ? due - song : pulsePeriod > 0.f ? std::min(d, due + perPulse - 1e-9 - song) : 0.0;
				d = std::max(d, 0.0);
			}
		}
		if (time.running)
			followChain(time.song() + d);
		bool gateStep = false;
		auto run = [&](Lane& l) {
			l.player.advance(d, l.pattern, l.settings);
			gateStep |= l.player.gateStep;
			l.player.gateStep = false;
		};
		// The master track first, so notes starting with one of its own already follow it.
		lanes[MASTER].player.tonality = Tonality{scale};
		run(lanes[MASTER]);
		followMaster();
		int note = muted(MASTER) ? -1 : masterNote;
		if (note != movedBy || transposition.scale.key != scale.key || transposition.scale.mode != scale.mode) {
			movedBy = note;
			transposition = note >= 0 ? masterMove(scale, note) : Tonality{scale};
		}
		for (int t = 0; t < TRACKS; t++) {
			lanes[t].player.tonality = transposed >> t & 1 ? transposition : Tonality{scale};
			run(lanes[t]);
		}
		for (int t = FX1; t < LANES; t++) {
			lanes[t].player.tonality = Tonality{scale};
			run(lanes[t]);
		}
		// A jump's gate step advances the tracks that wait for triggers, as ADV does.
		if (gateStep)
			advanceWaiting();
		if (time.running) {
			long tick = (long) std::floor(time.song() / perPulse + 1e-9);
			if (tick != lastTick) {
				lastTick = tick;
				clockOut.trigger(1e-3f);
			}
			long beat = (long) std::floor(time.song() / 4.0 + 1e-9);
			if (beat != lastBeat) {
				lastBeat = beat;
				metronome.trigger((int) params[METRONOME_SOUND_PARAM].getValue(), beat % 4 == 0);
			}
		}
	}

	/** Each lane to the bus for its engine, then the mix and the transport to the jacks. */
	void writeOutputs(float dt) {
		Downstream* bus = toRight(this);
		for (int t = 0; t < LANES; t++) {
			if (t == MASTER)
				continue;
			Lane& l = lanes[t];
			int n = clamp(l.settings.channels, 1, CHANNELS);
			if (bus) {
				Downstream::Lane& out = bus->lanes[t];
				out.channels = n;
				// The FX tracks have no group level: muted, their effect is.
				float group = t >= FX1 ? 1.f : params[LEVEL_PARAM + (t < DRUM_TRACKS ? DRUM_LEVEL : SYNTH_LEVEL)].getValue();
				out.level = audioMuted(t) || (t >= FX1 && muted(t)) ? 0.f : group * group;
				// The playhead's step locks the engine's dials; while stopped, a held step does, to hear it.
				int s = l.player.running ? l.player.locking() : t == track && !modal() ? held : -1;
				const Step* step = s >= 0 ? &l.pattern.steps[s] : nullptr;
				out.locks = step ? step->locks : 0;
				if (step)
					std::copy(std::begin(step->lockValues), std::end(step->lockValues), out.lockValues);
				const Player::Channel* voices = l.player.voices();
				bool mute = muted(t);
				for (int c = 0; c < n; c++) {
					const Player::Channel& channel = voices[c];
					out.pitch[c] = channel.pitch;
					out.velocity[c] = channel.velocity;
					out.gate[c] = channel.gate && !channel.retrigger && !mute;
				}
				for (int k = 0; k < SWEEP_TARGETS; k++)
					out.sweep[k] = l.player.sweep(k);
			}
			for (int c = 0; c < n; c++)
				l.player.channels[c].retrigger = l.player.arped[c].retrigger = false;
		}
		if (bus) {
			bus->secondsPerBeat = 60.f / bpm();
			bus->selected = track;
			bus->trackMode = trackMode;
			bus->page = page;
			bus->pageRequests = pageRequests;
			bus->drums = bus->synths = bus->effects = 0;
			std::copy(std::begin(engines.send), std::end(engines.send), bus->send);
		}
		const Upstream* up = fromRight(this);
		engines = up ? *up : Upstream();
		// The mix: punch, the metronome while playing, then the master level.
		float left = engines.left, right = engines.right;
		punch.process(left, right, params[LEVEL_PARAM + PUNCH].getValue(), dt);
		float level = params[METRONOME_LEVEL_PARAM].getValue(), click = metronome.process(dt) * 5.f * level * level;
		float master = params[LEVEL_PARAM + MASTER_LEVEL].getValue();
		outputs[LEFT_OUTPUT].setVoltage(softLimit((left + click) * master * master));
		outputs[RIGHT_OUTPUT].setVoltage(softLimit((right + click) * master * master));
		outputs[CLOCK_OUTPUT].setVoltage(clockOut.process(dt) ? 10.f : 0.f);
		outputs[RUN_OUTPUT].setVoltage(playing && !armed ? 10.f : 0.f);
		outputs[RESET_OUTPUT].setVoltage(resetOut.process(dt) ? 10.f : 0.f);
		outputs[CONT_OUTPUT].setVoltage(contOut.process(dt) ? 10.f : 0.f);
	}

	void process(const ProcessArgs& args) override {
		if (controlDivider.process())
			handleControls(args.sampleTime * controlDivider.getDivision());
		if (scaleDivider.process())
			scale = findScale();
		handleInputs();
		handleTransport();
		runClock(args.sampleTime);
		writeOutputs(args.sampleTime);
		if (lightDivider.process())
			updateLights(args.sampleTime * lightDivider.getDivision());
	}

	/** How many steps the held step's longest note reaches over, itself included. */
	int heldSpan(int loop) {
		const Step& s = lane().pattern.steps[held];
		int span = 0;
		for (int n = 0; n < s.count; n++) {
			int ticks = s.notes[n].length ? s.notes[n].length : lane().settings.noteLength;
			span = std::max(span, ticks == DRONE ? loop : (ticks + TICKS - 1) / TICKS);
		}
		return std::min(span, loop);
	}

	void setRgb(int light, NVGcolor c, float brightness) {
		lights[light].setBrightness(c.r * brightness);
		lights[light + 1].setBrightness(c.g * brightness);
		lights[light + 2].setBrightness(c.b * brightness);
	}

	void setKeyLight(int param, NVGcolor c, float brightness) {
		setRgb(keyLight(param), c, brightness);
	}

	static bool sounding(const Lane& l) {
		return std::any_of(l.player.channels, l.player.channels + clamp(l.settings.channels, 1, CHANNELS), [](const Player::Channel& c) { return c.gate; });
	}

	/** Whether a track's key lights as busy: a track while its notes play, the master track while it moves the others. */
	bool busy(int t) const {
		return t == MASTER ? masterNote >= 0 && !muted(MASTER) : sounding(lanes[t]);
	}

	/** A dial's value from 0 to 1 as its LED shows it, or -1 for a dial with nothing to do. */
	float dialValue(int d) {
		std::pair<Module*, int> target = dialTarget(d);
		if (target.first == this)
			return paramQuantities[target.second]->getScaledValue();
		return projectMode ? -1.f : engines.dials[d];
	}

	void updateLights(float dt) {
		blink = std::fmod(blink + dt, 0.5f);
		// The battery: it runs down while playing and charges while stopped, and SCREEN shows it a moment longer than
		// it's held.
		battery = playing ? std::fmax(battery - dt / 18000.f, 0.05f) : std::fmin(battery + dt / 3600.f, 1.f);
		batteryShown = gestures[SCREEN_PARAM - TRACK_PARAM].down() ? 1.f : std::fmax(batteryShown - dt, 0.f);
		bool on = blink < 0.25f;
		Lane& l = lane();
		int count = stepCount(), head = l.player.running ? l.player.playing() : -1;
		int loop = held < count ? count : STEPS, span = held >= 0 ? heldSpan(loop) : 0;
		bool beat = l.player.running && std::fmod(l.player.song(), 4.0) < 1.0;

		// Steps: orange for notes, white for the playhead (red while recording), blue for how long the held step's
		// notes last. In track mode, the tracks on the first eight, white for the one selected and orange while one
		// plays; with SHIFT, white up to the step count. Holding STOP there fills them red until the pattern clears.
		for (int i = 0; i < STEPS; i++) {
			NVGcolor c = led::WHITE;
			float b = 0.f;
			if (batteryShown > 0.f) {
				c = led::GREEN;
				b = i < std::ceil(battery * STEPS);
			}
			else if (clearing >= 0.f) {
				c = led::RED;
				b = i < clearing / CLEAR_HOLD * STEPS;
			}
			else if (mixerMode) {
				// Lit tracks play, dark ones are muted, red ones play with their sound muted. Step 12 is the master track.
				if (int t = stepLane(i); t >= 0) {
					c = audioMuted(t) ? led::RED : led::WHITE;
					b = muted(t) ? 0.f : audioMuted(t) ? 0.8f : shifted ? 0.3f : 1.f;
				}
			}
			else if (projectMode) {
				// The patterns: white for the one playing, green for the rest of its chain, dim for the others with
				// notes. Copying, they all blink, to take the copy.
				bool inChain = std::find(chain.patterns, chain.patterns + chain.length, i) < chain.patterns + chain.length;
				if (i == pattern)
					b = 1.f;
				else if (inChain && chain.length > 1) {
					c = led::GREEN;
					b = 0.6f;
				}
				else
					b = scenes[i].empty() ? 0.f : 0.2f;
				if (copying && !on)
					b = std::max(b * 0.3f, 0.1f);
			}
			else if (trackMode && shifted)
				b = i == count - 1 ? 1.f : i < count ? 0.15f : 0.f;
			else if (trackMode) {
				int t = stepLane(i);
				if (t >= 0 && t == track)
					b = 1.f;
				else if (t >= 0 && busy(t)) {
					c = NOTE_COLOR;
					b = 0.6f;
				}
			}
			else if (editingComponents()) {
				// Green for steps with components, blinking for the ones picked.
				const Step& s = l.pattern.steps[i];
				c = s.components || selection >> i & 1 ? led::GREEN : NOTE_COLOR;
				b = selection >> i & 1 ? (on ? 1.f : 0.3f) : s.components ? 0.4f : s.empty() ? 0.f : 0.2f;
				if (i == head) {
					c = led::WHITE;
					b = 1.f;
				}
			}
			else {
				if (!l.pattern.steps[i].empty()) {
					c = NOTE_COLOR;
					b = i < count ? 0.6f : 0.2f;
				}
				int d = i - held;
				if (d < 0)
					d += loop;
				if (held >= 0 && i < loop && d > 0 && d < span) {
					c = led::BLUE;
					b = 0.8f;
				}
				if (i == held)
					b = on ? std::max(b, 0.4f) : 0.f;
				if (recording && !playing && i == cursor) {
					c = led::RED;
					b = on;
				}
				if (i == head) {
					c = recording ? led::RED : led::WHITE;
					b = 1.f;
				}
			}
			setKeyLight(STEP_PARAM + i, c, b);
		}

		// Keys: the notes sounding, brighter for the ones held or on the held step. In track mode, the step length, in
		// blue like the held step's length on the steps; in tempo mode, the value keys that type it.
		float level[KEYS] = {};
		auto light = [&](int pitch, float b) {
			int k = pitch - keyPitch(0);
			if (k >= 0 && k < KEYS)
				level[k] = std::max(level[k], b);
		};
		NVGcolor keyColor = NOTE_COLOR;
		const Player::Channel* voices = l.player.voices();
		if (trackMode) {
			keyColor = led::BLUE;
			for (int v = 0; v < 10; v++)
				level[valueKey(v)] = v == (int) params[laneParam(STEP_LENGTH_PARAM, track)].getValue() ? 1.f : 0.15f;
		}
		else if (tempoMode) {
			keyColor = led::WHITE;
			for (int v = 0; v < 10; v++)
				level[valueKey(v)] = 0.2f;
		}
		else if (projectMode) {
			keyColor = led::WHITE;
			for (int v = 0; v < PROJECTS; v++)
				level[valueKey(v)] = v == project ? 1.f : projects[v].used ? 0.3f : 0.05f;
			for (int c = 0; c < SAVED_CHAINS; c++)
				level[componentKey(c)] = !chains[c].length ? 0.f : chains[c] == chain ? 1.f : 0.3f;
		}
		else if (mixerMode) {
			keyColor = led::WHITE;
			for (int v = 0; v < MUTE_GROUPS; v++)
				level[valueKey(v)] = v == muteGroup ? 1.f : mutes[v] || audioMutes[v] ? 0.3f : 0.08f;
		}
		else if (editingComponents()) {
			// The white keys light the components on the picked steps, or on any step while none is picked, and the
			// one picked brightest; the black keys, its value.
			keyColor = led::GREEN;
			uint16_t present = 0;
			int value = -1;
			for (int i = 0; i < STEPS; i++) {
				const Step& s = l.pattern.steps[i];
				if (!selection || selection >> i & 1)
					present |= s.components;
				if (selection >> i & 1 && component >= 0 && s.has(component) && value < 0)
					value = s.key(component);
			}
			for (int c = 0; c < COMPONENTS_LEN; c++)
				level[componentKey(c)] = c == component ? 1.f : present >> c & 1 ? (selection ? 0.5f : 0.25f) : 0.f;
			if (component >= 0)
				for (int v = 0; v < 10; v++)
					level[valueKey(v)] = v == value ? 1.f : 0.12f;
		}
		else {
			// On the master track, the scale dimly, its tonic brighter, and the note moving the tracks brighter still.
			if (track == MASTER) {
				for (int k = 0; k < KEYS; k++)
					level[k] = keyPitch(k) % 12 == scale.tonic() ? 0.3f : scale.contains(keyPitch(k)) ? 0.1f : 0.f;
				if (masterNote >= 0)
					light(masterNote, 0.6f);
			}
			for (int c = 0; c < CHANNELS; c++)
				if (voices[c].gate)
					light(60 + (int) std::round(voices[c].pitch * 12.f), 0.4f);
			// On an FX track, the key that set its effect's time.
			if (track >= FX1 && engines.timeKey[track - FX1] >= 0)
				light(FX_LOWEST + engines.timeKey[track - FX1], 0.3f);
			for (const Live& n : live)
				if (n.on && n.lane == track)
					light(n.pitch, 1.f);
			if (held >= 0)
				for (int n = 0; n < l.pattern.steps[held].count; n++)
					light(l.pattern.steps[held].notes[n].pitch, 1.f);
		}
		for (int k = 0; k < KEYS; k++)
			setKeyLight(KEY_PARAM + k, keyColor, level[k]);
		for (int c = 0; c < CHANNELS; c++) {
			const Player::Channel& channel = voices[c];
			lights[CHANNEL_LIGHT + c].setBrightness(channel.gate && c < l.settings.channels ? 0.2f + 0.8f * channel.velocity : 0.f);
		}

		// The track keys: white for the one selected, orange while one is busy, or on the master track with SHIFT
		// latched, green for the tracks it transposes. The page keys light in their colours, brightly for the page the
		// selected track's engine is on.
		for (int t = 0; t < FX1; t++) {
			int key = t == MASTER ? MASTER_PARAM : SELECT_PARAM + t;
			if (choosingTransposed() && t < TRACKS)
				setKeyLight(key, led::GREEN, transposed >> t & 1);
			else
				setKeyLight(key, t == track ? led::WHITE : NOTE_COLOR, t == track ? 1.f : busy(t) ? 0.8f : 0.f);
		}
		int shown = modal() || track == MASTER ? -1 : engines.page[track];
		for (int p = 0; p < PAGES; p++)
			setKeyLight(PAGE_PARAM + p, PAGE_COLORS[p], shown < 0 ? 0.f : p == shown ? 1.f : 0.12f);
		for (int d = 0; d < 4; d++) {
			float v = dialValue(d);
			bool locked = static_cast<LockQuantity*>(paramQuantities[LOCK_PARAM + d])->locked();
			setRgb(DIAL_LIGHT + 3 * d, shown >= 0 && !locked ? PAGE_COLORS[shown] : led::WHITE, v < 0.f ? 0.f : 0.15f + 0.85f * v);
		}

		setKeyLight(TRACK_PARAM, led::WHITE, trackMode);
		setKeyLight(REC_PARAM, led::RED, recording);
		setKeyLight(PLAY_PARAM, led::GREEN, projectMode && chaining ? on : playing && (!armed || on));
		setKeyLight(STOP_PARAM, led::RED, clearing >= 0.f);
		setKeyLight(OCTAVE_DOWN_PARAM, erasing ? led::RED : led::WHITE, erasing ? on : octave < 0 ? 0.2f - 0.2f * octave : 0.f);
		setKeyLight(OCTAVE_UP_PARAM, led::WHITE, octave > 0 ? 0.2f + 0.2f * octave : 0.f);
		setKeyLight(SHIFT_PARAM, led::WHITE, shifted || (projectMode && copying && on));
		setKeyLight(PROJECT_PARAM, led::WHITE, projectMode);
		setKeyLight(MIXER_PARAM, led::WHITE, mixerMode);
		setKeyLight(METRONOME_PARAM, led::WHITE, tempoMode ? 1.f : beat ? 0.3f : 0.f);
		setKeyLight(SCREEN_PARAM, led::GREEN, batteryShown > 0.f);
		lights[INTERNAL_LIGHT].setBrightness(!external);
		lights[EXTERNAL_LIGHT].setBrightness(external);
		shownTempo = !external || pulsePeriod > 0.f ? bpm() : -1.f;
		Scale now = transposition.moved();
		shownScale = 8 * now.key + now.mode;

		for (int i = 0; i < INPUTS_LEN; i++)
			meter(inputMeters[i], inputs[i], INPUT_LIGHT + 2 * i, dt);
		for (int i = 0; i < JACKS; i++)
			meter(outputMeters[i], outputs[i], OUTPUT_LIGHT + 2 * i, dt);
	}

	void meter(Meter& m, engine::Port& port, int light, float dt) {
		float level = 0.f;
		for (int c = 0; c < port.getChannels(); c++)
			level = std::fmax(level, std::fabs(port.getVoltage(c)));
		m.process(level / 10.f, level > 10.f, dt);
		lights[light].setBrightness(m.peak);
		lights[light + 1].setBrightness(m.clip > 0.f);
	}

	/** What the display shows, for the UI thread: the tempo, right-aligned, or dashes while a clock is measured; on the
	master track, the key and mode the tracks it transposes play in. */
	std::string displayText() {
		if (batteryShown > 0.f) {
			int percent = (int) std::round(100.f * battery);
			return percent >= 100 ? "FUL" : string::f("b%2d", percent);
		}
		if (projectMode && chaining)
			return string::f("C%2d", std::min(chained, CHAIN_LENGTH));
		if (projectMode)
			return string::f("P%2d", project + 1);
		if (mixerMode)
			return string::f("G%2d", muteGroup + 1);
		if (track == MASTER && !tempoMode) {
			int shown = shownScale;
			return scaleDisplay({shown / 8, shown % 8});
		}
		float tempo = shownTempo.load();
		return tempo < 0.f ? "---" : string::f("%3d", std::min((int) std::round(tempo), 999));
	}

	void onReset(const ResetEvent& e) override {
		Module::onReset(e);
		for (int t = 0; t < LANES; t++) {
			Lane& l = lanes[t];
			l.pattern.clear();
			l.player.panic();
			l.settings.channels = TRACK_CHANNELS[t];
			l.recorded = l.pattern;
		}
		transposed = blank.transposed;
		masterNote = -1;
		memory = Step();
		memory.add(Note());
		selection = 0;
		component = -1;
		playing = armed = resuming = recording = trackMode = tempoMode = projectMode = mixerMode = shifted = erasing = chord = false;
		{
			std::lock_guard<std::mutex> lock(projectsMutex);
			for (Project& p : projects)
				p = blank;
			project = 0;
			for (Project::Scene& scene : scenes)
				scene = blankScene();
			pattern = chainAt = 0;
			chain = Project::Chain::of(0);
			std::fill(std::begin(chains), std::end(chains), Project::Chain());
			patternStart = 0.0;
			chaining = copying = rewound = clearAll = false;
			picked = pastes = 0;
			chainKey = -1;
		}
		std::fill(std::begin(mutes), std::end(mutes), 0);
		std::fill(std::begin(audioMutes), std::end(audioMutes), 0);
		muteGroup = 0;
		held = -1;
		cursor = octave = 0;
		track = DRUM_TRACKS;
		ppqn = 4;
	}

	/** A patch from 2.0.5 or earlier had keys where the mixer's and the metronome's params are now, so they'd load as
	0, silent: they go back to their defaults. Rack keeps each module's plugin version in the patch; none is older
	still. */
	void fromJson(json_t* root) override {
		Module::fromJson(root);
		const char* version = json_string_value(json_object_get(root, "version"));
		if (!version || string::Version(version) < string::Version("2.0.6"))
			for (int p = LEVEL_PARAM; p <= METRONOME_LEVEL_PARAM; p++)
				paramQuantities[p]->reset();
	}

	/** Buttons are saved like any param, so one held down while saving would come back stuck. */
	void paramsFromJson(json_t* root) override {
		Module::paramsFromJson(root);
		for (int k = 0; k < ALL_KEYS; k++)
			params[keyParam(k)].setValue(0.f);
	}

	/** A pattern is a list of steps, each a list of notes: [pitch, velocity, micro timing, length]. */
	static json_t* patternToJson(const Pattern& pattern) {
		json_t* steps = json_array();
		for (const Step& s : pattern.steps) {
			json_t* notes = json_array();
			for (int i = 0; i < s.count; i++) {
				const Note& n = s.notes[i];
				json_array_append_new(notes, json_pack("[iiii]", n.pitch, n.velocity, n.offset, n.length));
			}
			json_array_append_new(steps, notes);
		}
		return steps;
	}

	static void patternFromJson(json_t* steps, Pattern& pattern) {
		pattern.clear();
		size_t i, j;
		json_t *notes, *n;
		json_array_foreach(steps, i, notes) {
			if (i >= STEPS)
				break;
			json_array_foreach(notes, j, n) {
				int pitch, velocity, offset, length;
				if (json_unpack(n, "[iiii]", &pitch, &velocity, &offset, &length) == 0)
					pattern.steps[i].add({int8_t(clamp(pitch, 0, 127)), uint8_t(clamp(velocity, 1, 127)),
						int8_t(clamp(offset, MIN_OFFSET, MAX_OFFSET)), int16_t(clamp(length, 0, DRONE))});
			}
		}
	}

	/** Each step's components, a list of [component, value]. */
	static json_t* componentsToJson(const Pattern& pattern) {
		json_t* steps = json_array();
		for (const Step& s : pattern.steps) {
			json_t* components = json_array();
			for (int c = 0; c < COMPONENTS_LEN; c++)
				if (s.has(c))
					json_array_append_new(components, json_pack("[ii]", c, s.values[c]));
			json_array_append_new(steps, components);
		}
		return steps;
	}

	static void componentsFromJson(json_t* steps, Pattern& pattern) {
		size_t i, j;
		json_t *components, *c;
		json_array_foreach(steps, i, components) {
			if (i >= STEPS)
				break;
			json_array_foreach(components, j, c) {
				int component, value;
				if (json_unpack(c, "[ii]", &component, &value) == 0 && component >= 0 && component < COMPONENTS_LEN) {
					value = clamp(value, 0, 63);
					pattern.steps[i].set(component, std::min(value & 15, 9) | (isSpark(component) ? value & 0x30 : 0));
				}
			}
		}
	}

	/** Each step's parameter locks, a list of [param, value]. */
	static json_t* locksToJson(const Pattern& pattern) {
		json_t* steps = json_array();
		for (const Step& s : pattern.steps) {
			json_t* locks = json_array();
			for (int p = 0; p < SOUND_PARAMS; p++)
				if (s.locked(p))
					json_array_append_new(locks, json_pack("[if]", p, s.lockValues[p]));
			json_array_append_new(steps, locks);
		}
		return steps;
	}

	static void locksFromJson(json_t* steps, Pattern& pattern) {
		size_t i, j;
		json_t *locks, *l;
		json_array_foreach(steps, i, locks) {
			if (i >= STEPS)
				break;
			json_array_foreach(locks, j, l) {
				int param;
				double value;
				if (json_unpack(l, "[iF]", &param, &value) == 0 && param >= 0 && param < SOUND_PARAMS)
					pattern.steps[i].lock(param, (float) value);
			}
		}
	}

	static json_t* masksToJson(const uint16_t* masks) {
		json_t* a = json_array();
		for (int i = 0; i < MUTE_GROUPS; i++)
			json_array_append_new(a, json_integer(masks[i]));
		return a;
	}

	static void masksFromJson(json_t* a, uint16_t* masks) {
		size_t i;
		json_t* m;
		json_array_foreach(a, i, m)
			if (i < MUTE_GROUPS)
				masks[i] = (uint16_t) json_integer_value(m);
	}

	/** A pattern, or null for an empty one. */
	static json_t* sceneToJson(const Project::Scene& scene) {
		if (!scene.used)
			return json_null();
		json_t* root = json_object();
		json_t* parts = json_array();
		for (const Project::Part& part : scene.parts) {
			json_t* t = json_object();
			json_object_set_new(t, "pattern", patternToJson(part.pattern));
			json_object_set_new(t, "components", componentsToJson(part.pattern));
			json_object_set_new(t, "locks", locksToJson(part.pattern));
			json_object_set_new(t, "settings", json_pack("[iiifi]", part.stepCount, part.stepLength, part.noteLength, part.quantize, part.channels));
			if (part.hasSound) {
				json_t* sound = json_array();
				for (float v : part.sound)
					json_array_append_new(sound, json_real(v));
				json_object_set_new(t, "sound", sound);
			}
			json_array_append_new(parts, t);
		}
		json_object_set_new(root, "tracks", parts);
		json_object_set_new(root, "muteGroup", json_integer(scene.muteGroup));
		return root;
	}

	/** A pattern from JSON; a project from 2.0.14 or earlier has the same keys, for its one pattern. */
	static Project::Scene sceneFromJson(json_t* root) {
		Project::Scene scene = blankScene();
		if (!json_is_object(root))
			return scene;
		size_t i;
		json_t* t;
		json_array_foreach(json_object_get(root, "tracks"), i, t) {
			if (i >= LANES)
				break;
			Project::Part& part = scene.parts[i];
			if (json_t* j = json_object_get(t, "pattern"))
				patternFromJson(j, part.pattern);
			if (json_t* j = json_object_get(t, "components"))
				componentsFromJson(j, part.pattern);
			if (json_t* j = json_object_get(t, "locks"))
				locksFromJson(j, part.pattern);
			double quantize = 0.0;
			if (json_unpack(json_object_get(t, "settings"), "[iiiFi]", &part.stepCount, &part.stepLength, &part.noteLength, &quantize, &part.channels) == 0)
				part.quantize = (float) quantize;
			part.stepCount = clamp(part.stepCount, 1, STEPS);
			part.stepLength = clamp(part.stepLength, 0, 9);
			part.noteLength = clamp(part.noteLength, 0, (int) std::size(NOTE_LENGTHS) - 1);
			part.quantize = clamp(part.quantize, 0.f, 1.f);
			part.channels = clamp(part.channels, 1, CHANNELS);
			if (json_t* sound = json_object_get(t, "sound"); sound && i != MASTER) {
				part.hasSound = true;
				for (int k = 0; k < SOUND_PARAMS; k++)
					part.sound[k] = clamp((float) json_number_value(json_array_get(sound, k)), 0.f, 1.f);
			}
		}
		scene.muteGroup = clamp((int) json_integer_value(json_object_get(root, "muteGroup")), 0, MUTE_GROUPS - 1);
		scene.used = true;
		return scene;
	}

	static json_t* chainToJson(const Project::Chain& c) {
		json_t* a = json_array();
		for (int i = 0; i < c.length; i++)
			json_array_append_new(a, json_integer(c.patterns[i]));
		return a;
	}

	/** A chain from JSON, up to its first id that's no pattern's. */
	static Project::Chain chainFromJson(json_t* a) {
		Project::Chain c;
		size_t i;
		json_t* j;
		json_array_foreach(a, i, j) {
			json_int_t q = json_integer_value(j);
			if (!json_is_integer(j) || q < 0 || q >= PATTERN_COUNT || c.length == CHAIN_LENGTH)
				break;
			c.patterns[c.length++] = (uint8_t) q;
		}
		return c;
	}

	static void scenesToJson(json_t* root, const Project::Scene* scenes, int pattern, const Project::Chain& chain, const Project::Chain* chains) {
		json_t* a = json_array();
		for (int i = 0; i < PATTERN_COUNT; i++)
			json_array_append_new(a, sceneToJson(scenes[i]));
		json_object_set_new(root, "patterns", a);
		json_object_set_new(root, "pattern", json_integer(pattern));
		json_object_set_new(root, "chain", chainToJson(chain));
		json_t* c = json_array();
		for (int i = 0; i < SAVED_CHAINS; i++)
			json_array_append_new(c, chainToJson(chains[i]));
		json_object_set_new(root, "chains", c);
	}

	/** The patterns and chains, as scenesToJson keeps them. Without them, there are none but the pattern playing. */
	static void scenesFromJson(json_t* root, Project::Scene* scenes, int* pattern, Project::Chain* chain, Project::Chain* chains) {
		json_t* a = json_object_get(root, "patterns");
		for (int i = 0; i < PATTERN_COUNT; i++)
			scenes[i] = sceneFromJson(json_array_get(a, i));
		*pattern = clamp((int) json_integer_value(json_object_get(root, "pattern")), 0, PATTERN_COUNT - 1);
		*chain = chainFromJson(json_object_get(root, "chain"));
		if (!chain->length)
			*chain = Project::Chain::of(*pattern);
		for (int i = 0; i < SAVED_CHAINS; i++)
			chains[i] = chainFromJson(json_array_get(json_object_get(root, "chains"), i));
	}

	/** A project slot, or null for an empty one. Its .opz file, if it came from one, goes in the patch's storage. */
	static json_t* projectToJson(const Project& p) {
		if (!p.used)
			return json_null();
		json_t* root = json_object();
		scenesToJson(root, p.scenes, p.pattern, p.chain, p.chains);
		json_object_set_new(root, "masterStyle", json_integer(p.masterStyle));
		json_object_set_new(root, "transposed", json_integer(p.transposed));
		json_object_set_new(root, "tempo", json_real(p.tempo));
		json_object_set_new(root, "swing", json_real(p.swing));
		json_object_set_new(root, "levels", json_pack("[ffff]", p.levels[0], p.levels[1], p.levels[2], p.levels[3]));
		json_object_set_new(root, "metronome", json_pack("[if]", p.metronomeSound, p.metronomeLevel));
		json_object_set_new(root, "mutes", masksToJson(p.mutes));
		json_object_set_new(root, "audioMutes", masksToJson(p.audioMutes));
		return root;
	}

	/** A project slot from JSON. One from 2.0.14 or earlier is its first pattern. */
	static Project projectFromJson(json_t* root) {
		Project p = blankProject();
		if (!json_is_object(root))
			return p;
		if (json_object_get(root, "patterns"))
			scenesFromJson(root, p.scenes, &p.pattern, &p.chain, p.chains);
		else
			p.scenes[0] = sceneFromJson(root);
		p.masterStyle = clamp((int) json_integer_value(json_object_get(root, "masterStyle")), (int) LATCH, (int) FREE);
		if (json_t* j = json_object_get(root, "transposed"))
			p.transposed = (uint16_t) json_integer_value(j) & 0xff;
		p.tempo = clamp((float) json_number_value(json_object_get(root, "tempo")), 40.f, 200.f);
		p.swing = clamp((float) json_number_value(json_object_get(root, "swing")), 0.5f, 0.75f);
		for (int l = 0; l < LEVELS_LEN; l++)
			p.levels[l] = clamp((float) json_number_value(json_array_get(json_object_get(root, "levels"), l)), 0.f, 1.f);
		json_t* metronome = json_object_get(root, "metronome");
		p.metronomeSound = clamp((int) json_integer_value(json_array_get(metronome, 0)), 0, Metronome::SOUNDS - 1);
		p.metronomeLevel = clamp((float) json_number_value(json_array_get(metronome, 1)), 0.f, 1.f);
		masksFromJson(json_object_get(root, "mutes"), p.mutes);
		masksFromJson(json_object_get(root, "audioMutes"), p.audioMutes);
		p.used = true;
		return p;
	}

	/** Every slot, as the patch and the projects files keep them. */
	json_t* projectsToJson() {
		std::lock_guard<std::mutex> lock(projectsMutex);
		json_t* a = json_array();
		for (const Project& p : projects)
			json_array_append_new(a, projectToJson(p));
		return a;
	}

	void projectsFromJson(json_t* a) {
		std::lock_guard<std::mutex> lock(projectsMutex);
		for (int i = 0; i < PROJECTS; i++)
			projects[i] = projectFromJson(json_array_get(a, i));
	}

	json_t* dataToJson() override {
		json_t* root = json_object();
		json_t* tracks = json_array();
		for (const Lane& l : lanes) {
			json_t* t = json_object();
			json_object_set_new(t, "pattern", patternToJson(l.pattern));
			json_object_set_new(t, "components", componentsToJson(l.pattern));
			json_object_set_new(t, "locks", locksToJson(l.pattern));
			json_object_set_new(t, "channels", json_integer(l.settings.channels));
			json_array_append_new(tracks, t);
		}
		json_object_set_new(root, "tracks", tracks);
		json_object_set_new(root, "transposed", json_integer(transposed));
		json_object_set_new(root, "track", json_integer(track));
		json_object_set_new(root, "octave", json_integer(octave));
		json_object_set_new(root, "ppqn", json_integer(ppqn));
		json_object_set_new(root, "longPressHolds", json_boolean(longPressHolds));
		json_object_set_new(root, "ctrlClickHolds", json_boolean(ctrlClickHolds));
		json_object_set_new(root, "projects", projectsToJson());
		json_object_set_new(root, "project", json_integer(project));
		json_object_set_new(root, "autoSave", json_boolean(autoSave));
		json_object_set_new(root, "mutes", masksToJson(mutes));
		json_object_set_new(root, "audioMutes", masksToJson(audioMutes));
		json_object_set_new(root, "muteGroup", json_integer(muteGroup));
		{
			std::lock_guard<std::mutex> lock(projectsMutex);
			scenesToJson(root, scenes, pattern, chain, chains);
			json_object_set_new(root, "chainAt", json_integer(chainAt));
		}
		return root;
	}

	/** A patch from before the tracks has one pattern, which goes to the bass track. */
	void dataFromJson(json_t* root) override {
		if (json_t* tracks = json_object_get(root, "tracks")) {
			size_t i;
			json_t* t;
			json_array_foreach(tracks, i, t) {
				if (i >= LANES)
					break;
				if (json_t* p = json_object_get(t, "pattern"))
					patternFromJson(p, lanes[i].pattern);
				if (json_t* c = json_object_get(t, "components"))
					componentsFromJson(c, lanes[i].pattern);
				if (json_t* c = json_object_get(t, "locks"))
					locksFromJson(c, lanes[i].pattern);
				if (json_t* c = json_object_get(t, "channels"))
					lanes[i].settings.channels = clamp((int) json_integer_value(c), 1, CHANNELS);
			}
		}
		else if (json_t* p = json_object_get(root, "pattern"))
			patternFromJson(p, lanes[DRUM_TRACKS].pattern);
		for (Lane& l : lanes)
			l.recorded = l.pattern;
		if (json_t* j = json_object_get(root, "transposed"))
			transposed = (uint16_t) json_integer_value(j) & 0xff;
		if (json_t* j = json_object_get(root, "track"))
			track = clamp((int) json_integer_value(j), 0, LANES - 1);
		if (json_t* j = json_object_get(root, "octave"))
			octave = clamp((int) json_integer_value(j), -3, 3);
		if (json_t* j = json_object_get(root, "ppqn"))
			if (std::count(std::begin(Track::PPQN), std::end(Track::PPQN), json_integer_value(j)))
				ppqn = (int) json_integer_value(j);
		if (json_t* j = json_object_get(root, "longPressHolds"))
			longPressHolds = json_boolean_value(j);
		if (json_t* j = json_object_get(root, "ctrlClickHolds"))
			ctrlClickHolds = json_boolean_value(j);
		if (json_t* j = json_object_get(root, "projects"))
			projectsFromJson(j);
		if (json_t* j = json_object_get(root, "project"))
			project = clamp((int) json_integer_value(j), 0, PROJECTS - 1);
		if (json_t* j = json_object_get(root, "autoSave"))
			autoSave = json_boolean_value(j);
		masksFromJson(json_object_get(root, "mutes"), mutes);
		masksFromJson(json_object_get(root, "audioMutes"), audioMutes);
		if (json_t* j = json_object_get(root, "muteGroup"))
			muteGroup = clamp((int) json_integer_value(j), 0, MUTE_GROUPS - 1);
		// The pattern playing is in the tracks and the params; the rest, and the chains, are here.
		std::lock_guard<std::mutex> lock(projectsMutex);
		scenesFromJson(root, scenes, &pattern, &chain, chains);
		const uint8_t* at = std::find(chain.patterns, chain.patterns + chain.length, pattern);
		int first = at < chain.patterns + chain.length ? int(at - chain.patterns) : 0;
		chainAt = clamp((int) json_integer_value(json_object_get(root, "chainAt")), 0, chain.length - 1);
		if (chain.patterns[chainAt] != pattern)
			chainAt = first;
		chaining = copying = false;
		picked = pastes = 0;
		chainKey = -1;
		rewound = clearAll = false;
	}

	static std::string opzName(int slot) {
		return string::f("project%02d.opz", slot + 1);
	}

	/** The .opz files the slots were imported from live in the patch's storage, so an export keeps what they hold
	beyond what a slot models. */
	void onSave(const SaveEvent& e) override {
		std::lock_guard<std::mutex> lock(projectsMutex);
		bool any = std::any_of(std::begin(projects), std::end(projects), [](const Project& p) { return !p.opz.empty(); });
		std::string dir = any ? createPatchStorageDirectory() : getPatchStorageDirectory();
		for (int i = 0; i < PROJECTS; i++) {
			std::string path = system::join(dir, opzName(i));
			if (!projects[i].opz.empty())
				system::writeFile(path, projects[i].opz);
			else if (system::isFile(path))
				system::remove(path);
		}
	}

	void onAdd(const AddEvent& e) override {
		std::lock_guard<std::mutex> lock(projectsMutex);
		std::string dir = getPatchStorageDirectory();
		for (int i = 0; i < PROJECTS; i++) {
			std::string path = system::join(dir, opzName(i));
			if (system::isFile(path))
				projects[i].opz = system::readFile(path);
		}
	}
};


/** A project switch, or a slot loaded after a file changed it, as one step: it puts back the whole live project, with
its settings, dials and mixer. */
struct ProjectSwitch : history::ModuleAction {
	int from, to;
	Project before, after;

	void enter(int slot, const Project& p) {
		if (Sequencer* m = dynamic_cast<Sequencer*>(APP->engine->getModule(moduleId))) {
			std::lock_guard<std::mutex> lock(m->projectsMutex);
			m->restored = p;
			m->restoring = slot;
		}
	}
	void undo() override {
		enter(from, before);
	}
	void redo() override {
		enter(to, after);
	}
};

/** A pattern changes in the audio thread, from the keys and inputs, so the widget notices each change and saves it
here for undo, rather than being told. One made on a pattern no longer playing goes back to that pattern. */
struct PatternChange : history::ModuleAction {
	int lane, pattern;
	Pattern before, after;

	void set(const Pattern& p) {
		Sequencer* m = dynamic_cast<Sequencer*>(APP->engine->getModule(moduleId));
		if (!m)
			return;
		std::lock_guard<std::mutex> lock(m->projectsMutex);
		if (pattern == m->pattern)
			m->lanes[lane].pattern = m->lanes[lane].recorded = p;
		else
			m->scenes[pattern].parts[lane].pattern = p;
	}
	void undo() override {
		set(before);
	}
	void redo() override {
		set(after);
	}
};


/** One setting of every note on a step, for its context menu: velocity, micro timing or length. */
struct NoteField : Quantity {
	enum Field { VELOCITY, OFFSET, LENGTH };
	Step* step;
	Field field;

	NoteField(Step* step, Field field) : step(step), field(field) {}

	float getMinValue() override {
		return field == VELOCITY ? 1.f : field == OFFSET ? MIN_OFFSET : 0.f;
	}
	float getMaxValue() override {
		return field == VELOCITY ? 127.f : field == OFFSET ? MAX_OFFSET : LONGEST + 1;
	}
	float getDefaultValue() override {
		return field == VELOCITY ? 100.f : 0.f;
	}
	float getValue() override {
		const Note& n = step->notes[0];
		return field == VELOCITY ? n.velocity : field == OFFSET ? n.offset : n.length == DRONE ? LONGEST + 1 : n.length;
	}
	void setValue(float value) override {
		int v = (int) std::round(clamp(value, getMinValue(), getMaxValue()));
		for (int i = 0; i < step->count; i++) {
			Note& n = step->notes[i];
			if (field == VELOCITY)
				n.velocity = v;
			else if (field == OFFSET)
				n.offset = v;
			else
				n.length = v > LONGEST ? DRONE : v;
		}
	}
	std::string getLabel() override {
		return field == VELOCITY ? "Velocity" : field == OFFSET ? "Micro timing" : "Length";
	}
	std::string getDisplayValueString() override {
		int v = (int) getValue();
		if (field == VELOCITY)
			return string::f("%d", v);
		if (field == OFFSET)
			return string::f("%+d ticks", v);
		return v == 0 ? "Default" : v > LONGEST ? "Drone" : string::f("%.3g steps", v / float(TICKS));
	}
};

struct NoteSlider : ui::Slider {
	explicit NoteSlider(Quantity* q) {
		quantity = q;
		box.size.x = 220.f;
	}
	~NoteSlider() {
		delete quantity;
	}
};

/** A step key, whose context menu edits its notes. */
struct StepKey : RoundKey {
	Sequencer* sequencer() {
		return module ? static_cast<Sequencer*>(module) : nullptr;
	}

	void appendContextMenu(Menu* menu) override {
		Sequencer* m = sequencer();
		if (!m)
			return;
		Step* step = &m->lane().pattern.steps[paramId - Sequencer::STEP_PARAM];
		menu->addChild(new MenuSeparator);
		if (step->empty())
			menu->addChild(createMenuLabel("No notes"));
		else {
			for (NoteField::Field f : {NoteField::VELOCITY, NoteField::OFFSET, NoteField::LENGTH})
				menu->addChild(new NoteSlider(new NoteField(step, f)));
			menu->addChild(createMenuItem("Clear", "", [=]() { step->clear(); }));
		}
		if (step->components)
			menu->addChild(createMenuItem("Clear step components", "", [=]() { step->clearComponents(); }));
		if (step->locks)
			menu->addChild(createMenuItem("Clear parameter locks", "", [=]() { step->clearLocks(); }));
	}
};


/** Three seven-segment characters: the tempo, or whatever the mode keys show. */
struct TempoDisplay : widget::Widget {
	Sequencer* module = nullptr;

	math::Rect digits() {
		float pad = mm2px(1.4f);
		return math::Rect(pad, pad, box.size.x - 2 * pad, box.size.y - 2 * pad);
	}

	void draw(const DrawArgs& args) override {
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, mm2px(1.f));
		nvgFillColor(args.vg, nvgRGB(0x14, 0x15, 0x17));
		nvgFill(args.vg);
		SevenSegment::draw(args.vg, digits(), "888", false, nvgTransRGBAf(NOTE_COLOR, 0.08f));
	}

	void drawLayer(const DrawArgs& args, int layer) override {
		if (layer == 1)
			SevenSegment::draw(args.vg, digits(), module ? module->displayText() : "120", true, NOTE_COLOR);
		Widget::drawLayer(args, layer);
	}
};




/** One of the four dials. It turns whatever the master says it does now, mostly a param of the selected track's
engine, so its tooltip, undo and MIDI mapping are that param's own; with nothing to turn, it stands still. */
struct MasterDial : SmallDial {
	Sequencer* master = nullptr;
	int dial = 0;

	void step() override {
		std::pair<Module*, int> target = master ? master->dialTarget(dial) : std::make_pair<Module*, int>(nullptr, -1);
		module = target.first;
		paramId = target.second;
		SmallDial::step();
	}

	bool locking() {
		return module && module == master && paramId >= Sequencer::LOCK_PARAM;
	}

	void onDragStart(const DragStartEvent& e) override {
		if (master)
			master->turningLock = locking();
		SmallDial::onDragStart(e);
	}

	/** A lock is part of the pattern, whose undo takes the whole turn as one edit, so the turn pushes no param change
	of its own. */
	void onDragEnd(const DragEndEvent& e) override {
		if (master)
			master->turningLock = false;
		if (!locking() || e.button != GLFW_MOUSE_BUTTON_LEFT)
			return SmallDial::onDragEnd(e);
		if (settings::knobMode == settings::KNOB_MODE_LINEAR || settings::knobMode == settings::KNOB_MODE_SCALED_LINEAR)
			APP->window->cursorUnlock();
		ParamWidget::onDragEnd(e);
	}

	void onDoubleClick(const DoubleClickEvent& e) override {
		if (locking())
			static_cast<Sequencer::LockQuantity*>(getParamQuantity())->unlock();
		else
			SmallDial::onDoubleClick(e);
	}
};


/** The project slots to and from files, for the context menu. Everything here runs on the UI thread. */
namespace files {

inline std::string choose(osdialog_file_action action, const char* filename = nullptr, const char* filters = nullptr) {
	osdialog_filters* f = filters ? osdialog_filters_parse(filters) : nullptr;
	char* path = osdialog_file(action, nullptr, filename, f);
	if (f)
		osdialog_filters_free(f);
	std::string chosen = path ? path : "";
	std::free(path);
	return chosen;
}

inline void warn(const std::string& message) {
	osdialog_message(OSDIALOG_WARNING, OSDIALOG_OK, message.c_str());
}

/** The live project goes into its slot first, as switching projects would save it. */
inline void storeLive(Sequencer* m) {
	std::lock_guard<std::mutex> lock(m->projectsMutex);
	if (m->autoSave)
		m->saveProject(m->project);
}

inline void saveSlots(Sequencer* m) {
	std::string path = choose(OSDIALOG_SAVE, "projects.json", "Projects:json");
	if (path.empty())
		return;
	if (system::getExtension(path) != ".json")
		path += ".json";
	storeLive(m);
	json_t* root = json_object();
	json_object_set_new(root, "projects", m->projectsToJson());
	if (json_dump_file(root, path.c_str(), JSON_INDENT(1)) != 0)
		warn("Couldn't write " + path);
	json_decref(root);
}

inline void loadSlots(Sequencer* m) {
	std::string path = choose(OSDIALOG_OPEN, nullptr, "Projects:json");
	if (path.empty())
		return;
	json_error_t error;
	json_t* root = json_load_file(path.c_str(), 0, &error);
	json_t* projects = root ? json_object_get(root, "projects") : nullptr;
	if (!json_is_array(projects))
		warn("Not a projects file: " + path);
	else {
		m->projectsFromJson(projects);
		m->reload = m->project;
	}
	json_decref(root);
}

/** Every used slot as projects/projectNN.opz, for the OP-Z's disk mode. */
inline void exportOpz(Sequencer* m) {
	std::string dir = choose(OSDIALOG_OPEN_DIR);
	if (dir.empty())
		return;
	storeLive(m);
	std::vector<uint8_t> files[PROJECTS];
	{
		std::lock_guard<std::mutex> lock(m->projectsMutex);
		for (int i = 0; i < PROJECTS; i++)
			if (m->projects[i].used)
				files[i] = opzfile::write(m->projects[i]);
	}
	for (int i = 0; i < PROJECTS; i++)
		if (!files[i].empty())
			system::writeFile(system::join(dir, Sequencer::opzName(i)), files[i]);
}

inline bool readOpz(const std::string& path, Project& p) {
	std::string error;
	p = Sequencer::blankProject();
	if (opzfile::read(system::readFile(path), p, &error))
		return true;
	warn(path + ": " + error);
	return false;
}

/** projectNN.opz from a folder, the OP-Z's projects folder say, into the slots of the same numbers. */
inline void importOpzFolder(Sequencer* m) {
	std::string dir = choose(OSDIALOG_OPEN_DIR);
	if (dir.empty())
		return;
	int found = 0;
	for (int i = 0; i < PROJECTS; i++) {
		std::string path = system::join(dir, Sequencer::opzName(i));
		Project p;
		if (system::isFile(path) && readOpz(path, p)) {
			std::lock_guard<std::mutex> lock(m->projectsMutex);
			m->projects[i] = p;
			found++;
		}
	}
	if (!found)
		warn("No project01.opz to project10.opz in " + dir);
	else
		m->reload = m->project;
}

inline void importOpz(Sequencer* m) {
	std::string path = choose(OSDIALOG_OPEN, nullptr, "OP-Z project:opz");
	Project p;
	if (path.empty() || !readOpz(path, p))
		return;
	std::lock_guard<std::mutex> lock(m->projectsMutex);
	m->projects[m->project] = p;
	m->reload = m->project;
}

} // namespace files


struct SequencerWidget : ModuleWidget {
	SequencerWidget(Sequencer* module) {
		setModule(module);
		std::string panel = asset::plugin(pluginInstance, "res/Sequencer.svg");
		setPanel(createPanel(panel, asset::plugin(pluginInstance, "res/Sequencer-dark.svg")));
		addScrews(this);

		PanelLayout layout(panel);
#define AT(ID) layout.center(#ID), module, Sequencer::ID
		TempoDisplay* display = createWidget<TempoDisplay>(math::Vec());
		display->box = layout.box("TEMPO_WIDGET");
		display->module = module;
		addChild(display);
		addChild(createLightCentered<SmallSimpleLight<NoteLight>>(AT(INTERNAL_LIGHT)));
		addChild(createLightCentered<SmallSimpleLight<NoteLight>>(AT(EXTERNAL_LIGHT)));
		for (int i = 0; i < CHANNELS; i++)
			addChild(createLightCentered<DotLight<NoteLight>>(layout.center(string::f("CHANNEL%d_LIGHT", i)), module, Sequencer::CHANNEL_LIGHT + i));

		for (int d = 0; d < 4; d++) {
			MasterDial* dial = createWidgetCentered<MasterDial>(layout.center(string::f("DIAL%d_WIDGET", d)));
			dial->color(d);
			dial->master = module;
			dial->dial = d;
			addParam(dial);
			addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(layout.center(string::f("DIAL%d_LIGHT", d)), module, Sequencer::DIAL_LIGHT + 3 * d));
		}

		// Every key, with the symbol its LED lights: an icon for the function and track keys, a number for the black
		// keys, a dot for the rest.
		static const char* const functions[][2] = {{"TRACK", "track"}, {"REC", "rec"}, {"PLAY", "play"}, {"STOP", "stop"},
			{"OCTAVE_DOWN", "minus"}, {"OCTAVE_UP", "plus"}, {"SHIFT", "shift"},
			{"PROJECT", "project"}, {"MIXER", "mixer"}, {"METRONOME", "metronome"}, {"SCREEN", "screen"}};
		static const char* const tracks[TRACKS] = {"kick", "snare", "perc", "sample", "bass", "lead", "arp", "chord"};
		for (int k = 0; k < Sequencer::ALL_KEYS; k++) {
			int p = Sequencer::keyParam(k), light = Sequencer::keyLight(p);
			std::string name, symbol = "dot";
			RoundKey* key;
			if (p == Sequencer::MASTER_PARAM) {
				name = "MASTER";
				symbol = "master";
			}
			else if (p >= Sequencer::PAGE_PARAM)
				name = string::f("PAGE%d", p - Sequencer::PAGE_PARAM);
			else if (p >= Sequencer::SELECT_PARAM) {
				name = string::f("SELECT%d", p - Sequencer::SELECT_PARAM);
				symbol = tracks[p - Sequencer::SELECT_PARAM];
			}
			else if (p >= Sequencer::KEY_PARAM) {
				int k = p - Sequencer::KEY_PARAM;
				name = string::f("KEY%d", k);
				if (keyValue(k) >= 0)
					symbol = std::to_string((keyValue(k) + 1) % 10);
			}
			else if (p >= Sequencer::STEP_PARAM)
				name = string::f("STEP%d", p - Sequencer::STEP_PARAM);
			else {
				name = functions[p - Sequencer::TRACK_PARAM][0];
				symbol = functions[p - Sequencer::TRACK_PARAM][1];
			}
			if (p >= Sequencer::STEP_PARAM && p < Sequencer::KEY_PARAM)
				key = createLightParam<StepKey>(math::Vec(), module, p, light);
			else
				key = createLightParam<RoundKey>(math::Vec(), module, p, light);
			key->setRing(layout.box(name + "_PARAM"), symbol);
			addParam(key);
		}

		static const char* const inputs[] = {"CLOCK", "RUN", "RESET", "CONT", "ADVANCE", "VOCT", "GATE", "VELOCITY"};
		static const char* const outputs[] = {"CLOCK", "RUN", "RESET", "CONT", "LEFT", "RIGHT"};
		for (int i = 0; i < Sequencer::INPUTS_LEN; i++) {
			addInput(createInputCentered<ThemedPJ301MPort>(layout.center(string::f("%s_INPUT", inputs[i])), module, i));
			addChild(createLightCentered<DotLight<GreenRedLight>>(layout.center(string::f("%s_INPUT_LIGHT", inputs[i])), module, Sequencer::INPUT_LIGHT + 2 * i));
		}
		for (int i = 0; i < Sequencer::JACKS; i++) {
			addOutput(createOutputCentered<ThemedPJ301MPort>(layout.center(string::f("%s_OUTPUT", outputs[i])), module, i));
			addChild(createLightCentered<DotLight<GreenRedLight>>(layout.center(string::f("%s_OUTPUT_LIGHT", outputs[i])), module, Sequencer::OUTPUT_LIGHT + 2 * i));
		}
#undef AT
	}

	void step() override {
		if (Sequencer* m = getModule<Sequencer>())
			recordChanges(m);
		ModuleWidget::step();
	}

	/** Each lane's edits as a step of the undo history, and a change of live project as one step for them all. Under
	the slots' lock, so the audio thread can't be half way through a switch. */
	void recordChanges(Sequencer* m) {
		std::lock_guard<std::mutex> lock(m->projectsMutex);
		if (m->recall != Sequencer::SETTLED) {
			for (Sequencer::Lane& l : m->lanes)
				l.recorded = l.pattern;
			if (m->recall == Sequencer::SWITCHED) {
				ProjectSwitch* h = new ProjectSwitch;
				h->name = string::f("switch to project %d", m->project + 1);
				h->moduleId = m->id;
				h->from = m->leftSlot;
				h->to = m->project;
				h->before = m->left;
				h->after = m->entered;
				APP->history->push(h);
			}
			m->recall = Sequencer::SETTLED;
		}
		for (int t = 0; !m->turningLock && t < LANES; t++) {
			Sequencer::Lane& l = m->lanes[t];
			if (l.pattern == l.recorded)
				continue;
			PatternChange* h = new PatternChange;
			h->name = string::f("edit track %d", trackNumber(t));
			h->moduleId = m->id;
			h->lane = t;
			h->pattern = m->pattern;
			h->before = l.recorded;
			h->after = l.recorded = l.pattern;
			APP->history->push(h);
		}
	}

	void appendContextMenu(Menu* menu) override {
		Sequencer* module = getModule<Sequencer>();
		std::vector<std::string> ppqn, channels;
		for (int n : Track::PPQN)
			ppqn.push_back(string::f("%d PPQN", n));
		for (int n = 1; n <= CHANNELS; n++)
			channels.push_back(string::f("%d", n));
		menu->addChild(new MenuSeparator);
		menu->addChild(createIndexSubmenuItem("Clock pulses per beat", ppqn,
			[=]() { return std::find(std::begin(Track::PPQN), std::end(Track::PPQN), module->ppqn) - std::begin(Track::PPQN); },
			[=](size_t i) { module->ppqn = Track::PPQN[i]; }));
		menu->addChild(createIndexSubmenuItem(trackName(module->track) + ": polyphony", channels,
			[=]() { return module->lane().settings.channels - 1; },
			[=](size_t i) { module->lane().settings.channels = i + 1; }));
		menu->addChild(createBoolPtrMenuItem("Long-press toggles hold", "", &module->longPressHolds));
		menu->addChild(createBoolPtrMenuItem(RACK_MOD_CTRL_NAME "-click toggles hold", "", &module->ctrlClickHolds));
		menu->addChild(createSubmenuItem("Projects", "", [=](Menu* menu) {
			menu->addChild(createBoolPtrMenuItem("Save the project on switching", "", &module->autoSave));
			menu->addChild(new MenuSeparator);
			menu->addChild(createMenuItem("Save the slots to a file…", "", [=]() { files::saveSlots(module); }));
			menu->addChild(createMenuItem("Load the slots from a file…", "", [=]() { files::loadSlots(module); }));
			menu->addChild(new MenuSeparator);
			menu->addChild(createMenuLabel("OP-Z disk mode"));
			menu->addChild(createMenuItem("Export the slots to a projects folder…", "", [=]() { files::exportOpz(module); }));
			menu->addChild(createMenuItem("Import a projects folder…", "", [=]() { files::importOpzFolder(module); }));
			menu->addChild(createMenuItem(string::f("Import a project into slot %d…", module->project + 1), "", [=]() { files::importOpz(module); }));
		}));
	}
};

} // namespace opz

Model* modelSequencer = createModel<opz::Sequencer, opz::SequencerWidget>("Sequencer");
