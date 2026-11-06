/*
Pattern Editor: a pattern of the Sequencer's on a big screen, beside it on its left, to see and edit with the mouse and
the computer's keys. Track view is a roll of one track's notes (a drum track's rows are its kit's sounds), with a lane
under it for the notes' velocity or one dial's locks. Overview shows every lane's steps at once. Steps past a track's
step count are greyed and can't be edited.

The screen reads the pattern through the hub on the UI thread (PatternView) and writes it back a step at a time; each
mouse gesture or key is one step of the undo history (Hub::beginEdit). A click on the screen gives it the computer's
keys, shown by an outline; Escape or a click anywhere else takes them away. Rack's own undo and save keys still work.

The mouse works as a DAW's clip editor does: a press is a click if it moves less than 3 px, else a drag. With the
Pointer a click selects a note and a drag moves the selection, or box-selects on an empty cell; the Pen adds notes and
the Eraser deletes them. What each gesture does is set in the module's menu.

VIEW swaps the views, FOLLOW ties the track shown to the Sequencer's selected one, LANE picks what the lane under the
roll shows, and the octave keys scroll the roll. The PATTERN wheel shows another of the 16 patterns: the screen follows
the pattern playing until it's turned to another. The module is as wide as its left edge is dragged, 24 to 64 HP.
*/
#include "plugin.hpp"
#include "ui.hpp"
#include "opz/Hub.hpp"

using namespace opz;

static const NVGcolor LANE_COLORS[LANES] = {
	nvgRGB(0xf0, 0x94, 0x7f), nvgRGB(0xf2, 0xce, 0x62), nvgRGB(0x8f, 0xcc, 0x88), nvgRGB(0x6f, 0xd3, 0xd0),
	nvgRGB(0x8d, 0xb2, 0xec), nvgRGB(0xb4, 0x9c, 0xf0), nvgRGB(0xf0, 0x8f, 0xc4), nvgRGB(0xf7, 0xb2, 0x67),
	nvgRGB(0xe6, 0xe5, 0xe1), nvgRGB(0x5e, 0xc8, 0xf2), nvgRGB(0xe5, 0x8e, 0xf0),
};
static constexpr int PAD_LOW = 53, ROWS = 24, LOCK_LANES = 16; // a drum kit's 24 sounds from F3; the four pages' dials
static constexpr float HEIGHT = 128.5f;

enum Tool { POINTER, PEN, ERASER, TOOLS };
static const char* const TOOL_TIPS[TOOLS] = {"Pointer (P): select, move and box-select", "Pen (D): add notes, drag to set the length; hold D to draw for a moment",
	"Eraser (E): delete the notes you click or drag across"};

/** What each mouse gesture does, set in the module's menu. The first choice of each is the default. */
enum Gesture { EMPTY_CLICK, EMPTY_DRAG, NOTE_CLICK, EMPTY_DOUBLE, NOTE_DOUBLE, FREE_KEY, GESTURES };
static const struct {
	const char* name;
	std::vector<std::string> choices;
} GESTURE_MENU[GESTURES] = {
	{"Click on an empty cell", {"Clears the selection", "Adds a note"}},
	{"Drag on an empty cell", {"Box-selects", "Draws notes"}},
	{"Click on a note", {"Selects it", "Deletes it"}},
	{"Double-click on an empty cell", {"Adds a note", "Does nothing"}},
	{"Double-click on a note", {"Deletes it", "Does nothing"}},
	{"Free timing while dragging", {"Shift", "Ctrl", "Alt"}},
};
static const char* const GESTURE_KEYS[GESTURES] = {"emptyClick", "emptyDrag", "noteClick", "emptyDouble", "noteDouble", "freeKey"};

struct PatternEditor : Module {
	enum ParamId { VIEW_PARAM, FOLLOW_PARAM, LANE_PARAM, OCTAVE_DOWN_PARAM, OCTAVE_UP_PARAM, PARAMS_LEN };
	enum InputId { INPUTS_LEN };
	enum OutputId { OUTPUTS_LEN };
	enum LightId { VIEW_LIGHT, FOLLOW_LIGHT, ENUMS(LANE_LIGHT, 3), ENUMS(KEY_LIGHT, 2 * 3), LIGHTS_LEN };
	static constexpr int MIN_WIDTH = 24, MAX_WIDTH = 64;

	// The screen's state, the UI thread's and saved in the patch; the lights read the atomic ones.
	int width = 32;                         // HP
	std::atomic<bool> overview{false}, follow{true};
	std::atomic<int> lane{0};               // 0 for velocity, or 1 + the dial the lane shows locks of
	int track = DRUM_TRACKS + 1;            // lead
	int pattern = 0;
	bool followPattern = true;              // the pattern playing, until the wheel turns to another
	int low[LANES] = {PAD_LOW, PAD_LOW, PAD_LOW, PAD_LOW, 31, 60, 52, 45, 52, 48, 48}; // each roll's bottom row
	int tool = POINTER;
	int gestures[GESTURES] = {};

	std::atomic<int> presses[PARAMS_LEN] = {}; // the keys' presses, for the UI thread to take
	dsp::BooleanTrigger triggers[PARAMS_LEN];
	dsp::ClockDivider lightDivider;

	PatternEditor() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configButton(VIEW_PARAM, "View: overview (lit) or one track");
		configButton(FOLLOW_PARAM, "Follow the Sequencer's selected track");
		configButton(LANE_PARAM, "Lane: velocity, then each dial's locks");
		configButton(OCTAVE_DOWN_PARAM, "Octave down");
		configButton(OCTAVE_UP_PARAM, "Octave up");
		lightDivider.setDivision(64);
	}

	void onExpanderChange(const ExpanderChangeEvent& e) override {
		busMoved();
	}

	void process(const ProcessArgs& args) override {
		for (int p = 0; p < PARAMS_LEN; p++)
			if (triggers[p].process(params[p].getValue() > 0.f))
				presses[p]++;
		if (!lightDivider.process())
			return;
		lights[VIEW_LIGHT].setBrightness(overview);
		lights[FOLLOW_LIGHT].setBrightness(follow);
		int l = lane;
		NVGcolor c = l ? DIAL_COLORS[(l - 1) % 4] : nvgRGBf(0.f, 0.f, 0.f);
		lights[LANE_LIGHT + 0].setBrightness(c.r);
		lights[LANE_LIGHT + 1].setBrightness(c.g);
		lights[LANE_LIGHT + 2].setBrightness(c.b);
		for (int k = 0; k < 2; k++)
			for (int i = 0; i < 3; i++)
				lights[KEY_LIGHT + 3 * k + i].setBrightness(params[OCTAVE_DOWN_PARAM + k].getValue() > 0.f);
	}

	json_t* dataToJson() override {
		json_t* root = json_object();
		json_object_set_new(root, "width", json_integer(width));
		json_object_set_new(root, "overview", json_boolean(overview));
		json_object_set_new(root, "follow", json_boolean(follow));
		json_object_set_new(root, "lane", json_integer(lane));
		json_object_set_new(root, "track", json_integer(track));
		json_object_set_new(root, "pattern", json_integer(followPattern ? -1 : pattern));
		json_t* lows = json_array();
		for (int l : low)
			json_array_append_new(lows, json_integer(l));
		json_object_set_new(root, "low", lows);
		json_object_set_new(root, "tool", json_integer(tool));
		for (int g = 0; g < GESTURES; g++)
			json_object_set_new(root, GESTURE_KEYS[g], json_integer(gestures[g]));
		return root;
	}

	void dataFromJson(json_t* root) override {
		auto integer = [&](const char* key, int fallback) {
			json_t* j = json_object_get(root, key);
			return j ? (int) json_integer_value(j) : fallback;
		};
		width = clamp(integer("width", width), MIN_WIDTH, MAX_WIDTH);
		overview = json_is_true(json_object_get(root, "overview"));
		follow = !json_is_false(json_object_get(root, "follow"));
		lane = clamp(integer("lane", 0), 0, LOCK_LANES);
		track = clamp(integer("track", track), 0, LANES - 1);
		int p = integer("pattern", -1);
		followPattern = p < 0;
		pattern = clamp(p, 0, PATTERN_COUNT - 1);
		json_t* lows = json_object_get(root, "low");
		for (int t = 0; t < LANES && t < (int) json_array_size(lows); t++)
			low[t] = clamp((int) json_integer_value(json_array_get(lows, t)), 0, 127 - ROWS + 1);
		tool = clamp(integer("tool", POINTER), 0, TOOLS - 1);
		for (int g = 0; g < GESTURES; g++)
			gestures[g] = clamp(integer(GESTURE_KEYS[g], 0), 0, (int) GESTURE_MENU[g].choices.size() - 1);
	}
};


/** A tool of the palette at the screen's left: its icon, lit while it's the tool, and a tooltip. */
struct ToolButton : widget::OpaqueWidget {
	int tool = POINTER;
	std::function<int()> current;
	std::function<void(int)> pick;
	ui::Tooltip* tooltip = nullptr;

	static NSVGimage* icon(int tool, bool lit) {
		static const char* const PATHS[TOOLS] = {
			"M6 2.5l13 11.2h-6.6l3.7 7.2-2.6 1.3-3.7-7.3L6 19.6z",
			"M3 17.25V21h3.75L17.81 9.94l-3.75-3.75L3 17.25zM20.71 7.04a1 1 0 0 0 0-1.41l-2.34-2.34a1 1 0 0 0-1.41 0l-1.83 1.83 3.75 3.75 1.83-1.83z",
			"M15.1 3.6l5.3 5.3a2 2 0 0 1 0 2.8L12 20h8v2H7.6l-4.1-4.1a2 2 0 0 1 0-2.8L12.3 3.6a2 2 0 0 1 2.8 0zM5 16.5l3.3 3.5h2.9l1.9-1.9-5.4-5.4z",
		};
		static NSVGimage* images[TOOLS][2] = {};
		NSVGimage*& image = images[tool][lit];
		if (!image) {
			std::string svg = string::f("<svg xmlns='http://www.w3.org/2000/svg' width='24' height='24'><path fill='%s' d='%s'/></svg>", lit ? "#0e0e10" : "#dcdbd8", PATHS[tool]);
			image = nsvgParse(&svg[0], "px", 96.f);
		}
		return image;
	}

	void onButton(const ButtonEvent& e) override {
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT && pick)
			pick(tool);
		e.consume(this);
	}

	void onEnter(const EnterEvent& e) override {
		if (!settings::tooltips || tooltip)
			return;
		tooltip = new ui::Tooltip;
		tooltip->text = TOOL_TIPS[tool];
		APP->scene->addChild(tooltip);
	}

	void onLeave(const LeaveEvent& e) override {
		hide();
	}

	void hide() {
		if (!tooltip)
			return;
		APP->scene->removeChild(tooltip);
		delete tooltip;
		tooltip = nullptr;
	}

	~ToolButton() {
		hide();
	}

	void drawLayer(const DrawArgs& args, int layer) override {
		if (layer != 1)
			return;
		bool lit = current && current() == tool;
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, mm2px(0.6f));
		nvgFillColor(args.vg, lit ? nvgRGB(0xdc, 0xdb, 0xd8) : nvgRGBA(0xff, 0xff, 0xff, 0x10));
		nvgFill(args.vg);
		if (NSVGimage* image = icon(tool, lit)) {
			nvgSave(args.vg);
			float inset = mm2px(0.5f), scale = (box.size.x - 2 * inset) / 24.f;
			nvgTranslate(args.vg, inset, inset);
			nvgScale(args.vg, scale, scale);
			window::svgDraw(args.vg, image);
			nvgRestore(args.vg);
		}
	}
};


/** The pattern on the glass. Every measure is in mm from the glass's top left; a step's columns and a pitch's rows
come from Layout, which the drawing and the mouse share. */
struct PatternScreen : widget::OpaqueWidget {
	PatternEditor* module = nullptr;
	PatternView view;
	bool linked = false;
	int cursorStep = 0, cursorPitch = 72;
	std::string typed;                  // digits of a value being typed
	int requested = -1, waited = 0;     // a track asked of the Sequencer, until it's selected
	int flashStep = -1;
	double flashAt = 0.0;
	bool editing = false;               // a gesture is under way
	std::set<std::pair<int, int>> selected; // notes by step and pitch, of the track shown
	math::Vec hover{-1.f, -1.f};        // mm, or off the screen
	double lastClickAt = -1.0;          // the last click, to tell a double-click
	std::pair<int, int> lastCell{-1, -1};
	bool lastClickAdded = false;
	int toolBeforeD = -1;               // while D is held: the tool to go back to if it's held, not tapped
	double pressedD = 0.0;
	ToolButton* tools[TOOLS];

	/** A note to put back somewhere: as it was, from its step and its start in ticks from step 1. */
	struct Moving {
		Note note;
		int step, ticks;
	};

	enum Kind { NONE, PRESS, MOVE, LENGTH, BOX, ERASE, PAINT, LANE };
	struct Hit {
		bool roll = false;
		int step = -1, pitch = 0;
		int noteStep = -1;              // the step of the note under it, or -1
		bool edge = false;              // on the note's end, which sets its length
		float x = 0.f, w = 0.f;         // the note's span, mm
	};
	struct DragState {
		Kind kind = NONE;
		Hit hit;
		math::Vec start, at;            // mm
		float travel = 0.f;             // px on the window
		int mods = 0, tool = POINTER;
		Pattern base;                   // MOVE: the track without the notes moved
		std::vector<Moving> items;      // MOVE: the notes moved; LENGTH: the notes, their length in `note.length`
		int lead = 0;                   // the item pressed
		int leadLength = 0;
		bool copies = false, placed = false;
		std::set<std::pair<int, int>> before; // BOX: the selection it adds to; MOVE of copies: the originals
	} drag;

	struct Clip {
		bool notes = false;
		Step step;
		std::vector<Moving> moving;     // ticks from the first note's step
		int firstOffset = 0;
	};

	static float px2mm(float px) { return px * MM_PER_IN / SVG_DPI; }
	static math::Vec px2mm(math::Vec v) { return v.mult(MM_PER_IN / SVG_DPI); }

	struct Layout {
		float w, h, palX = 1.5f, palW = 4.6f, palStep = 5.6f, keyX = 7.1f, keyW = 10.5f, gridX, gridW, cw, gridY = 12.5f, gridH, rowH;
		float laneY, laneH = 13.f, compY, compH = 4.5f, labelW = 17.f, ovX, ovW, ocw, ovY = 12.5f, ovH;
		Layout(math::Vec size) {
			w = px2mm(size.x);
			h = px2mm(size.y);
			gridX = keyX + keyW + 1.f;
			gridW = w - 1.5f - gridX;
			cw = gridW / STEPS;
			compY = h - 1.5f - compH;
			laneY = compY - 1.f - laneH;
			gridH = laneY - 1.5f - gridY;
			rowH = gridH / ROWS;
			ovX = 1.5f + labelW + 1.f;
			ovW = w - 1.5f - ovX;
			ocw = ovW / STEPS;
			ovH = (h - 1.5f - ovY) / LANES;
		}
	};

	PatternScreen() {
		Layout L(math::Vec(1.f, 1.f));
		for (int t = 0; t < TOOLS; t++) {
			ToolButton* b = tools[t] = createWidget<ToolButton>(mm2px(math::Vec(L.palX, L.gridY + t * L.palStep)));
			b->box.size = mm2px(math::Vec(L.palW, L.palW));
			b->tool = t;
			b->current = [this] { return tool(); };
			b->pick = [this](int t) {
				if (module)
					module->tool = t;
			};
			addChild(b);
		}
	}

	Hub* hub() const { return findHubRight(module); }
	int track() const { return module ? module->track : DRUM_TRACKS + 1; }
	bool drum() const { return track() < DRUM_TRACKS; }
	int low() const { return drum() ? PAD_LOW : module ? module->low[track()] : 60; }
	int lowest() const { return drum() ? PAD_LOW : 0; }
	int highest() const { return drum() ? PAD_LOW + ROWS - 1 : 127; }
	bool overview() const { return module && module->overview; }
	int lane() const { return track() == MASTER ? 0 : module ? module->lane.load() : 0; }
	bool focused() { return APP->event->getSelectedWidget() == this; }
	const Settings& settings() const { return view.settings[track()]; }
	Pattern& pattern() { return view.lanes[track()]; }
	bool editable(int step) const { return step >= 0 && step < clamp(settings().stepCount, 1, STEPS); }
	int gesture(Gesture g) const { return module ? module->gestures[g] : 0; }
	static int mods() { return APP->window->getMods() & RACK_MOD_MASK; }
	int tool() const { return module ? module->tool : POINTER; }
	bool freeTiming(int m) const {
		static const int KEYS[3] = {GLFW_MOD_SHIFT, RACK_MOD_CTRL, GLFW_MOD_ALT};
		return m & KEYS[clamp(gesture(FREE_KEY), 0, 2)];
	}

	// The gesture's edits, on screen at once and in the Sequencer.

	void begin() {
		Hub* h = hub();
		if (editing || !h || !linked)
			return;
		h->beginEdit(view.pattern);
		editing = true;
	}

	void end() {
		if (!editing)
			return;
		editing = false;
		if (Hub* h = hub())
			h->endEdit();
	}

	void put(int step, const Step& s) {
		Hub* h = hub();
		if (!editing || !h)
			return;
		pattern().steps[step] = s;
		h->editStep(track(), step, s);
	}

	/** Writes every step of the track that differs. */
	void put(const Pattern& p) {
		for (int s = 0; s < STEPS; s++)
			if (!(p.steps[s] == pattern().steps[s]))
				put(s, p.steps[s]);
	}

	/** One edit, a key's or a click's, as a step of the undo history of its own. */
	template <class F>
	void edit(F change) {
		begin();
		change();
		end();
	}

	void flash(int step) {
		flashStep = step;
		flashAt = system::getTime();
	}

	int noteAt(int step, int pitch) const {
		return view.lanes[track()].steps[step].indexOf(pitch);
	}

	/** The selected notes still there, by step and pitch. */
	std::vector<std::pair<int, int>> selection() {
		std::vector<std::pair<int, int>> out;
		for (auto [s, p] : selected)
			if (editable(s) && noteAt(s, p) >= 0)
				out.push_back({s, p});
		return out;
	}

	/** What the keys act on: the selection, or else the note at the cursor. */
	std::vector<std::pair<int, int>> targets() {
		std::vector<std::pair<int, int>> out = selection();
		if (out.empty() && editable(cursorStep) && noteAt(cursorStep, cursorPitch) >= 0)
			out.push_back({cursorStep, cursorPitch});
		return out;
	}

	Moving moving(int step, int pitch) {
		const Note& n = pattern().steps[step].notes[noteAt(step, pitch)];
		return {n, step, step * TICKS + n.offset};
	}

	bool addNote(int step, int pitch) {
		Step s = pattern().steps[step];
		if (!editable(step) || s.count >= STEP_NOTES || s.indexOf(pitch) >= 0) {
			flash(step);
			return false;
		}
		s.notes[s.count++] = Note{(int8_t) pitch};
		put(step, s);
		return true;
	}

	void removeNotes(const std::vector<std::pair<int, int>>& notes) {
		Pattern p = pattern();
		for (auto [s, pitch] : notes) {
			p.steps[s].remove(pitch);
			selected.erase({s, pitch});
		}
		put(p);
	}

	/** A start in ticks from step 1, as a step and an offset in reach of it: `from` if it can, else the nearest. */
	static std::pair<int, int> rebase(int from, int ticks) {
		int offset = ticks - from * TICKS;
		if (offset >= MIN_OFFSET && offset <= MAX_OFFSET)
			return {from, offset};
		int step = (int) std::lround(ticks / (double) TICKS);
		return {step, ticks - step * TICKS};
	}

	/** Adds the items to `out` at a tick and pitch delta from where they started, all or none: refused past the step
	count, onto a full step, or onto a note of the same pitch. A snapped move keeps each note's offset. */
	bool shifted(Pattern& out, const std::vector<Moving>& items, int ticks, int semitones, bool snapped, std::vector<std::pair<int, int>>& placed) const {
		placed.clear();
		for (const Moving& m : items) {
			auto [step, offset] = snapped ? std::make_pair(m.step + ticks / TICKS, m.ticks - m.step * TICKS) : rebase(m.step, m.ticks + ticks);
			int pitch = m.note.pitch + semitones;
			if (!editable(step) || pitch < lowest() || pitch > highest())
				return false;
			Step& s = out.steps[step];
			if (s.count >= STEP_NOTES || s.indexOf(pitch) >= 0)
				return false;
			Note n = m.note;
			n.pitch = pitch;
			n.offset = offset;
			s.notes[s.count++] = n;
			placed.push_back({step, pitch});
		}
		return true;
	}

	/** The track without the notes. */
	Pattern without(const std::vector<std::pair<int, int>>& notes) {
		Pattern p = pattern();
		for (auto [s, pitch] : notes)
			p.steps[s].remove(pitch);
		return p;
	}

	int mostLength(int step, const Note& n) const {
		return (STEPS - step) * TICKS - n.offset;
	}

	int lengthOf(int step, const Note& n) const {
		int most = mostLength(step, n);
		return n.length == DRONE ? most : n.length ? std::min<int>(n.length, most) : std::min(settings().noteLength, most);
	}

	void setLength(int step, Note& n, int length) {
		n.length = length >= DRONE ? DRONE : clamp(length, 1, mostLength(step, n));
		if (n.length == settings().noteLength)
			n.length = 0;
	}

	/** Moves the selection, or the cursor's note, by keys; the cursor goes with its note. */
	void nudge(int ticks, int semitones, bool snapped) {
		std::vector<std::pair<int, int>> notes = targets();
		if (notes.empty())
			return;
		std::vector<Moving> items;
		int cursor = -1;
		for (auto [s, p] : notes) {
			if (s == cursorStep && p == cursorPitch)
				cursor = items.size();
			items.push_back(moving(s, p));
		}
		Pattern out = without(notes);
		std::vector<std::pair<int, int>> placed;
		if (!shifted(out, items, ticks, semitones, snapped, placed))
			return;
		edit([&] { put(out); });
		if (!selected.empty())
			selected = {placed.begin(), placed.end()};
		if (cursor >= 0) {
			std::tie(cursorStep, cursorPitch) = placed[cursor];
			reveal();
		}
	}

	/** The selection's notes, or the cursor's, longer or shorter, or drones or back. */
	void lengthen(int ticks, bool drone) {
		std::vector<std::pair<int, int>> notes = targets();
		Pattern p = pattern();
		bool allDrones = true;
		for (auto [s, pitch] : notes)
			allDrones = allDrones && p.steps[s].notes[p.steps[s].indexOf(pitch)].length == DRONE;
		for (auto [s, pitch] : notes) {
			Note& n = p.steps[s].notes[p.steps[s].indexOf(pitch)];
			if (drone)
				n.length = allDrones ? 0 : DRONE;
			else
				setLength(s, n, lengthOf(s, n) + ticks);
		}
		edit([&] { put(p); });
	}

	/** The lane's value at a step, 0 to 1: every note's velocity, or the dial's lock. */
	void setLane(int step, float value) {
		if (!editable(step))
			return;
		Step s = pattern().steps[step];
		value = clamp(value, 0.f, 1.f);
		if (int l = lane())
			s.lock(l - 1, value);
		else
			for (int i = 0; i < s.count; i++)
				s.notes[i].velocity = (uint8_t) clamp((int) std::lround(value * 127.f), 1, 127);
		if (!(s == pattern().steps[step]))
			put(step, s);
	}

	static Clip& clipboard() {
		static Clip clip;
		return clip;
	}

	/** The selection's notes, or else the cursor's step. */
	void copy() {
		Clip& c = clipboard();
		std::vector<std::pair<int, int>> notes = selection();
		c.notes = !notes.empty();
		if (!c.notes) {
			c.step = pattern().steps[cursorStep];
			return;
		}
		c.moving.clear();
		int first = INT_MAX;
		for (auto [s, p] : notes)
			first = std::min(first, moving(s, p).ticks);
		for (auto [s, p] : notes) {
			Moving m = moving(s, p);
			m.ticks -= first;
			if (m.ticks == 0)
				c.firstOffset = m.note.offset;
			c.moving.push_back(m);
		}
	}

	/** Notes land with the first on the cursor's step, at its own offset, and become the selection. */
	void paste() {
		const Clip& c = clipboard();
		if (!editable(cursorStep))
			return flash(cursorStep);
		if (!c.notes)
			return edit([&] { put(cursorStep, c.step); });
		std::vector<Moving> items;
		int base = cursorStep * TICKS + c.firstOffset;
		for (Moving m : c.moving) {
			m.ticks += base;
			m.step = (int) std::lround(m.ticks / (double) TICKS);
			items.push_back(m);
		}
		Pattern out = pattern();
		std::vector<std::pair<int, int>> placed;
		if (!shifted(out, items, 0, 0, false, placed))
			return flash(cursorStep);
		edit([&] { put(out); });
		selected = {placed.begin(), placed.end()};
	}

	/** Scrolls the roll to keep the cursor's pitch on it. */
	void reveal() {
		if (!module || drum())
			return;
		int& l = module->low[track()];
		l = clamp(l, cursorPitch - ROWS + 1, cursorPitch);
		l = clamp(l, 0, 127 - ROWS + 1);
	}

	void scroll(int semitones) {
		if (module && !drum())
			module->low[track()] = clamp(module->low[track()] + semitones, 0, 127 - ROWS + 1);
	}

	void selectTrack(int t) {
		if (!module)
			return;
		t = (t + LANES) % LANES;
		if (t != module->track)
			selected.clear();
		module->track = t;
		if (module->follow)
			if (Hub* h = hub()) {
				h->requestTrack(t);
				requested = t;
				waited = 0;
			}
		const Step& s = view.lanes[t].steps[cursorStep];
		cursorPitch = s.count ? s.notes[0].pitch : low() + 12;
		cursorPitch = clamp(cursorPitch, lowest(), highest());
		reveal();
	}

	void turnPattern(int steps) {
		if (!module || !linked)
			return;
		end();
		selected.clear();
		module->pattern = clamp(view.pattern + steps, 0, PATTERN_COUNT - 1);
		module->followPattern = module->pattern == view.playingPattern;
	}

	void step() override {
		Hub* h = hub();
		if (module && h) {
			int shown = view.pattern;
			linked = h->patternView(view, module->followPattern ? -1 : module->pattern) || linked;
			if (view.pattern != shown)
				selected.clear();
			if (module->followPattern)
				module->pattern = view.pattern;
			// A track asked of the Sequencer shows at once, though it's selected a frame or two later.
			if (requested >= 0 && (view.selected == requested || ++waited > 30))
				requested = -1;
			if (module->follow && requested < 0 && module->track != view.selected) {
				module->track = view.selected;
				selected.clear();
			}
			auto take = [&](int p) { return module->presses[p].exchange(0); };
			if (take(PatternEditor::VIEW_PARAM) % 2)
				module->overview = !module->overview;
			if (take(PatternEditor::FOLLOW_PARAM) % 2)
				module->follow = !module->follow;
			if (int n = take(PatternEditor::LANE_PARAM))
				module->lane = (module->lane + n) % (LOCK_LANES + 1);
			scroll(12 * (take(PatternEditor::OCTAVE_UP_PARAM) - take(PatternEditor::OCTAVE_DOWN_PARAM)));
		}
		else
			linked = false;
		cursorStep = clamp(cursorStep, 0, STEPS - 1);
		cursorPitch = clamp(cursorPitch, lowest(), highest());
		for (ToolButton* b : tools)
			b->visible = linked && !overview();
		OpaqueWidget::step();
	}

	// The mouse

	/** Where a note starts on the roll and how long it is drawn, in mm: a drone to the roll's end. */
	void noteSpan(const Layout& L, int step, const Note& n, float* x, float* w) const {
		int length = n.length ? n.length : settings().noteLength;
		*x = L.gridX + (step + n.offset / (float) TICKS) * L.cw;
		*w = length >= DRONE ? L.gridX + L.gridW - *x : std::max(length / (float) TICKS * L.cw, 0.6f);
		*w = std::min(*w, L.gridX + L.gridW - *x);
	}

	/** The roll's cell under a point, and the note there if any. */
	Hit hit(const Layout& L, math::Vec p) {
		Hit h;
		h.step = (int) std::floor((p.x - L.gridX) / L.cw);
		if (p.x < L.gridX || h.step < 0 || h.step >= STEPS || p.y < L.gridY || p.y > L.gridY + L.gridH)
			return h;
		h.roll = true;
		h.pitch = low() + ROWS - 1 - (int) std::floor((p.y - L.gridY) / L.rowH);
		const Pattern& pat = pattern();
		for (int s = 0; s < STEPS; s++) {
			int i = pat.steps[s].indexOf(h.pitch);
			if (i < 0)
				continue;
			float x, w;
			noteSpan(L, s, pat.steps[s].notes[i], &x, &w);
			if (p.x < x || p.x > x + std::max(w, 1.f))
				continue;
			h.noteStep = s;
			h.x = x;
			h.w = w;
			h.edge = p.x > x + w - std::min(1.f, w / 3);
			return h;
		}
		return h;
	}

	/** The notes of editable steps that a box in mm touches. */
	std::vector<std::pair<int, int>> inBox(const Layout& L, math::Vec a, math::Vec b) {
		math::Rect r = math::Rect::fromCorners(a, b);
		std::vector<std::pair<int, int>> out;
		int lo = low(), top = lo + ROWS - 1;
		const Pattern& pat = pattern();
		for (int s = 0; s < STEPS; s++) {
			if (!editable(s))
				continue;
			for (int i = 0; i < pat.steps[s].count; i++) {
				const Note& n = pat.steps[s].notes[i];
				float x, w, y = L.gridY + (top - n.pitch) * L.rowH;
				noteSpan(L, s, n, &x, &w);
				if (n.pitch >= lo && n.pitch <= top && x < r.getRight() && x + w > r.pos.x && y < r.getBottom() && y + L.rowH > r.pos.y)
					out.push_back({s, n.pitch});
			}
		}
		return out;
	}

	void onButton(const ButtonEvent& e) override {
		// A right-click on a step opens its menu; elsewhere it goes through to the module's.
		if (e.button == GLFW_MOUSE_BUTTON_RIGHT) {
			Widget::onButton(e);
			if (!e.isConsumed() && e.action == GLFW_PRESS && module && linked && stepMenu(Layout(box.size), px2mm(e.pos)))
				e.consume(this);
			return;
		}
		OpaqueWidget::onButton(e);
		if (e.getTarget() != this || e.action != GLFW_PRESS || e.button != GLFW_MOUSE_BUTTON_LEFT || !module || !linked)
			return;
		typed.clear();
		Layout L(box.size);
		math::Vec p = px2mm(e.pos);
		drag = DragState();
		drag.start = drag.at = p;
		drag.mods = e.mods & RACK_MOD_MASK;
		drag.tool = tool();
		if (overview())
			return clickOverview(L, p);
		int step = (int) std::floor((p.x - L.gridX) / L.cw);
		if (p.x >= L.gridX && step >= 0 && step < STEPS && p.y >= L.laneY && p.y <= L.laneY + L.laneH) {
			if (int l = lane(); l && drag.mods == RACK_MOD_CTRL) {
				Step s = pattern().steps[step];
				if (editable(step) && s.locked(l - 1)) {
					s.unlock(l - 1);
					edit([&] { put(step, s); });
				}
				return;
			}
			drag.kind = LANE;
			return;
		}
		drag.hit = hit(L, p);
		if (!drag.hit.roll)
			return;
		cursorStep = drag.hit.step;
		cursorPitch = drag.hit.pitch;
		drag.kind = PRESS;
	}

	void clickOverview(const Layout& L, math::Vec p) {
		int t = (int) std::floor((p.y - L.ovY) / L.ovH);
		if (t < 0 || t >= LANES)
			return;
		int step = (int) std::floor((p.x - L.ovX) / L.ocw);
		if (p.x >= L.ovX)
			cursorStep = clamp(step, 0, STEPS - 1);
		selectTrack(t);
		if (p.x < L.ovX)
			return;
		if (!editable(step))
			return flash(step);
		module->overview = false;
	}

	void onDragStart(const DragStartEvent& e) override {
		if (e.button != GLFW_MOUSE_BUTTON_LEFT || drag.kind != LANE)
			return;
		begin();
		dragLane();
	}

	/** A press that has moved 3 px: what it drags depends on the tool, what's under it and the gesture settings. */
	void startDrag(const Layout& L) {
		const Hit& h = drag.hit;
		begin();
		if (drag.tool == ERASER) {
			drag.kind = ERASE;
			return erase(L, drag.start);
		}
		if (h.noteStep >= 0) {
			if (!editable(h.noteStep)) {
				flash(h.noteStep);
				drag.kind = NONE;
				return;
			}
			std::pair<int, int> pressed{h.noteStep, h.pitch};
			if (!selected.count(pressed)) {
				if (!(drag.mods & GLFW_MOD_SHIFT))
					selected.clear();
				selected.insert(pressed);
			}
			std::vector<std::pair<int, int>> notes = selection();
			drag.lead = std::find(notes.begin(), notes.end(), pressed) - notes.begin();
			for (auto [s, p] : notes)
				drag.items.push_back(moving(s, p));
			if (h.edge) {
				drag.kind = LENGTH;
				for (Moving& m : drag.items)
					m.note.length = lengthOf(m.step, m.note);
				drag.leadLength = drag.items[drag.lead].note.length;
				return;
			}
			drag.kind = MOVE;
			drag.copies = drag.mods & RACK_MOD_CTRL; // Alt-drag is Rack's, to scroll the rack
			drag.before = selected;
			drag.base = drag.copies ? pattern() : without(notes);
			return;
		}
		if (drag.tool == PEN) {
			if (!addNote(h.step, h.pitch)) {
				drag.kind = NONE;
				return;
			}
			selected = {{h.step, h.pitch}};
			drag.kind = LENGTH;
			drag.items = {moving(h.step, h.pitch)};
			drag.items[0].note.length = lengthOf(h.step, drag.items[0].note);
			drag.leadLength = drag.items[0].note.length;
			drag.hit.x = L.gridX + h.step * L.cw;
			return;
		}
		if (gesture(EMPTY_DRAG) == 1) {
			drag.kind = PAINT;
			return paint(L, drag.start);
		}
		drag.kind = BOX;
		drag.before = drag.mods & GLFW_MOD_SHIFT ? selected : std::set<std::pair<int, int>>();
	}

	void erase(const Layout& L, math::Vec p) {
		Hit h = hit(L, p);
		if (h.noteStep >= 0 && editable(h.noteStep))
			removeNotes({{h.noteStep, h.pitch}});
	}

	void paint(const Layout& L, math::Vec p) {
		Hit h = hit(L, p);
		if (h.roll && h.noteStep < 0 && editable(h.step) && pattern().steps[h.step].count < STEP_NOTES)
			addNote(h.step, h.pitch);
	}

	void onDragMove(const DragMoveEvent& e) override {
		if (drag.kind == NONE)
			return;
		drag.at = drag.at.plus(px2mm(e.mouseDelta.div(getAbsoluteZoom())));
		drag.travel += e.mouseDelta.norm();
		Layout L(box.size);
		if (drag.kind == PRESS) {
			if (drag.travel < 3.f)
				return;
			startDrag(L);
		}
		math::Vec d = drag.at.minus(drag.start);
		bool loose = freeTiming(mods());
		switch (drag.kind) {
			case LANE: return dragLane();
			case ERASE: return erase(L, drag.at);
			case PAINT: return paint(L, drag.at);
			case BOX: {
				selected = drag.before;
				for (auto n : inBox(L, drag.start, drag.at))
					selected.insert(n);
				return;
			}
			case MOVE: {
				float steps = d.x / L.cw;
				int ticks = loose ? (int) std::lround(steps * TICKS) : (int) std::lround(steps) * TICKS;
				int semitones = (int) std::lround(-d.y / L.rowH);
				Pattern out = drag.base;
				std::vector<std::pair<int, int>> placed;
				if (!shifted(out, drag.items, ticks, semitones, !loose, placed))
					return;
				put(out);
				drag.placed = true;
				selected = {placed.begin(), placed.end()};
				std::tie(cursorStep, cursorPitch) = placed[drag.lead];
				return;
			}
			case LENGTH: {
				Pattern p = pattern();
				bool drone = drag.at.x > L.gridX + L.gridW + 1.f;
				float ticks = (drag.at.x - drag.hit.x) / L.cw * TICKS;
				int length = loose ? (int) std::lround(ticks) : std::max(1, (int) std::lround(ticks / TICKS)) * TICKS;
				for (const Moving& m : drag.items) {
					Step& s = p.steps[m.step];
					int i = s.indexOf(m.note.pitch);
					if (i >= 0)
						setLength(m.step, s.notes[i], drone ? DRONE : m.note.length + length - drag.leadLength);
				}
				return put(p);
			}
			default: return;
		}
	}

	void dragLane() {
		Layout L(box.size);
		int step = (int) std::floor((drag.at.x - L.gridX) / L.cw);
		if (step >= 0 && step < STEPS)
			setLane(step, (L.laneY + L.laneH - drag.at.y) / L.laneH);
	}

	void onDragEnd(const DragEndEvent& e) override {
		if (e.button != GLFW_MOUSE_BUTTON_LEFT)
			return;
		if (drag.kind == PRESS)
			edit([&] { click(drag.hit); });
		if (drag.kind == MOVE && drag.copies && !drag.placed)
			selected = drag.before;
		drag.kind = NONE;
		end();
	}

	/** A click: what it does depends on the tool, on what's under it and on the gesture settings. */
	void click(const Hit& h) {
		double now = system::getTime();
		std::pair<int, int> cell{h.step, h.pitch};
		bool twice = now - lastClickAt < 0.3 && lastCell == cell && !lastClickAdded;
		lastClickAt = now;
		lastCell = cell;
		lastClickAdded = false;
		bool shift = drag.mods & GLFW_MOD_SHIFT;
		if (h.noteStep >= 0) {
			std::pair<int, int> note{h.noteStep, h.pitch};
			if (!editable(h.noteStep))
				return flash(h.noteStep);
			if (drag.tool == ERASER || (twice && gesture(NOTE_DOUBLE) == 0) || gesture(NOTE_CLICK) == 1) {
				removeNotes({note});
				lastClickAt = -1.0;
			}
			else if (shift && selected.count(note))
				selected.erase(note);
			else {
				if (!shift)
					selected.clear();
				selected.insert(note);
			}
			return;
		}
		if (drag.tool == ERASER)
			return;
		if (drag.tool == PEN || (twice && gesture(EMPTY_DOUBLE) == 0) || gesture(EMPTY_CLICK) == 1) {
			if (addNote(h.step, h.pitch)) {
				selected = {{h.step, h.pitch}};
				lastClickAdded = true;
			}
			return;
		}
		if (!shift)
			selected.clear();
	}

	void onHover(const HoverEvent& e) override {
		hover = px2mm(e.pos);
		OpaqueWidget::onHover(e);
	}

	void onLeave(const LeaveEvent& e) override {
		hover = math::Vec(-1.f, -1.f);
	}

	void onHoverScroll(const HoverScrollEvent& e) override {
		// Only while the screen has the keys, so a scroll over it still moves round the rack otherwise.
		if (!focused() || overview() || e.scrollDelta.y == 0.f)
			return;
		scroll((e.scrollDelta.y > 0.f ? 1 : -1) * (mods() == GLFW_MOD_SHIFT ? 12 : 1));
		e.consume(this);
	}

	/** A right-click on a step: copy, paste and clear it, and set its components. False if it isn't on a step. */
	bool stepMenu(const Layout& L, math::Vec p) {
		int step = -1;
		if (overview()) {
			int t = (int) std::floor((p.y - L.ovY) / L.ovH);
			if (p.x >= L.ovX && t >= 0 && t < LANES) {
				selectTrack(t);
				step = (int) std::floor((p.x - L.ovX) / L.ocw);
			}
		}
		else if (p.x >= L.gridX && p.y >= L.gridY)
			step = (int) std::floor((p.x - L.gridX) / L.cw);
		if (step < 0 || step >= STEPS || (!overview() && p.y > L.compY + L.compH))
			return false;
		if (!editable(step)) {
			flash(step);
			return true;
		}
		cursorStep = step;
		static const char* const NAMES[COMPONENTS_LEN] = {"Pulse", "Pulse hold", "Multiply", "Velocity", "Ramp up", "Ramp down",
			"Random", "Portamento", "Sweep", "Tonality", "Jump", "Parameter spark", "Component spark", "Trigger spark"};
		auto change = [this, step](std::function<void(Step&)> f) {
			Step s = pattern().steps[step];
			f(s);
			edit([&] { put(step, s); });
		};
		ui::Menu* menu = createMenu();
		menu->addChild(createMenuLabel(string::f("Step %d", step + 1)));
		menu->addChild(createMenuItem("Copy step", "", [=] { clipboard().notes = false; clipboard().step = pattern().steps[step]; }));
		menu->addChild(createMenuItem("Paste step", "", [=] { if (!clipboard().notes) change([](Step& s) { s = clipboard().step; }); }, clipboard().notes));
		menu->addChild(createMenuItem("Clear notes", "", [=] { change([](Step& s) { s.count = 0; }); }));
		menu->addChild(createMenuItem("Clear locks", "", [=] { change([](Step& s) { s.locks = 0; std::fill(std::begin(s.lockValues), std::end(s.lockValues), 0.f); }); }));
		menu->addChild(createMenuItem("Clear components", "", [=] { change([](Step& s) { s.clearComponents(); }); }));
		menu->addChild(new ui::MenuSeparator);
		menu->addChild(createMenuLabel("Components"));
		const Step& s = pattern().steps[step];
		for (int c = 0; c < COMPONENTS_LEN; c++) {
			std::string value = s.has(c) ? string::f("key %d", (s.key(c) + 1) % 10) : "";
			menu->addChild(createSubmenuItem(NAMES[c], value, [=](ui::Menu* sub) {
				sub->addChild(createCheckMenuItem("Off", "", [=] { return !pattern().steps[step].has(c); }, [=] { change([=](Step& s) { s.unset(c); }); }));
				for (int k = 0; k < 10; k++)
					sub->addChild(createCheckMenuItem(string::f("Value key %d", (k + 1) % 10), "",
						[=] { const Step& s = pattern().steps[step]; return s.has(c) && s.key(c) == k; },
						[=] { change([=](Step& s) { s.set(c, (s.values[c] & 0x30) | k); }); }));
			}));
		}
		return true;
	}

	// The computer's keys, while the screen has them

	void onSelectKey(const SelectKeyEvent& e) override {
		int mods = e.mods & RACK_MOD_MASK;
		// D picks the Pen; held, it's the Pen only until it's let go.
		if (e.key == GLFW_KEY_D && module && !(mods & (RACK_MOD_CTRL | GLFW_MOD_ALT))) {
			if (e.action == GLFW_PRESS) {
				toolBeforeD = module->tool;
				pressedD = system::getTime();
				module->tool = PEN;
			}
			else if (e.action == GLFW_RELEASE && toolBeforeD >= 0) {
				if (system::getTime() - pressedD > 0.4)
					module->tool = toolBeforeD;
				toolBeforeD = -1;
			}
			e.consume(this);
			return;
		}
		if (e.action != GLFW_PRESS && e.action != GLFW_REPEAT)
			return;
		bool ctrlDown = mods & RACK_MOD_CTRL, shift = mods & GLFW_MOD_SHIFT, alt = mods & GLFW_MOD_ALT;
		// Rack's undo, redo and save go through.
		if (ctrlDown && (e.key == GLFW_KEY_Z || e.key == GLFW_KEY_Y || e.key == GLFW_KEY_S))
			return;
		if (!module || !linked) {
			if (e.key == GLFW_KEY_ESCAPE) {
				APP->event->setSelectedWidget(NULL);
				e.consume(this);
			}
			return;
		}
		bool rolled = !overview();
		bool plain = !ctrlDown && !alt;
		int digit = e.key >= GLFW_KEY_0 && e.key <= GLFW_KEY_9 ? e.key - GLFW_KEY_0
			: e.key >= GLFW_KEY_KP_0 && e.key <= GLFW_KEY_KP_9 ? e.key - GLFW_KEY_KP_0 : -1;
		int dx = e.key == GLFW_KEY_RIGHT ? 1 : e.key == GLFW_KEY_LEFT ? -1 : 0;
		int dy = e.key == GLFW_KEY_UP ? 1 : e.key == GLFW_KEY_DOWN ? -1 : 0;
		if (e.key == GLFW_KEY_ESCAPE) {
			if (!selection().empty())
				selected.clear();
			else
				APP->event->setSelectedWidget(NULL);
		}
		else if (digit >= 0 && !ctrlDown) {
			typed += char('0' + digit);
			if (typed.size() == 2) {
				int value = std::stoi(typed);
				typed.clear();
				edit([&] { setLane(cursorStep, value / 99.f); });
			}
		}
		else if (e.key == GLFW_KEY_TAB)
			selectTrack(track() + (shift ? -1 : 1));
		else if (e.key == GLFW_KEY_V && plain)
			module->overview = !module->overview;
		else if (e.key == GLFW_KEY_P && plain)
			module->tool = POINTER;
		else if (e.key == GLFW_KEY_E && plain)
			module->tool = ERASER;
		else if (e.key == GLFW_KEY_L && plain)
			module->lane = (module->lane + (shift ? LOCK_LANES : 1)) % (LOCK_LANES + 1);
		else if (e.key == GLFW_KEY_PAGE_UP || e.key == GLFW_KEY_PAGE_DOWN)
			scroll(e.key == GLFW_KEY_PAGE_UP ? 12 : -12);
		else if (ctrlDown && e.key == GLFW_KEY_A && rolled) {
			selected.clear();
			for (int s = 0; s < STEPS; s++)
				for (int i = 0; editable(s) && i < pattern().steps[s].count; i++)
					selected.insert({s, pattern().steps[s].notes[i].pitch});
		}
		else if (e.key == GLFW_KEY_DELETE || e.key == GLFW_KEY_BACKSPACE) {
			std::vector<std::pair<int, int>> notes = selection();
			if (!notes.empty())
				edit([&] { removeNotes(notes); });
			else if (editable(cursorStep))
				edit([&] { put(cursorStep, Step()); });
		}
		else if (ctrlDown && e.key == GLFW_KEY_C)
			copy();
		else if (ctrlDown && e.key == GLFW_KEY_V)
			paste();
		else if (e.key == GLFW_KEY_ENTER || e.key == GLFW_KEY_KP_ENTER || e.key == GLFW_KEY_SPACE) {
			if (rolled)
				edit([&] {
					if (noteAt(cursorStep, cursorPitch) >= 0)
						removeNotes({{cursorStep, cursorPitch}});
					else
						addNote(cursorStep, cursorPitch);
				});
			else if (editable(cursorStep))
				module->overview = false;
		}
		else if (alt && e.key == GLFW_KEY_D)
			lengthen(0, true);
		else if (dx || dy) {
			if (!rolled) {
				cursorStep = clamp(cursorStep + dx, 0, STEPS - 1);
				if (dy)
					selectTrack(clamp(track() - dy, 0, LANES - 1));
			}
			else if (alt && dx)
				lengthen(dx * (ctrlDown ? 1 : TICKS), false);
			else if (shift && dx)
				nudge(dx * (ctrlDown ? 1 : TICKS), 0, !ctrlDown);
			else if (shift && dy)
				nudge(0, dy * (ctrlDown ? 12 : 1), true);
			else {
				cursorStep = clamp(cursorStep + dx, 0, STEPS - 1);
				cursorPitch = clamp(cursorPitch + dy, lowest(), highest());
				reveal();
			}
		}
		else
			return;
		e.consume(this);
	}

	void onDeselect(const DeselectEvent& e) override {
		typed.clear();
		end();
	}

	// Drawing

	NVGcontext* vg = nullptr;
	std::shared_ptr<window::Font> font;
	const NVGcolor ink = nvgRGB(0xdc, 0xdb, 0xd8), dim = nvgRGBA(0xdc, 0xdb, 0xd8, 0x70), rule = nvgRGBA(0xdc, 0xdb, 0xd8, 0x38);
	const NVGcolor faint = nvgRGBA(0xdc, 0xdb, 0xd8, 0x1c), glass = nvgRGB(0x0e, 0x0e, 0x10), lit = PAGE_COLORS[0];
	const NVGcolor none = nvgRGBA(0, 0, 0, 0);

	void text(float x, float y, const std::string& s, NVGcolor c, float size, int align = NVG_ALIGN_LEFT) {
		nvgFontSize(vg, mm2px(size));
		nvgTextAlign(vg, align | NVG_ALIGN_MIDDLE);
		nvgFillColor(vg, c);
		nvgText(vg, mm2px(x), mm2px(y), s.c_str(), NULL);
	}

	void rect(float x, float y, float w, float h, NVGcolor fill, NVGcolor stroke = nvgRGBA(0, 0, 0, 0), float radius = 0.8f, float width = 1.f) {
		nvgBeginPath(vg);
		nvgRoundedRect(vg, mm2px(x), mm2px(y), mm2px(w), mm2px(h), mm2px(radius));
		if (fill.a > 0.f) {
			nvgFillColor(vg, fill);
			nvgFill(vg);
		}
		if (stroke.a > 0.f) {
			nvgStrokeColor(vg, stroke);
			nvgStrokeWidth(vg, width);
			nvgStroke(vg);
		}
	}

	void line(float x0, float y0, float x1, float y1, NVGcolor c, float width = 1.f) {
		nvgBeginPath(vg);
		nvgMoveTo(vg, mm2px(x0), mm2px(y0));
		nvgLineTo(vg, mm2px(x1), mm2px(y1));
		nvgStrokeColor(vg, c);
		nvgStrokeWidth(vg, width);
		nvgStroke(vg);
	}

	/** A step past the step count: dark, with faint hatching. */
	void hatch(float x, float y, float w, float h) {
		rect(x, y, w, h, nvgRGBA(0, 0, 0, 0x90), none, 0.f);
		nvgSave(vg);
		nvgScissor(vg, mm2px(x), mm2px(y), mm2px(w), mm2px(h));
		for (float d = -h; d < w; d += 1.6f)
			line(x + d, y + h, x + d + h, y, nvgRGBA(0xff, 0xff, 0xff, 0x0d));
		nvgRestore(vg);
	}

	void field(float x, float y, float w, float h, const char* label, const std::string& value, NVGcolor c, const std::string& note = "") {
		rect(x, y, w, h, nvgRGBA(0xff, 0xff, 0xff, 0x08), rule);
		text(x + 1.f, y + 1.8f, label, dim, 2.2f);
		if (!note.empty())
			text(x + w - 1.f, y + 1.8f, note, ink, 2.2f, NVG_ALIGN_RIGHT);
		text(x + w / 2, y + h / 2 + 1.1f, value, c, 3.4f, NVG_ALIGN_CENTER);
	}

	void drawLayer(const DrawArgs& args, int layer) override {
		if (layer == 1) {
			vg = args.vg;
			font = APP->window->loadFont(asset::system("res/fonts/ShareTechMono-Regular.ttf"));
			if (font) {
				nvgSave(vg);
				nvgScissor(vg, 0, 0, box.size.x, box.size.y);
				nvgFontFaceId(vg, font->handle);
				drawScreen();
				nvgRestore(vg);
			}
		}
		OpaqueWidget::drawLayer(args, layer);
	}

	void draw(const DrawArgs& args) override {
		// The outline that shows the screen has the computer's keys, round the glass.
		if (focused()) {
			nvgBeginPath(args.vg);
			nvgRoundedRect(args.vg, -mm2px(1.4f), -mm2px(1.4f), box.size.x + mm2px(2.8f), box.size.y + mm2px(2.8f), mm2px(3.f));
			nvgStrokeColor(args.vg, lit);
			nvgStrokeWidth(args.vg, mm2px(0.45f));
			nvgStroke(args.vg);
		}
		OpaqueWidget::draw(args);
	}

	void drawScreen() {
		Layout L(box.size);
		if (!linked) {
			text(L.w / 2, L.h / 2, "PUT ME LEFT OF A SEQUENCER", dim, 3.8f, NVG_ALIGN_CENTER);
			return;
		}
		drawHeader(L);
		if (overview())
			drawOverview(L);
		else
			drawRoll(L);
	}

	static std::string twoDigits(int n) {
		return string::f("%02d", n);
	}

	std::string rowName(int pitch) {
		if (!drum())
			return noteName(pitch);
		Hub* h = hub();
		TrackCore* core = h ? h->trackCore(track()) : nullptr;
		std::string name = core ? core->soundName(pitch) : string::f("sound %d", pitch - PAD_LOW + 1);
		return string::uppercase(name.substr(0, 8));
	}

	std::string laneName() {
		int l = lane();
		if (!l)
			return "VEL";
		Hub* h = hub();
		std::string name = h ? h->lockName(track(), l - 1) : "";
		return string::uppercase(name.empty() ? string::f("P%d", l) : name.substr(0, 6));
	}

	void drawHeader(const Layout& L) {
		int t = track();
		const Settings& s = settings();
		Hub* h = hub();
		bool playing = view.pattern == view.playingPattern;
		std::string plug = h && t != MASTER ? string::uppercase(h->plugName(t)) : "-";
		std::string length = s.noteLength >= DRONE ? "DRONE" : string::f("%g", s.noteLength / (double) TICKS);
		struct F { float w; const char* label; std::string value; NVGcolor color; std::string note; };
		F fields[] = {
			{12.f, "PAT", "P" + twoDigits(view.pattern + 1), playing ? lit : ink, playing && view.playing ? ">" : ""},
			{24.f, "TRACK", string::f("%d %s", trackNumber(t), string::uppercase(TRACK_NAMES[t]).c_str()), LANE_COLORS[t], module->follow ? "FOLLOW" : ""},
			{21.f, "PLUG", plug.substr(0, 10), ink, ""},
			{11.f, "STEPS", std::to_string(clamp(s.stepCount, 1, STEPS)), ink, ""},
			{10.f, "STEP", s.stepLength ? string::f("x%d", s.stepLength) : "TRIG", ink, ""},
			{11.f, "NOTE", length, ink, ""},
		};
		float x = 1.5f, y = 1.f, hh = 8.f;
		for (const F& f : fields) {
			field(x, y, f.w, hh, f.label, f.value, f.color, f.note);
			x += f.w + 1.5f;
		}
		float rest = L.w - 1.5f - x;
		if (rest < 22.f)
			return;
		std::string readout = "S" + twoDigits(cursorStep + 1) + " ";
		if (overview())
			readout += string::uppercase(TRACK_NAMES[t]);
		else {
			readout += drum() ? rowName(cursorPitch) : noteName(cursorPitch);
			int i = noteAt(cursorStep, cursorPitch);
			if (i >= 0) {
				const Note& n = pattern().steps[cursorStep].notes[i];
				readout += string::f(" V%d %+dt", n.velocity, n.offset);
			}
		}
		bool keys = focused();
		size_t count = overview() ? 0 : selection().size();
		std::string note = !typed.empty() ? laneName() + " " + typed + "_" : count ? string::f("%d SEL", (int) count) : "";
		field(x, y, rest, hh, keys ? "CURSOR" : "CURSOR (CLICK TO EDIT)", readout, keys ? ink : dim, note);
	}

	void drawRoll(const Layout& L) {
		int t = track(), lo = low(), top = lo + ROWS - 1, steps = clamp(settings().stepCount, 1, STEPS), playhead = view.positions[t];
		const Pattern& pat = pattern();
		NVGcolor color = LANE_COLORS[t];
		bool keys = focused();
		auto rowY = [&](int pitch) { return L.gridY + (top - pitch) * L.rowH; };
		// The keys or sounds down the side, and the rows' shading.
		for (int p = lo; p <= top; p++) {
			float y = rowY(p);
			bool shaded = drum() ? (p - PAD_LOW) / 4 % 2 : (0x54a >> (p % 12)) & 1; // a drum kit's fours, a keyboard's black keys
			rect(L.keyX, y + 0.05f, L.keyW, L.rowH - 0.1f, nvgRGBA(0xff, 0xff, 0xff, shaded ? 0x0c : 0x16), none, 0.3f);
			if (shaded)
				rect(L.gridX, y, L.gridW, L.rowH, nvgRGBA(0xff, 0xff, 0xff, 0x05), none, 0.f);
			bool named = drum() || p % 12 == 0 || p == cursorPitch;
			if (named && L.rowH > 1.6f)
				text(L.keyX + L.keyW - 0.8f, y + L.rowH / 2, rowName(p), p == cursorPitch && keys ? ink : dim, std::min(2.3f, L.rowH * 0.95f), NVG_ALIGN_RIGHT);
			if (!drum() && p % 12 == 0)
				line(L.gridX, y + L.rowH, L.gridX + L.gridW, y + L.rowH, rule);
		}
		// The steps: those past the step count greyed, the playhead's lit, a line every four.
		for (int s = 0; s < STEPS; s++) {
			float x = L.gridX + s * L.cw;
			if (s >= steps) {
				hatch(x, L.gridY, L.cw, L.gridH);
				hatch(x, L.laneY, L.cw, L.laneH);
				hatch(x, L.compY, L.cw, L.compH);
			}
			if (s == playhead)
				rect(x, L.gridY, L.cw, L.gridH, nvgTransRGBAf(lit, 0.12f), none, 0.f);
			line(x, L.gridY, x, L.gridY + L.gridH, s % 4 ? faint : rule);
			text(x + L.cw / 2, L.gridY - 1.6f, std::to_string(s + 1), s == playhead ? lit : s == cursorStep && keys ? ink : dim, 2.1f, NVG_ALIGN_CENTER);
		}
		rect(L.gridX, L.gridY, L.gridW, L.gridH, none, rule, 0.f);
		if (keys && cursorPitch >= lo && cursorPitch <= top)
			rect(L.gridX + cursorStep * L.cw, rowY(cursorPitch), L.cw, L.rowH, none, ink, 0.3f, 1.2f);
		// The notes, those off the roll as arrows at its edge.
		float end = L.gridX + L.gridW;
		for (int s = 0; s < STEPS; s++)
			for (int i = 0; i < pat.steps[s].count; i++) {
				const Note& n = pat.steps[s].notes[i];
				if (n.pitch < lo || n.pitch > top) {
					bool up = n.pitch > top;
					float x = L.gridX + (s + 0.5f) * L.cw, y = up ? L.gridY + 0.4f : L.gridY + L.gridH - 0.4f, d = up ? 1.f : -1.f;
					nvgBeginPath(vg);
					nvgMoveTo(vg, mm2px(x - 0.9f), mm2px(y + d));
					nvgLineTo(vg, mm2px(x + 0.9f), mm2px(y + d));
					nvgLineTo(vg, mm2px(x), mm2px(y));
					nvgFillColor(vg, color);
					nvgFill(vg);
					continue;
				}
				float x, w, y = rowY(n.pitch);
				noteSpan(L, s, n, &x, &w);
				w = std::min(w, end - x);
				bool ghost = s >= steps, chosen = !ghost && selected.count({s, (int) n.pitch});
				float a = ghost ? 0.18f : chosen ? 1.f : 0.8f * (0.35f + 0.65f * n.velocity / 127.f);
				rect(x, y + 0.15f, w, L.rowH - 0.3f, nvgTransRGBAf(color, a), chosen ? nvgRGB(0xff, 0xff, 0xff) : none, 0.35f, 1.2f);
				rect(x, y + 0.15f, 0.35f, L.rowH - 0.3f, ghost ? dim : nvgRGBA(0xff, 0xff, 0xff, 0xd0), none, 0.f);
				if (n.length == DRONE || (!n.length && settings().noteLength >= DRONE))
					text(end - 0.4f, y + L.rowH / 2, ">", glass, std::min(2.4f, L.rowH), NVG_ALIGN_RIGHT);
				if (keys && s == cursorStep && n.pitch == cursorPitch)
					rect(x - 0.2f, y, w + 0.4f, L.rowH, none, nvgRGB(0xff, 0xff, 0xff), 0.4f, 1.2f);
			}
		if (flashStep >= 0 && system::getTime() - flashAt < 0.3)
			rect(L.gridX + flashStep * L.cw, L.gridY, L.cw, L.gridH, nvgRGBA(0xff, 0x4d, 0x3d, 0x40), none, 0.f);
		drawPointer(L);
		drawLane(L, pat, color);
		drawComponents(L, pat);
	}

	/** What a press would do, as Rack's pointer can't change shape: the box being dragged, the note's end that sets
	its length, the note the Eraser would take, the cell the Pen would fill. */
	void drawPointer(const Layout& L) {
		if (drag.kind == BOX) {
			math::Rect r = math::Rect::fromCorners(drag.start, drag.at);
			rect(r.pos.x, r.pos.y, r.size.x, r.size.y, nvgRGBA(0xff, 0xff, 0xff, 0x10), ink, 0.2f, 1.f);
			return;
		}
		if (drag.kind != NONE || hover.x < 0.f)
			return;
		Hit h = hit(L, hover);
		if (!h.roll || (h.noteStep >= 0 && !editable(h.noteStep)))
			return;
		float y = L.gridY + (low() + ROWS - 1 - h.pitch) * L.rowH;
		int t = tool();
		if (h.noteStep >= 0 && t == ERASER)
			rect(h.x, y, h.w, L.rowH, nvgRGBA(0xff, 0x4d, 0x3d, 0x70), none, 0.35f);
		else if (h.noteStep >= 0 && h.edge)
			rect(h.x + h.w - 0.5f, y - 0.2f, 0.5f, L.rowH + 0.4f, nvgRGB(0xff, 0xff, 0xff), none, 0.2f);
		else if (h.noteStep >= 0)
			rect(h.x - 0.15f, y, h.w + 0.3f, L.rowH, none, nvgRGBA(0xff, 0xff, 0xff, 0x90), 0.4f, 1.f);
		else if (t == PEN && editable(h.step))
			rect(L.gridX + h.step * L.cw, y, L.cw, L.rowH, nvgRGBA(0xff, 0xff, 0xff, 0x14), rule, 0.3f, 1.f);
	}

	void drawLane(const Layout& L, const Pattern& pat, NVGcolor trackColor) {
		int l = lane();
		NVGcolor color = l ? DIAL_COLORS[(l - 1) % 4] : trackColor;
		bool keys = focused();
		float side = L.gridX - 2.5f;
		rect(1.5f, L.laneY, side, L.laneH, nvgRGBA(0xff, 0xff, 0xff, 0x10), none, 0.5f);
		text(1.5f + side / 2, L.laneY + 3.f, laneName(), color, 2.4f, NVG_ALIGN_CENTER);
		if (l)
			text(1.5f + side / 2, L.laneY + L.laneH - 2.5f, "LOCKS", dim, 2.f, NVG_ALIGN_CENTER);
		rect(L.gridX, L.laneY, L.gridW, L.laneH, none, rule, 0.f);
		for (float f : {0.25f, 0.5f, 0.75f})
			line(L.gridX, L.laneY + L.laneH * f, L.gridX + L.gridW, L.laneY + L.laneH * f, faint);
		for (int s = 0; s < STEPS; s++) {
			const Step& step = pat.steps[s];
			float x = L.gridX + s * L.cw;
			line(x, L.laneY, x, L.laneY + L.laneH, s % 4 ? faint : rule);
			if (l) {
				if (!step.locked(l - 1)) {
					line(x + L.cw * 0.3f, L.laneY + L.laneH - 0.6f, x + L.cw * 0.7f, L.laneY + L.laneH - 0.6f, dim);
					continue;
				}
				float v = step.lockValues[l - 1];
				rect(x + L.cw * 0.2f, L.laneY + L.laneH * (1.f - v), L.cw * 0.6f, L.laneH * v, nvgTransRGBAf(color, 0.85f), none, 0.3f);
				continue;
			}
			float bar = L.cw * 0.7f / std::max(step.count, 1);
			for (int i = 0; i < step.count; i++) {
				const Note& n = step.notes[i];
				float v = n.velocity / 127.f;
				bool cursor = keys && s == cursorStep && n.pitch == cursorPitch;
				rect(x + L.cw * 0.15f + i * bar + 0.1f, L.laneY + L.laneH * (1.f - v), bar - 0.2f, L.laneH * v, nvgTransRGBAf(color, cursor ? 1.f : 0.7f), none, 0.2f);
			}
		}
	}

	/** Each step's components, by short name: the first, and + for more. A right-click on the step sets them. */
	void drawComponents(const Layout& L, const Pattern& pat) {
		static const char* const NAMES[COMPONENTS_LEN] = {"PLS", "PHD", "MUL", "VEL", "R+", "R-", "RND", "POR", "SWP", "TON", "JMP", "P*", "C*", "T*"};
		rect(1.5f, L.compY, L.gridX - 2.5f, L.compH, nvgRGBA(0xff, 0xff, 0xff, 0x10), none, 0.5f);
		text(1.5f + (L.gridX - 2.5f) / 2, L.compY + L.compH / 2, "COMP", dim, 2.1f, NVG_ALIGN_CENTER);
		for (int s = 0; s < STEPS; s++) {
			const Step& step = pat.steps[s];
			if (!step.components)
				continue;
			int first = 0;
			while (!step.has(first))
				first++;
			float x = L.gridX + s * L.cw;
			rect(x + 0.3f, L.compY + 0.3f, L.cw - 0.6f, L.compH - 0.6f, nvgRGBA(0xff, 0xff, 0xff, 0x14), none, 0.4f);
			bool more = step.components & ~(1u << first);
			text(x + L.cw / 2, L.compY + L.compH / 2, std::string(NAMES[first]) + (more ? "+" : ""), ink, std::min(2.3f, L.cw * 0.32f), NVG_ALIGN_CENTER);
		}
	}

	void drawOverview(const Layout& L) {
		bool keys = focused();
		for (int s = 0; s < STEPS; s++)
			text(L.ovX + (s + 0.5f) * L.ocw, L.ovY - 1.6f, std::to_string(s + 1), s == cursorStep && keys ? ink : dim, 2.1f, NVG_ALIGN_CENTER);
		for (int t = 0; t < LANES; t++) {
			const Pattern& pat = view.lanes[t];
			int steps = clamp(view.settings[t].stepCount, 1, STEPS), playhead = view.positions[t];
			float y = L.ovY + t * L.ovH;
			bool selected = t == track();
			NVGcolor color = LANE_COLORS[t];
			rect(1.5f, y + 0.2f, L.labelW, L.ovH - 0.4f, selected ? color : nvgRGBA(0xff, 0xff, 0xff, 0x0c), none, 0.5f);
			text(2.5f, y + L.ovH / 2, string::f("%2d %s", trackNumber(t), string::uppercase(TRACK_NAMES[t]).c_str()), selected ? glass : color, std::min(2.8f, L.ovH * 0.5f));
			for (int s = 0; s < STEPS; s++) {
				float x = L.ovX + s * L.ocw;
				if (s >= steps) {
					hatch(x, y, L.ocw, L.ovH);
					continue;
				}
				const Step& step = pat.steps[s];
				rect(x + 0.2f, y + 0.2f, L.ocw - 0.4f, L.ovH - 0.4f, s == playhead ? nvgTransRGBAf(lit, 0.22f) : nvgRGBA(0xff, 0xff, 0xff, s % 4 ? 0x07 : 0x0e), none, 0.4f);
				float d = std::min(L.ocw / 6, L.ovH / 4);
				for (int i = 0; i < step.count; i++) {
					nvgBeginPath(vg);
					nvgCircle(vg, mm2px(x + L.ocw / 2 + (i - (step.count - 1) / 2.f) * d * 2.1f), mm2px(y + L.ovH / 2), mm2px(d * 0.85f));
					nvgFillColor(vg, nvgTransRGBAf(color, 0.35f + 0.65f * step.notes[i].velocity / 127.f));
					nvgFill(vg);
				}
				if (step.components) {
					nvgBeginPath(vg);
					nvgMoveTo(vg, mm2px(x + L.ocw - 0.3f), mm2px(y + 0.3f));
					nvgLineTo(vg, mm2px(x + L.ocw - 1.5f), mm2px(y + 0.3f));
					nvgLineTo(vg, mm2px(x + L.ocw - 0.3f), mm2px(y + 1.5f));
					nvgFillColor(vg, ink);
					nvgFill(vg);
				}
				if (step.locks) {
					nvgBeginPath(vg);
					nvgCircle(vg, mm2px(x + L.ocw - 0.9f), mm2px(y + L.ovH - 0.9f), mm2px(0.45f));
					nvgFillColor(vg, DIAL_COLORS[1]);
					nvgFill(vg);
				}
			}
			if (selected)
				rect(1.4f, y + 0.1f, L.w - 2.9f, L.ovH - 0.2f, none, nvgTransRGBAf(color, 0.6f), 0.5f);
		}
		if (keys)
			rect(L.ovX + cursorStep * L.ocw, L.ovY + track() * L.ovH, L.ocw, L.ovH, none, ink, 0.4f, 1.2f);
		for (int s = 4; s < STEPS; s += 4)
			line(L.ovX + s * L.ocw, L.ovY, L.ovX + s * L.ocw, L.ovY + LANES * L.ovH, rule);
		if (flashStep >= 0 && system::getTime() - flashAt < 0.3)
			rect(L.ovX + flashStep * L.ocw, L.ovY + track() * L.ovH, L.ocw, L.ovH, nvgRGBA(0xff, 0x4d, 0x3d, 0x60), none, 0.4f);
	}
};


/** The left edge, dragged to make the module wider or narrower a whole HP at a time, as far as its neighbours let it.
The right edge stays where it is, against the module on its right. */
struct ResizeHandle : widget::OpaqueWidget {
	PatternEditor* module = nullptr;
	math::Vec from;
	math::Rect original;

	void onDragStart(const DragStartEvent& e) override {
		if (e.button != GLFW_MOUSE_BUTTON_LEFT)
			return;
		from = APP->scene->rack->getMousePos();
		original = getAncestorOfType<ModuleWidget>()->box;
	}

	void onDragMove(const DragMoveEvent& e) override {
		ModuleWidget* mw = getAncestorOfType<ModuleWidget>();
		float dx = APP->scene->rack->getMousePos().x - from.x;
		math::Rect wanted = original, before = mw->box;
		float hp = std::round((original.size.x - dx) / RACK_GRID_WIDTH);
		wanted.size.x = clamp(hp, (float) PatternEditor::MIN_WIDTH, (float) PatternEditor::MAX_WIDTH) * RACK_GRID_WIDTH;
		wanted.pos.x = original.getRight() - wanted.size.x;
		mw->box = wanted;
		if (!APP->scene->rack->requestModulePos(mw, wanted.pos))
			mw->box = before;
		if (module)
			module->width = (int) std::round(mw->box.size.x / RACK_GRID_WIDTH);
	}

	void draw(const DrawArgs& args) override {
		bool dark = settings::preferDarkPanels;
		for (int i = -2; i <= 2; i++) {
			nvgBeginPath(args.vg);
			nvgCircle(args.vg, box.size.x / 2, box.size.y / 2 + i * mm2px(1.6f), mm2px(0.35f));
			nvgFillColor(args.vg, dark ? nvgRGBA(0xff, 0xff, 0xff, 0x40) : nvgRGBA(0, 0, 0, 0x38));
			nvgFill(args.vg);
		}
	}
};


/** The panel is drawn, not an SVG, since it stretches: the colours are res-src/panelkit.py's, the text its Nunito. */
struct PatternEditorWidget : ModuleWidget {
	static constexpr float BEZEL_X = 3.f, BEZEL_Y = 11.f, BEZEL_H = 93.f, ROW_Y = 115.5f, LABEL_Y = 108.6f;
	static constexpr float CAP_HEIGHT = 0.705f; // Nunito Bold's, of its em
	static constexpr float VIEW_X = 12.f, FOLLOW_X = 26.f, LANE_X = 40.f, DOWN_X = 56.f, UP_X = 68.f;

	PatternScreen* screen;
	JogWheel* wheel;
	ResizeHandle* handle;
	widget::Widget* rightScrews[2];
	float laidOut = 0.f;

	PatternEditorWidget(PatternEditor* module) {
		setModule(module);
		box.size = math::Vec((module ? module->width : 32) * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);
		addChild(createWidget<ThemedScrew>(math::Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ThemedScrew>(math::Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		for (int i = 0; i < 2; i++)
			addChild(rightScrews[i] = createWidget<ThemedScrew>(math::Vec(0, i * (RACK_GRID_HEIGHT - RACK_GRID_WIDTH))));

		screen = createWidget<PatternScreen>(mm2px(math::Vec(BEZEL_X + 1.f, BEZEL_Y + 1.f)));
		screen->module = module;
		addChild(screen);

		addParam(createLightParamCentered<VCVLightBezel<GreenLight>>(mm2px(math::Vec(VIEW_X, ROW_Y)), module, PatternEditor::VIEW_PARAM, PatternEditor::VIEW_LIGHT));
		addParam(createLightParamCentered<VCVLightBezel<GreenLight>>(mm2px(math::Vec(FOLLOW_X, ROW_Y)), module, PatternEditor::FOLLOW_PARAM, PatternEditor::FOLLOW_LIGHT));
		addParam(createLightParamCentered<VCVLightBezel<RedGreenBlueLight>>(mm2px(math::Vec(LANE_X, ROW_Y)), module, PatternEditor::LANE_PARAM, PatternEditor::LANE_LIGHT));
		float d = keyDiameter();
		static const char* const symbols[2] = {"minus", "plus"};
		for (int k = 0; k < 2; k++) {
			RoundKey* key = createLightParam<RoundKey>(math::Vec(), module, PatternEditor::OCTAVE_DOWN_PARAM + k, PatternEditor::KEY_LIGHT + 3 * k);
			float x = k ? UP_X : DOWN_X;
			key->setRing(math::Rect(mm2px(math::Vec(x - d / 2, ROW_Y - d / 2)), mm2px(math::Vec(d, d))), symbols[k]);
			addParam(key);
		}

		wheel = createWidget<JogWheel>(math::Vec());
		wheel->box.size = mm2px(math::Vec(9.f, 9.f));
		wheel->cap = DIAL_COLORS[0];
		wheel->turned = [this](int steps) { screen->turnPattern(steps); };
		addChild(wheel);

		handle = createWidget<ResizeHandle>(math::Vec());
		handle->module = module;
		handle->box.size = math::Vec(mm2px(2.5f), box.size.y); // at the left edge, outside the bezel
		addChild(handle);
		layOut();
	}

	static float keyDiameter() {
		return 4.2f * 2.16f; // res-src/opz.py's KEY at the Sequencer's SCALE
	}

	/** Places what moves with the right edge. */
	void layOut() {
		laidOut = box.size.x;
		float w = px2mm(box.size.x);
		screen->box.size = mm2px(math::Vec(w - 2 * BEZEL_X - 2.f, BEZEL_H - 2.f));
		wheel->box.pos = mm2px(math::Vec(w - 14.f, ROW_Y)).minus(wheel->box.size.div(2));
		for (widget::Widget* s : rightScrews)
			s->box.pos.x = box.size.x - 2 * RACK_GRID_WIDTH;
	}

	static float px2mm(float px) { return px * MM_PER_IN / SVG_DPI; }

	void step() override {
		if (module)
			box.size.x = static_cast<PatternEditor*>(module)->width * RACK_GRID_WIDTH;
		if (box.size.x != laidOut)
			layOut();
		ModuleWidget::step();
	}

	void appendContextMenu(ui::Menu* menu) override {
		PatternEditor* m = static_cast<PatternEditor*>(module);
		menu->addChild(new ui::MenuSeparator);
		menu->addChild(createMenuLabel("Mouse gestures"));
		for (int g = 0; g < GESTURES; g++)
			menu->addChild(createIndexSubmenuItem(GESTURE_MENU[g].name, GESTURE_MENU[g].choices,
				[=] { return (size_t) m->gestures[g]; }, [=](size_t i) { m->gestures[g] = (int) i; }));
		menu->addChild(createIndexSubmenuItem("Tool", {"Pointer", "Pen", "Eraser"},
			[=] { return (size_t) m->tool; }, [=](size_t i) { m->tool = (int) i; }));
	}

	void draw(const DrawArgs& args) override {
		NVGcontext* vg = args.vg;
		bool dark = settings::preferDarkPanels;
		NVGcolor panel = dark ? nvgRGB(0x2b, 0x2c, 0x2e) : nvgRGB(0xec, 0xeb, 0xe8), ink = dark ? nvgRGB(0xc9, 0xc8, 0xc4) : nvgRGB(0x3a, 0x3a, 0x3c);
		NVGcolor title = dark ? nvgRGB(0xec, 0xeb, 0xe8) : nvgRGB(0x2b, 0x2b, 0x2d), footer = dark ? nvgRGB(0x70, 0x71, 0x75) : nvgRGB(0xa9, 0xa8, 0xa4);
		NVGcolor bezel = dark ? nvgRGB(0x14, 0x15, 0x16) : nvgRGB(0x2f, 0x30, 0x32), glass = nvgRGB(0x03, 0x03, 0x04);
		float w = px2mm(box.size.x);
		auto rect = [&](float x, float y, float rw, float rh, float r, NVGcolor c) {
			nvgBeginPath(vg);
			nvgRoundedRect(vg, mm2px(x), mm2px(y), mm2px(rw), mm2px(rh), mm2px(r));
			nvgFillColor(vg, c);
			nvgFill(vg);
		};
		rect(0.f, 0.f, w, HEIGHT, 0.f, panel);
		rect(BEZEL_X, BEZEL_Y, w - 2 * BEZEL_X, BEZEL_H, 3.f, bezel);
		rect(BEZEL_X + 1.f, BEZEL_Y + 1.f, w - 2 * BEZEL_X - 2.f, BEZEL_H - 2.f, 2.4f, glass);
		std::shared_ptr<window::Font> font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/Nunito-Bold.ttf"));
		if (font) {
			nvgFontFaceId(vg, font->handle);
			// As res-src/panelkit.py sets its outlines: `size` the em, the cap height centred on `y`.
			auto text = [&](float x, float y, const char* s, float size, NVGcolor c) {
				nvgFontSize(vg, mm2px(size));
				nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE);
				nvgFillColor(vg, c);
				nvgText(vg, mm2px(x), mm2px(y + CAP_HEIGHT * size / 2), s, NULL);
			};
			text(w / 2, 6.3f, "pattern editor", 5.2f, title);
			text(w / 2, HEIGHT - 2.54f, "un1tz3r0", 2.3f, footer);
			const std::pair<float, const char*> labels[] = {{VIEW_X, "VIEW"}, {FOLLOW_X, "FOLLOW"}, {LANE_X, "LANE"}, {(DOWN_X + UP_X) / 2, "OCTAVE"}, {w - 14.f, "PATTERN"}};
			for (auto [x, label] : labels)
				text(x, LABEL_Y, label, 2.4f, ink);
		}
		// The octave keys' rings, printed as the Sequencer's are.
		float d = keyDiameter(), ring = 0.3f * 2.16f;
		for (float x : {DOWN_X, UP_X}) {
			nvgBeginPath(vg);
			nvgCircle(vg, mm2px(x), mm2px(ROW_Y), mm2px((d - ring) / 2));
			nvgStrokeColor(vg, ink);
			nvgStrokeWidth(vg, mm2px(ring));
			nvgStroke(vg);
		}
		ModuleWidget::draw(args);
	}
};

Model* modelPatternEditor = createModel<PatternEditor, PatternEditorWidget>("PatternEditor");
