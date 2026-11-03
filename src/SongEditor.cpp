/*
Song Editor: the Sequencer's songs on a big screen, beside it on its left. It finds the hub by walking right, as
Remotes does, hands the Sequencer what its controls did each frame (Hub.hpp's SongIn), and its screen copies the
song back out on the UI thread (SongView).

MODE switches song and pattern mode; SONG picks the song, in song mode; ROW picks the row the column wheels edit, or
above the first, the song's settings, whose fade in, fade out and tempo sit over the PAT, LEN and MUTE columns. In
pattern mode the rows are the chain playing, and only their patterns are there to edit. Round the ROW wheel, + puts a
copy of the row in after it, - takes it out, and the arrows move it up or down, the cursor going with it. A click picks
a row, and a double click plays it.
*/
#include "plugin.hpp"
#include "ui.hpp"
#include "opz/Hub.hpp"

struct SongEditor : Module {
	enum ParamId { MODE_PARAM, INSERT_PARAM, DELETE_PARAM, UP_PARAM, DOWN_PARAM, PARAMS_LEN };
	enum InputId { INPUTS_LEN };
	enum OutputId { OUTPUTS_LEN };
	enum LightId { MODE_LIGHT, ENUMS(KEY_LIGHT, 4 * 3), LIGHTS_LEN }; // the INSERT, DELETE, UP and DOWN keys' RGB
	enum Wheel { SONG, ROW, PATTERN, LENGTH, MUTES, WHEELS };

	// The UI thread's turns and clicks, for the next frame to hand on.
	std::atomic<int> turns[WHEELS] = {};
	std::atomic<int> row{0};       // the row selected: -1 for the song's settings
	std::atomic<int> rows{1};      // how many the song has, as the screen saw it last
	std::atomic<int> jump{-1};     // a row double-clicked

	opz::Hub* hub = nullptr;
	uint32_t layoutSeen = ~0u;
	dsp::BooleanTrigger modePress, insertPress, deletePress, upPress, downPress;
	dsp::ClockDivider lightDivider;

	SongEditor() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configButton(MODE_PARAM, "Mode: songs (lit) or patterns");
		configButton(INSERT_PARAM, "Insert a copy of the row after it");
		configButton(DELETE_PARAM, "Delete the row");
		configButton(UP_PARAM, "Move the row up");
		configButton(DOWN_PARAM, "Move the row down");
		lightDivider.setDivision(64);
	}

	/** A wheel turned `steps` on the UI thread. ROW moves the selection at once, the others wait for a frame. */
	void turn(int wheel, int steps) {
		if (wheel == ROW)
			row = clamp(row + steps, -1, std::max(rows.load() - 1, 0));
		else
			turns[wheel] += steps;
	}

	void onExpanderChange(const ExpanderChangeEvent& e) override {
		opz::busMoved();
	}

	void process(const ProcessArgs& args) override {
		uint32_t layout = opz::busLayout.load(std::memory_order_relaxed);
		if (layout != layoutSeen) {
			layoutSeen = layout;
			hub = nullptr;
			for (Module* m = rightExpander.module; opz::onBus(m) && !hub; m = m->rightExpander.module)
				hub = dynamic_cast<opz::Hub*>(m);
		}
		bool mode = modePress.process(params[MODE_PARAM].getValue() > 0.f);
		bool insert = insertPress.process(params[INSERT_PARAM].getValue() > 0.f);
		bool remove = deletePress.process(params[DELETE_PARAM].getValue() > 0.f);
		int move = int(downPress.process(params[DOWN_PARAM].getValue() > 0.f)) - int(upPress.process(params[UP_PARAM].getValue() > 0.f));
		int taken[WHEELS];
		for (int w = 0; w < WHEELS; w++)
			taken[w] = turns[w].exchange(0);
		int jumped = jump.exchange(-1);
		if (hub) {
			opz::SongIn& in = hub->songInFor(args.frame);
			in = opz::SongIn();
			in.frame = args.frame;
			in.row = row;
			in.modes = mode;
			in.songs = taken[SONG];
			in.patterns = taken[PATTERN];
			in.lengths = taken[LENGTH];
			in.mutes = taken[MUTES];
			in.inserts = insert;
			in.removes = remove;
			in.jump = jumped;
			int r = row;
			if (move && r >= 0 && r + move >= 0 && r + move < rows) {
				in.moves = move;
				in.moveRow = r;
				row = r + move;
			}
		}
		if (lightDivider.process()) {
			lights[MODE_LIGHT].setBrightness(hub && hub->lastSongOut(args.frame).songMode ? 1.f : 0.f);
			// The keys light white while held.
			for (int k = 0; k < 4; k++)
				for (int c = 0; c < 3; c++)
					lights[KEY_LIGHT + 3 * k + c].setBrightness(params[INSERT_PARAM + k].getValue() > 0.f ? 1.f : 0.f);
		}
	}

	json_t* dataToJson() override {
		json_t* root = json_object();
		json_object_set_new(root, "row", json_integer(row));
		return root;
	}

	void dataFromJson(json_t* root) override {
		row = std::max((int) json_integer_value(json_object_get(root, "row")), -1);
	}
};


/** An endless wheel: a drag up or a scroll turns it one step at a time, four with Ctrl. */
struct SongWheel : widget::OpaqueWidget {
	SongEditor* module = nullptr;
	int wheel = 0;
	NVGcolor cap = nvgRGB(0xe2, 0xe1, 0xdc);
	float dragged = 0.f;
	int turned = 0;                // for the dimple

	static constexpr float STEP = 8.f; // px of drag a step

	void turn(int steps) {
		if (APP->window->getMods() & RACK_MOD_CTRL)
			steps *= 4;
		turned += steps;
		if (module)
			module->turn(wheel, steps);
	}

	void onDragStart(const DragStartEvent& e) override {
		if (e.button != GLFW_MOUSE_BUTTON_LEFT)
			return;
		dragged = 0.f;
		APP->window->cursorLock();
	}

	void onDragMove(const DragMoveEvent& e) override {
		dragged -= e.mouseDelta.y;
		for (; dragged >= STEP; dragged -= STEP)
			turn(1);
		for (; dragged <= -STEP; dragged += STEP)
			turn(-1);
	}

	void onDragEnd(const DragEndEvent& e) override {
		APP->window->cursorUnlock();
	}

	void onHoverScroll(const HoverScrollEvent& e) override {
		if (e.scrollDelta.y != 0.f)
			turn(e.scrollDelta.y > 0.f ? 1 : -1);
		e.consume(this);
	}

	void draw(const DrawArgs& args) override {
		NVGcontext* vg = args.vg;
		math::Vec c = box.size.div(2.f);
		float r = box.size.x / 2.f;
		nvgBeginPath(vg);
		nvgCircle(vg, c.x, c.y, r);
		nvgFillColor(vg, nvgRGB(0x2b, 0x2b, 0x2d));
		nvgFill(vg);
		// Ridges round the edge, turning with it, so a turn shows.
		float angle = turned * float(M_PI) / 12.f;
		nvgStrokeColor(vg, nvgRGB(0x44, 0x45, 0x48));
		nvgStrokeWidth(vg, 0.8f);
		for (int i = 0; i < 24; i++) {
			float a = angle + i * float(M_PI) / 12.f;
			nvgBeginPath(vg);
			nvgMoveTo(vg, c.x + 0.8f * r * std::cos(a), c.y + 0.8f * r * std::sin(a));
			nvgLineTo(vg, c.x + 0.97f * r * std::cos(a), c.y + 0.97f * r * std::sin(a));
			nvgStroke(vg);
		}
		nvgBeginPath(vg);
		nvgCircle(vg, c.x, c.y, 0.72f * r);
		nvgFillColor(vg, cap);
		nvgFill(vg);
		nvgBeginPath(vg);
		nvgCircle(vg, c.x + 0.45f * r * std::sin(angle), c.y - 0.45f * r * std::cos(angle), 0.11f * r);
		nvgFillColor(vg, nvgRGBA(0, 0, 0, 0x70));
		nvgFill(vg);
	}
};


/** The song on the glass, in boxes: the mode, transport and tempo beside MODE; the song, its length and the playhead
beside SONG; the song's settings over the columns whose wheels turn them; then the rows, each column in its wheel's
colour, the row edited shown inverted. Every measure is in mm from the glass's top left. */
struct SongScreen : widget::OpaqueWidget {
	SongEditor* module = nullptr;
	opz::SongView view;
	bool linked = false;
	int top = 0;                   // the first row shown

	struct Column { float x, w; };
	static constexpr Column ROW_CELL = {1.5f, 13.f}, BARS_CELL = {83.f, 8.5f};
	static constexpr Column CELLS[3] = {{17.f, 20.f}, {39.f, 20.f}, {61.f, 20.f}}; // under the PAT, LEN and MUTE wheels
	static constexpr float STATUS_Y = 1.f, STATUS_H = 8.f;     // MODE, TRANSPORT, TEMPO
	static constexpr float SONG_Y = 10.f, SONG_H = 15.f;       // SONG, with LENGTH and TIME
	static constexpr float SETTINGS_Y = 26.5f, SETTINGS_H = 10.5f;
	static constexpr float HEAD_Y = 38.5f, HEAD_H = 5.5f;
	static constexpr float ROWS_Y = 45.f, ROW_H = 6.5f;
	static constexpr float LABEL = 2.6f, VALUE = 3.8f, ROW_TEXT = 4.2f; // text heights

	int visibleRows() const { return std::max(1, int((px2mm(box.size.y) - ROWS_Y) / ROW_H)); }

	static float px2mm(float px) { return px * MM_PER_IN / SVG_DPI; }

	void step() override {
		opz::Hub* hub = nullptr;
		for (Module* m = module ? module->rightExpander.module : nullptr; opz::onBus(m) && !hub; m = m->rightExpander.module)
			hub = dynamic_cast<opz::Hub*>(m);
		linked = hub && (hub->songView(view) || linked);
		if (module && linked) {
			module->rows = view.chain.length;
			int r = module->row = clamp(module->row.load(), -1, std::max(view.chain.length - 1, 0));
			int n = visibleRows();
			top = clamp(top, std::max(r - n + 1, 0), std::max(r, 0));
		}
		OpaqueWidget::step();
	}

	/** The row under a point, -1 for the settings row, or -2 for none. */
	int rowAt(math::Vec p) const {
		float y = px2mm(p.y);
		if (y >= SETTINGS_Y && y < SETTINGS_Y + SETTINGS_H)
			return -1;
		int i = int(std::floor((y - ROWS_Y) / ROW_H));
		return y >= ROWS_Y && i < visibleRows() && top + i < view.chain.length ? top + i : -2;
	}

	void onButton(const ButtonEvent& e) override {
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT && module && linked)
			if (int r = rowAt(e.pos); r >= -1)
				module->row = r;
		OpaqueWidget::onButton(e);
	}

	void onDoubleClick(const DoubleClickEvent& e) override {
		if (module && linked && module->row >= 0)
			module->jump = module->row.load();
	}

	static std::string bars(double sixteenths) {
		long s = (long) std::floor(sixteenths + 1e-6);
		return string::f("%03ld:%ld:%ld", s / 16 + 1, s % 16 / 4 + 1, s % 4 + 1);
	}

	static std::string clock(double seconds) {
		long s = (long) std::floor(seconds + 1e-6);
		return string::f("%ld:%02ld:%02ld", s / 3600, s / 60 % 60, s % 60);
	}

	void drawLayer(const DrawArgs& args, int layer) override {
		if (layer == 1)
			drawSong(args.vg);
		OpaqueWidget::drawLayer(args, layer);
	}

	void drawSong(NVGcontext* vg) {
		std::shared_ptr<window::Font> font = APP->window->loadFont(asset::system("res/fonts/ShareTechMono-Regular.ttf"));
		if (!font)
			return;
		const NVGcolor ink = nvgRGB(0xdc, 0xdb, 0xd8), dim = nvgRGBA(0xdc, 0xdb, 0xd8, 0x70), rule = nvgRGBA(0xdc, 0xdb, 0xd8, 0x38);
		const NVGcolor glass = nvgRGB(0x0e, 0x0e, 0x10), lit = PAGE_COLORS[0];
		const float W = px2mm(box.size.x);
		nvgFontFaceId(vg, font->handle);
		auto text = [&](float x, float y, const std::string& s, NVGcolor c, float size, int align = NVG_ALIGN_LEFT) {
			nvgFontSize(vg, mm2px(size));
			nvgTextAlign(vg, align | NVG_ALIGN_MIDDLE);
			nvgFillColor(vg, c);
			nvgText(vg, mm2px(x), mm2px(y), s.c_str(), NULL);
		};
		auto rect = [&](float x, float y, float w, float h, NVGcolor fill, NVGcolor stroke, float radius = 0.8f) {
			nvgBeginPath(vg);
			nvgRoundedRect(vg, mm2px(x), mm2px(y), mm2px(w), mm2px(h), mm2px(radius));
			if (fill.a > 0.f) {
				nvgFillColor(vg, fill);
				nvgFill(vg);
			}
			if (stroke.a > 0.f) {
				nvgStrokeColor(vg, stroke);
				nvgStrokeWidth(vg, 1.f);
				nvgStroke(vg);
			}
		};
		auto line = [&](float x0, float y0, float x1, float y1, NVGcolor c) {
			nvgBeginPath(vg);
			nvgMoveTo(vg, mm2px(x0), mm2px(y0));
			nvgLineTo(vg, mm2px(x1), mm2px(y1));
			nvgStrokeColor(vg, c);
			nvgStrokeWidth(vg, 1.f);
			nvgStroke(vg);
		};
		// A labelled box: its label at the top left, an optional note at the top right, its value in the middle.
		auto field = [&](float x, float y, float w, float h, const char* label, const std::string& value, NVGcolor c,
				float size = VALUE, const std::string& note = "") {
			rect(x, y, w, h, nvgRGBA(0xff, 0xff, 0xff, 0x08), rule);
			text(x + 1.2f, y + 1.9f, label, dim, LABEL);
			if (!note.empty())
				text(x + w - 1.2f, y + 1.9f, note, ink, LABEL, NVG_ALIGN_RIGHT);
			text(x + w / 2, y + h / 2 + 1.f, value, c, size, NVG_ALIGN_CENTER);
		};
		const NVGcolor none = nvgRGBA(0, 0, 0, 0);

		if (!linked) {
			text(W / 2, px2mm(box.size.y) / 2, "PUT ME LEFT OF A SEQUENCER", dim, VALUE, NVG_ALIGN_CENTER);
			return;
		}
		const opz::SongView& v = view;
		const opz::Project::Chain& c = v.chain;
		bool song = v.songMode;
		int selected = module ? module->row.load() : 0;
		const float right = W - 1.5f, third = (right - 1.5f - 4.f) / 3.f;

		// Status, beside MODE.
		const char* source = v.external ? "EXT" : song && c.tempo > 0.f ? "SONG" : "PAT";
		field(1.5f, STATUS_Y, third, STATUS_H, "MODE", song ? "SONG" : "PATTERN", ink);
		field(3.5f + third, STATUS_Y, third, STATUS_H, "TRANSPORT", v.playing ? "PLAYING" : "STOPPED", v.playing ? lit : dim);
		field(5.5f + 2 * third, STATUS_Y, third, STATUS_H, "TEMPO", string::f("%.0f BPM", v.bpm), ink, VALUE, source);

		// The song, beside SONG, with its length and the playhead.
		float songW = ROW_CELL.w + CELLS[0].w + 2.5f, wide = right - 1.5f - songW - 2.f, half = (SONG_H - 1.f) / 2;
		field(1.5f, SONG_Y, songW, SONG_H, "SONG", song ? string::f("%02d", v.song + 1) : "-P", ink, 7.f);
		float x = 3.5f + songW;
		field(x, SONG_Y, wide, half, "LENGTH", song ? bars(v.length) + "  " + clock(v.seconds) : "-", song ? ink : dim);
		field(x, SONG_Y + half + 1.f, wide, half, "TIME", bars(v.at) + "  " + clock(v.secondsAt), ink);

		// The settings, over the columns whose wheels turn them.
		bool setting = selected == -1;
		rect(ROW_CELL.x, SETTINGS_Y, ROW_CELL.w, SETTINGS_H, setting ? ink : nvgRGBA(0xff, 0xff, 0xff, 0x08), rule);
		text(ROW_CELL.x + ROW_CELL.w / 2, SETTINGS_Y + SETTINGS_H / 2, "SONG", setting ? glass : dim, LABEL + 0.4f, NVG_ALIGN_CENTER);
		const char* settingLabels[3] = {"FADE IN", "FADE OUT", "TEMPO"};
		auto fade = [](float bars) { return bars > 0.f ? string::f("%g BAR", bars) : std::string("OFF"); };
		std::string settings[3] = {fade(c.fadeIn), fade(c.fadeOut), c.tempo > 0.f ? string::f("%.0f", c.tempo) : "PAT"};
		for (int k = 0; k < 3; k++) {
			const Column& col = CELLS[k];
			NVGcolor tint = DIAL_COLORS[k];
			rect(col.x, SETTINGS_Y, col.w, SETTINGS_H, setting ? tint : nvgRGBA(0xff, 0xff, 0xff, 0x08), setting ? none : nvgTransRGBA(tint, 0x80));
			text(col.x + col.w / 2, SETTINGS_Y + 2.2f, settingLabels[k], setting ? glass : nvgTransRGBA(tint, 0xb0), LABEL, NVG_ALIGN_CENTER);
			text(col.x + col.w / 2, SETTINGS_Y + 6.6f, song ? settings[k] : "-", setting ? glass : song ? tint : dim, VALUE, NVG_ALIGN_CENTER);
		}

		// The table: headings, then the rows, ruled between.
		int n = visibleRows(), shown = std::min(n, c.length - top);
		float bottom = ROWS_Y + n * ROW_H;
		auto heading = [&](const Column& col, const char* name, NVGcolor tint) {
			rect(col.x, HEAD_Y, col.w, HEAD_H, nvgTransRGBA(tint, 0x30), nvgTransRGBA(tint, 0x90));
			text(col.x + col.w / 2, HEAD_Y + HEAD_H / 2, name, tint, LABEL + 0.4f, NVG_ALIGN_CENTER);
		};
		heading(ROW_CELL, "ROW", ink);
		const char* heads[3] = {"PAT", "LEN", "MUTE"};
		for (int k = 0; k < 3; k++)
			heading(CELLS[k], heads[k], DIAL_COLORS[k]);
		heading(BARS_CELL, "BARS", ink);
		rect(ROW_CELL.x, ROWS_Y - 0.5f, right - ROW_CELL.x, bottom - ROWS_Y + 0.5f, none, rule);
		for (float dx : {CELLS[0].x - 1.f, CELLS[1].x - 1.f, CELLS[2].x - 1.f, BARS_CELL.x - 1.f})
			line(dx, ROWS_Y - 0.5f, dx, bottom, rule);
		for (int i = 1; i < n; i++)
			line(ROW_CELL.x, ROWS_Y + i * ROW_H, right, ROWS_Y + i * ROW_H, nvgRGBA(0xdc, 0xdb, 0xd8, 0x1c));

		for (int i = 0; i < shown; i++) {
			int r = top + i;
			float y = ROWS_Y + i * ROW_H, mid = y + ROW_H / 2;
			bool cursor = r == selected, playing = r == v.rowPlaying;
			if (playing)
				rect(ROW_CELL.x, y, right - ROW_CELL.x, ROW_H, nvgTransRGBA(lit, 0x28), none, 0.f);
			int length = c.lengths[r];
			std::string cells[3] = {
				string::f("P%02d", c.patterns[r] + 1),
				!song || !length ? "-" : length > 0 ? string::f("x%d", length) : string::f("%d/16", -length),
				song && c.mutes[r] ? string::f("G%d", c.mutes[r]) : "-",
			};
			bool set[3] = {true, song && length, song && c.mutes[r]};
			auto cell = [&](const Column& col, const std::string& s, NVGcolor tint, NVGcolor plain, float size, int align) {
				if (cursor)
					rect(col.x, y + 0.4f, col.w, ROW_H - 0.8f, tint, none, 0.6f);
				float tx = align == NVG_ALIGN_CENTER ? col.x + col.w / 2 : col.x + col.w - 0.8f;
				text(tx, mid, s, cursor ? glass : plain, size, align);
			};
			cell(ROW_CELL, string::f("%02d", r + 1), ink, playing ? lit : dim, ROW_TEXT, NVG_ALIGN_CENTER);
			for (int k = 0; k < 3; k++)
				cell(CELLS[k], cells[k], DIAL_COLORS[k], set[k] ? DIAL_COLORS[k] : dim, ROW_TEXT, NVG_ALIGN_CENTER);
			double b = v.rowLengths[r] / 16.0;
			cell(BARS_CELL, song ? (b == std::floor(b) ? string::f("%.0f", b) : string::f("%.1f", b)) : "", ink, dim, 3.4f, NVG_ALIGN_RIGHT);
			if (playing)
				rect(ROW_CELL.x, y + ROW_H - 0.7f, (right - ROW_CELL.x) * clamp(v.progress, 0.f, 1.f), 0.7f, lit, none, 0.f);
		}
		if (c.length > n) {
			// Where the rows shown sit in the song.
			float track = bottom - ROWS_Y;
			rect(right + 0.4f, ROWS_Y + track * top / c.length, 0.6f, track * n / c.length, dim, none, 0.3f);
		}
	}
};

struct SongEditorWidget : ModuleWidget {
	SongEditorWidget(SongEditor* module) {
		setModule(module);
		std::string panel = asset::plugin(pluginInstance, "res/SongEditor.svg");
		setPanel(createPanel(panel, asset::plugin(pluginInstance, "res/SongEditor-dark.svg")));
		addScrews(this);
		PanelLayout layout(panel);

		SongScreen* screen = createWidget<SongScreen>(math::Vec());
		screen->box = layout.box("SCREEN_WIDGET");
		screen->module = module;
		addChild(screen);

		static const char* const wheels[SongEditor::WHEELS] = {"SONG", "ROW", "PATTERN", "LENGTH", "MUTES"};
		for (int w = 0; w < SongEditor::WHEELS; w++) {
			SongWheel* wheel = createWidget<SongWheel>(math::Vec());
			wheel->box.size = mm2px(math::Vec(9.f, 9.f));
			wheel->box.pos = layout.center(std::string(wheels[w]) + "_WHEEL_WIDGET").minus(wheel->box.size.div(2.f));
			wheel->module = module;
			wheel->wheel = w;
			if (w >= SongEditor::PATTERN)
				wheel->cap = DIAL_COLORS[w - SongEditor::PATTERN];
			addChild(wheel);
		}
		addParam(createLightParamCentered<VCVLightBezel<GreenLight>>(layout.center("MODE_PARAM"), module, SongEditor::MODE_PARAM, SongEditor::MODE_LIGHT));
		static const char* const keys[4][2] = {{"INSERT", "plus"}, {"DELETE", "minus"}, {"UP", "up"}, {"DOWN", "down"}};
		for (int k = 0; k < 4; k++) {
			RoundKey* key = createLightParam<RoundKey>(math::Vec(), module, SongEditor::INSERT_PARAM + k, SongEditor::KEY_LIGHT + 3 * k);
			key->setRing(layout.box(std::string(keys[k][0]) + "_PARAM"), keys[k][1]);
			addParam(key);
		}
	}
};

Model* modelSongEditor = createModel<SongEditor, SongEditorWidget>("SongEditor");
