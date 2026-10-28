/*
Song Editor: the Sequencer's songs on a big screen, beside it on its left. It finds the hub by walking right, as
Remotes does, hands the Sequencer what its controls did each frame (Hub.hpp's SongIn), and its screen copies the
song back out on the UI thread (SongView).

MODE switches song and pattern mode; SONG picks the song, in song mode; ROW picks the row the column wheels edit, or
above the first, the song's settings, whose fade in, fade out and tempo sit over the PAT, LEN and MUTE columns. In
pattern mode the rows are the chain playing, and only their patterns are there to edit. INSERT puts a copy of the row in
after it, and DELETE takes it out. A click picks a row, and a double click plays it.
*/
#include "plugin.hpp"
#include "ui.hpp"
#include "opz/Hub.hpp"

struct SongEditor : Module {
	enum ParamId { MODE_PARAM, INSERT_PARAM, DELETE_PARAM, PARAMS_LEN };
	enum InputId { INPUTS_LEN };
	enum OutputId { OUTPUTS_LEN };
	enum LightId { MODE_LIGHT, LIGHTS_LEN };
	enum Wheel { SONG, ROW, PATTERN, LENGTH, MUTES, WHEELS };

	// The UI thread's turns and clicks, for the next frame to hand on.
	std::atomic<int> turns[WHEELS] = {};
	std::atomic<int> row{0};       // the row selected: -1 for the song's settings
	std::atomic<int> rows{1};      // how many the song has, as the screen saw it last
	std::atomic<int> jump{-1};     // a row double-clicked

	opz::Hub* hub = nullptr;
	uint32_t layoutSeen = ~0u;
	dsp::BooleanTrigger modePress, insertPress, deletePress;
	dsp::ClockDivider lightDivider;

	SongEditor() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configButton(MODE_PARAM, "Mode: songs (lit) or patterns");
		configButton(INSERT_PARAM, "Insert a copy of the row after it");
		configButton(DELETE_PARAM, "Delete the row");
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
		}
		if (lightDivider.process())
			lights[MODE_LIGHT].setBrightness(hub && hub->lastSongOut(args.frame).songMode ? 1.f : 0.f);
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


/** The song on the glass: the mode and transport by MODE, the song and tempo by SONG, its length and the playhead, the
settings row over the columns, then the rows. */
struct SongScreen : widget::OpaqueWidget {
	SongEditor* module = nullptr;
	float columns[3] = {};         // the PAT, LEN and MUTE wheels' x, in this widget's px
	float modeY = 0.f, songY = 0.f; // MODE's and SONG's lines' y
	opz::SongView view;
	bool linked = false;
	int top = 0;                   // the first row shown

	static constexpr float LINE = 5.f;  // mm between rows

	float rowsTop() const { return mm2px(51.f); }
	int visibleRows() const { return std::max(1, int((box.size.y - rowsTop() - mm2px(1.f)) / mm2px(LINE))); }

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
		if (p.y >= rowsTop() - mm2px(LINE) / 2 && p.y < rowsTop() + visibleRows() * mm2px(LINE) - mm2px(LINE) / 2) {
			int r = top + int((p.y - rowsTop() + mm2px(LINE) / 2) / mm2px(LINE));
			return r < view.chain.length ? r : -2;
		}
		return std::abs(p.y - mm2px(39.f)) < mm2px(LINE) / 2 ? -1 : -2;
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
		const NVGcolor ink = nvgRGB(0xdc, 0xdb, 0xd8), dim = nvgRGBA(0xdc, 0xdb, 0xd8, 0x60), lit = PAGE_COLORS[0];
		float w = box.size.x, pad = mm2px(3.f);
		nvgFontFaceId(vg, font->handle);
		auto text = [&](float x, float y, const std::string& s, NVGcolor c, int align = NVG_ALIGN_LEFT, float size = 3.2f) {
			nvgFontSize(vg, mm2px(size));
			nvgTextAlign(vg, align | NVG_ALIGN_MIDDLE);
			nvgFillColor(vg, c);
			nvgText(vg, x, y, s.c_str(), NULL);
		};
		if (!linked) {
			text(w / 2, box.size.y / 2, "PUT ME LEFT OF A SEQUENCER", dim, NVG_ALIGN_CENTER);
			return;
		}
		const opz::SongView& v = view;
		const opz::Project::Chain& c = v.chain;
		bool song = v.songMode;
		int selected = module ? module->row.load() : 0;

		text(pad, modeY, song ? "SONG MODE" : "PATTERN MODE", ink);
		text(w - pad, modeY, v.playing ? "PLAYING" : "STOPPED", v.playing ? lit : dim, NVG_ALIGN_RIGHT);
		text(pad, mm2px(13.f), song ? "LENGTH  " + bars(v.length) + "  " + clock(v.seconds) : "LENGTH  -", song ? ink : dim);
		text(pad, songY, song ? string::f("SONG %02d", v.song + 1) : "SONG -P", ink, NVG_ALIGN_LEFT, 5.f);
		std::string tempo = string::f("%.0f BPM", v.bpm);
		std::string source = v.external ? "EXT" : song && c.tempo > 0.f ? "SONG" : "PAT";
		text(w - pad, songY, source + "  " + tempo, ink, NVG_ALIGN_RIGHT);
		text(pad, mm2px(31.f), "TIME    " + bars(v.at) + "  " + clock(v.secondsAt), ink);

		// The settings row, over the columns whose wheels turn it.
		float settingsY = mm2px(39.f);
		if (selected == -1) {
			nvgBeginPath(vg);
			nvgRoundedRect(vg, pad / 2, settingsY - mm2px(LINE) / 2, w - pad, mm2px(LINE), mm2px(0.8f));
			nvgStrokeColor(vg, ink);
			nvgStrokeWidth(vg, 1.f);
			nvgStroke(vg);
		}
		NVGcolor setting = song ? ink : dim;
		text(pad, settingsY, "SONG", dim);
		text(columns[0], settingsY, song ? string::f("IN %g", c.fadeIn) : "IN -", setting, NVG_ALIGN_CENTER);
		text(columns[1], settingsY, song ? string::f("OUT %g", c.fadeOut) : "OUT -", setting, NVG_ALIGN_CENTER);
		text(columns[2], settingsY, song && c.tempo > 0.f ? string::f("%.0f", c.tempo) : song ? "PAT" : "-", setting, NVG_ALIGN_CENTER);

		float headY = mm2px(45.5f), barsX = w - pad;
		text(pad, headY, "ROW", dim, NVG_ALIGN_LEFT, 2.6f);
		const char* heads[3] = {"PAT", "LEN", "MUTE"};
		for (int k = 0; k < 3; k++)
			text(columns[k], headY, heads[k], DIAL_COLORS[k], NVG_ALIGN_CENTER, 2.6f);
		text(barsX, headY, "BARS", dim, NVG_ALIGN_RIGHT, 2.6f);

		int n = visibleRows();
		for (int i = 0; i < n && top + i < c.length; i++) {
			int r = top + i;
			float y = rowsTop() + i * mm2px(LINE), h = mm2px(LINE);
			if (r == v.rowPlaying) {
				nvgBeginPath(vg);
				nvgRect(vg, pad / 2, y - h / 2, w - pad, h);
				nvgFillColor(vg, nvgRGBA(0x3b, 0xe0, 0x7a, 0x30));
				nvgFill(vg);
				nvgBeginPath(vg);
				nvgRect(vg, pad / 2, y + h / 2 - mm2px(0.5f), (w - pad) * clamp(v.progress, 0.f, 1.f), mm2px(0.5f));
				nvgFillColor(vg, lit);
				nvgFill(vg);
			}
			if (r == selected) {
				nvgBeginPath(vg);
				nvgRoundedRect(vg, pad / 2, y - h / 2, w - pad, h, mm2px(0.8f));
				nvgStrokeColor(vg, ink);
				nvgStrokeWidth(vg, 1.f);
				nvgStroke(vg);
			}
			int length = c.lengths[r];
			std::string len = !song || !length ? "-" : length > 0 ? string::f("x%d", length) : string::f("%d/16", -length);
			std::string mute = song && c.mutes[r] ? string::f("G%d", c.mutes[r]) : "-";
			text(pad, y, string::f("%02d", r + 1), r == v.rowPlaying ? lit : dim);
			text(columns[0], y, string::f("P%02d", c.patterns[r] + 1), ink, NVG_ALIGN_CENTER);
			text(columns[1], y, len, song && length ? ink : dim, NVG_ALIGN_CENTER);
			text(columns[2], y, mute, song && c.mutes[r] ? ink : dim, NVG_ALIGN_CENTER);
			if (song)
				text(barsX, y, string::f("%g", std::round(v.rowLengths[r] / 16.0 * 100.0) / 100.0), dim, NVG_ALIGN_RIGHT);
		}
		if (c.length > n) {
			// Where the rows shown sit in the song.
			float track = n * mm2px(LINE), x = w - mm2px(1.2f), y0 = rowsTop() - mm2px(LINE) / 2;
			nvgBeginPath(vg);
			nvgRect(vg, x, y0 + track * top / c.length, mm2px(0.6f), track * n / c.length);
			nvgFillColor(vg, dim);
			nvgFill(vg);
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
		static const char* const columns[3] = {"PATTERN", "LENGTH", "MUTES"};
		for (int k = 0; k < 3; k++)
			screen->columns[k] = layout.center(std::string(columns[k]) + "_WHEEL_WIDGET").x - screen->box.pos.x;
		screen->modeY = layout.center("MODE_PARAM").y - screen->box.pos.y;
		screen->songY = layout.center("SONG_WHEEL_WIDGET").y - screen->box.pos.y;
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
		addParam(createParamCentered<VCVButton>(layout.center("INSERT_PARAM"), module, SongEditor::INSERT_PARAM));
		addParam(createParamCentered<VCVButton>(layout.center("DELETE_PARAM"), module, SongEditor::DELETE_PARAM));
	}
};

Model* modelSongEditor = createModel<SongEditor, SongEditorWidget>("SongEditor");
