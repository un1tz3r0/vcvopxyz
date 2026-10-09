/*
Screen: the OP-Z's phone app as an expander. Placed to the right of an OP-Z engine, it shows a picture of the page the
dials are on (the filter over a scope, the envelope with a dot per voice, the LFO, the mix, the track settings and
quick slots), the value of the dial last touched, and every dial of every page. Click a dial on it to select it and
its page, and drag anywhere on it to change the selected dial, like the app's touch pad.
*/
#include "SynthTrack.hpp"

namespace opz {

/** The module does nothing itself: its widget reads the track to its left. */
struct Screen : Module {
	Screen() {
		config(0, 0, 0, 0);
	}

	Track* track() {
		return dynamic_cast<Track*>(leftExpander.module);
	}
};


static const NVGcolor INK = nvgRGB(0xe8, 0xe7, 0xe3), DIM = nvgRGB(0x8c, 0x8b, 0x88), FAINT = nvgRGB(0x3a, 0x3b, 0x3f),
	WELL = nvgRGB(0x0d, 0x0e, 0x10);

/** Draws in millimetres, laid out like the mockup: a status bar, the selected dial's value, the page's picture, and
an overview of every dial with a row per page. */
struct Display : widget::OpaqueWidget {
	static constexpr float PAD = 2.6f, VISUAL_TOP = 19.f, VISUAL_HEIGHT = 38.f, OVERVIEW_TOP = 60.5f;

	Screen* module = nullptr;
	const Track::Snapshot* snapshot = nullptr;
	int focus = Track::P1_PARAM;          // the dial the readout shows and a drag changes
	float last[Track::PAGE_PARAM] = {};   // dial values last frame, to see which one was touched
	int lastPage = -1;
	Track* lastTrack = nullptr;
	float dragValue = 0.f, dragStart = 0.f;
	bool dragging = false;
	// Dummy snapshot for before the first one arrives.
	Track::Snapshot empty = {};

	math::Vec size() const {
		return box.size.div(mm2px(1.f));
	}

	math::Rect visual() const {
		return math::Rect(PAD, VISUAL_TOP, size().x - 2 * PAD, VISUAL_HEIGHT);
	}

	float rowHeight() const {
		return (size().y - OVERVIEW_TOP - 2.f) / PAGES_LEN;
	}

	float cellWidth() const {
		return (size().x - 2 * PAD) / 4;
	}

	/** Which page and dial a parameter belongs to. */
	static bool locate(int param, int* page, int* dial) {
		for (int p = 0; p < PAGES_LEN; p++)
			for (int d = 0; d < 4; d++)
				if (Track::DIALS[p][d] == param) {
					*page = p;
					*dial = d;
					return true;
				}
		return false;
	}

	static int firstDial(int page) {
		for (int d = 0; d < 4; d++)
			if (Track::DIALS[page][d] >= 0)
				return Track::DIALS[page][d];
		return -1;
	}

	void step() override {
		Track* track = module ? module->track() : nullptr;
		if (track) {
			if (track->snapshots.update() || !snapshot)
				snapshot = &track->snapshots.read();
			// The readout follows whichever dial moves, and the page when it changes.
			for (int i = 0; i < Track::PAGE_PARAM; i++) {
				float v = track->params[i].getValue();
				if (v != last[i] && !dragging && track == lastTrack)
					focus = i;
				last[i] = v;
			}
			lastTrack = track;
			int shown = track->shownPage(), page, dial;
			if (shown != lastPage && !(locate(focus, &page, &dial) && page == shown))
				focus = firstDial(shown);
			lastPage = shown;
		}
		else {
			snapshot = nullptr;
			lastTrack = nullptr;
		}
		OpaqueWidget::step();
	}

	// Drawing

	static void text(NVGcontext* vg, float x, float y, float size, NVGcolor color, int align, const std::string& s) {
		nvgFontSize(vg, size);
		nvgFillColor(vg, color);
		nvgTextAlign(vg, align | NVG_ALIGN_MIDDLE);
		nvgText(vg, x, y, s.c_str(), NULL);
	}

	static void stroke(NVGcontext* vg, NVGcolor color, float width) {
		nvgStrokeColor(vg, color);
		nvgStrokeWidth(vg, width);
		nvgLineCap(vg, NVG_ROUND);
		nvgLineJoin(vg, NVG_ROUND);
		nvgStroke(vg);
	}

	void drawLayer(const DrawArgs& args, int layer) override {
		if (layer == 1) {
			NVGcontext* vg = args.vg;
			std::shared_ptr<window::Font> font = APP->window->loadFont(asset::system("res/fonts/Nunito-Bold.ttf"));
			if (font) {
				nvgSave(vg);
				nvgScissor(vg, 0, 0, box.size.x, box.size.y);
				nvgScale(vg, mm2px(1.f), mm2px(1.f));
				nvgFontFaceId(vg, font->handle);
				Track* track = module ? module->track() : nullptr;
				if (track)
					drawTrack(vg, track, snapshot ? *snapshot : empty);
				else
					drawHint(vg);
				nvgRestore(vg);
			}
		}
		OpaqueWidget::drawLayer(args, layer);
	}

	/** With no engine beside it, the screen says where it goes. */
	void drawHint(NVGcontext* vg) {
		math::Vec s = size();
		float cx = s.x / 2, cy = s.y / 2 - 6.f;
		nvgBeginPath(vg);
		nvgRoundedRect(vg, cx - 13.f, cy - 9.f, 14.f, 18.f, 1.f);
		stroke(vg, DIM, 0.35f);
		nvgBeginPath(vg);
		nvgRoundedRect(vg, cx + 2.f, cy - 9.f, 11.f, 18.f, 1.f);
		nvgFillColor(vg, FAINT);
		nvgFill(vg);
		for (int d = 0; d < 4; d++) {
			nvgBeginPath(vg);
			nvgCircle(vg, cx - 10.6f + 3.2f * d, cy - 5.f, 1.2f);
			nvgFillColor(vg, DIAL_COLORS[d]);
			nvgFill(vg);
		}
		text(vg, cx, cy + 15.f, 2.4f, INK, NVG_ALIGN_CENTER, "place me to the right");
		text(vg, cx, cy + 18.5f, 2.4f, INK, NVG_ALIGN_CENTER, "of an OP-Z engine");
	}

	void drawTrack(NVGcontext* vg, Track* track, const Track::Snapshot& snap) {
		math::Vec s = size();
		int shown = track->shownPage();

		// Status bar
		text(vg, PAD, 4.2f, 2.8f, INK, NVG_ALIGN_LEFT, string::lowercase(track->model->name));
		text(vg, s.x / 2, 4.2f, 2.3f, PAGE_COLORS[shown], NVG_ALIGN_CENTER, Track::PAGE_NAMES[shown]);
		text(vg, s.x - PAD, 4.2f, 2.1f, DIM, NVG_ALIGN_RIGHT, string::f("%.0f BPM", 60.f / track->secondsPerBeat));
		nvgBeginPath(vg);
		nvgMoveTo(vg, PAD, 7.4f);
		nvgLineTo(vg, s.x - PAD, 7.4f);
		stroke(vg, FAINT, 0.25f);

		// The selected dial's name in its colour, and its value
		int page, dial;
		if (focus >= 0 && locate(focus, &page, &dial)) {
			ParamQuantity* q = track->paramQuantities[focus];
			text(vg, PAD, 10.6f, 2.0f, DIAL_COLORS[dial], NVG_ALIGN_LEFT, track->dialName(focus));
			text(vg, PAD, 15.0f, 3.8f, INK, NVG_ALIGN_LEFT, q->getDisplayValueString() + q->getUnit());
		}

		math::Rect v = visual();
		nvgBeginPath(vg);
		nvgRoundedRect(vg, RECT_ARGS(v), 1.2f);
		nvgFillColor(vg, WELL);
		nvgFill(vg);
		nvgSave(vg);
		nvgIntersectScissor(vg, RECT_ARGS(v));
		switch (shown) {
			case PAGE_SOUND: drawSound(vg, track, snap, v); break;
			case PAGE_ENVELOPE: drawEnvelope(vg, track, snap, v); break;
			case PAGE_LFO: drawLfo(vg, track, snap, v); break;
			case PAGE_MIX: drawMix(vg, track, snap, v); break;
			default: drawTrackSettings(vg, track, v); break;
		}
		nvgRestore(vg);

		drawOverview(vg, track, shown);
	}

	/** A scope of the output, and the filter's response over it. */
	void drawSound(NVGcontext* vg, Track* track, const Track::Snapshot& snap, math::Rect v) {
		// Start the trace on a rising zero crossing, so a steady note stands still.
		const int N = Track::Snapshot::SCOPE / 2;
		int start = 1;
		while (start < N && !(snap.scope[start - 1] < 0.f && snap.scope[start] >= 0.f))
			start++;
		if (start == N)
			start = 0;
		float mid = v.pos.y + v.size.y * 0.42f;
		nvgBeginPath(vg);
		for (int i = 0; i < N; i++) {
			float x = v.pos.x + v.size.x * i / (N - 1);
			float y = mid - clamp(snap.scope[start + i] / 6.f, -1.f, 1.f) * v.size.y * 0.3f;
			if (i == 0)
				nvgMoveTo(vg, x, y);
			else
				nvgLineTo(vg, x, y);
		}
		stroke(vg, nvgTransRGBAf(INK, 0.55f), 0.3f);

		// The two filters in series, each a 12 dB state-variable filter; only the half the dial is in resonates.
		float filter = track->params[Track::FILTER_PARAM].getValue();
		float q = 1.f / (2.f - 1.9f * track->params[Track::RESONANCE_PARAM].getValue());
		float lp = lowpassHz(filter), hp = highpassHz(filter);
		float lpQ = filter < 0.5f ? q : float(M_SQRT1_2), hpQ = filter > 0.5f ? q : float(M_SQRT1_2);
		auto y = [&](float t) {
			float f = 20.f * std::pow(1000.f, t);
			float a = f / lp, b = f / hp;
			float mag = 1.f / std::sqrt(std::pow(1.f - a * a, 2.f) + std::pow(a / lpQ, 2.f))
				* b * b / std::sqrt(std::pow(1.f - b * b, 2.f) + std::pow(b / hpQ, 2.f));
			float db = clamp(20.f * std::log10(std::fmax(mag, 1e-6f)), -36.f, 24.f);
			return v.pos.y + v.size.y * 0.62f - db / 36.f * v.size.y * 0.32f;
		};
		const int STEPS = 96;
		nvgBeginPath(vg);
		nvgMoveTo(vg, v.pos.x, v.pos.y + v.size.y);
		for (int i = 0; i <= STEPS; i++)
			nvgLineTo(vg, v.pos.x + v.size.x * i / STEPS, y(float(i) / STEPS));
		nvgLineTo(vg, v.pos.x + v.size.x, v.pos.y + v.size.y);
		nvgClosePath(vg);
		nvgFillColor(vg, nvgTransRGBAf(DIAL_COLORS[2], 0.14f));
		nvgFill(vg);
		nvgBeginPath(vg);
		for (int i = 0; i <= STEPS; i++) {
			float x = v.pos.x + v.size.x * i / STEPS;
			if (i == 0)
				nvgMoveTo(vg, x, y(0.f));
			else
				nvgLineTo(vg, x, y(float(i) / STEPS));
		}
		stroke(vg, DIAL_COLORS[2], 0.5f);
		text(vg, v.pos.x + 1.5f, v.pos.y + v.size.y - 1.6f, 1.6f, DIM, NVG_ALIGN_LEFT, "20 Hz");
		text(vg, v.getRight() - 1.5f, v.pos.y + v.size.y - 1.6f, 1.6f, DIM, NVG_ALIGN_RIGHT, "20 kHz");
	}

	/** The envelope, each stage in its dial's colour, with a dot riding it for each sounding voice. */
	void drawEnvelope(NVGcontext* vg, Track* track, const Track::Snapshot& snap, math::Rect v) {
		float a = 0.1f + track->params[Track::ATTACK_PARAM].getValue();
		float d = 0.1f + track->params[Track::DECAY_PARAM].getValue();
		float sustain = track->params[Track::SUSTAIN_PARAM].getValue();
		float r = 0.1f + track->params[Track::RELEASE_PARAM].getValue();
		float hold = 0.5f;
		float scale = (v.size.x - 4.f) / (a + d + hold + r);
		float x0 = v.pos.x + 2.f, top = v.pos.y + 4.f, bottom = v.getBottom() - 3.f;
		auto Y = [&](float level) { return bottom - (bottom - top) * level; };
		float xa = x0 + a * scale, xd = xa + d * scale, xs = xd + hold * scale, xr = xs + r * scale;
		const int STEPS = 24;
		// Attack curves up like the analog-style one it is; decay and release fall exponentially.
		auto segment = [&](float from, float to, std::function<float(float)> level, NVGcolor color) {
			nvgBeginPath(vg);
			for (int i = 0; i <= STEPS; i++) {
				float t = float(i) / STEPS;
				float x = from + (to - from) * t, y = Y(level(t));
				if (i == 0)
					nvgMoveTo(vg, x, y);
				else
					nvgLineTo(vg, x, y);
			}
			stroke(vg, color, 0.55f);
		};
		segment(x0, xa, [](float t) { return (1.f - std::exp(-1.8f * t)) / (1.f - std::exp(-1.8f)); }, DIAL_COLORS[0]);
		segment(xa, xd, [&](float t) { return sustain + (1.f - sustain) * std::exp(-4.6f * t); }, DIAL_COLORS[1]);
		segment(xd, xs, [&](float t) { return sustain; }, DIAL_COLORS[2]);
		segment(xs, xr, [&](float t) { return sustain * std::exp(-4.6f * t); }, DIAL_COLORS[3]);

		for (int i = 0; i < PORT_MAX_CHANNELS; i++) {
			float level = snap.levels[i], x;
			switch (snap.stages[i]) {
				case xyz::Adsr::ATTACK: x = x0 + (xa - x0) * level; break;
				case xyz::Adsr::DECAY:
					x = level - sustain > 0.01f ? xa + (xd - xa) * std::log((1.f - sustain) / (level - sustain)) / 4.6f : (xd + xs) / 2;
					break;
				case xyz::Adsr::RELEASE:
					x = xs + (xr - xs) * clamp(std::log(std::fmax(sustain, 0.01f) / std::fmax(level, 1e-4f)) / 4.6f, 0.f, 1.f);
					break;
				default: continue;
			}
			nvgBeginPath(vg);
			nvgCircle(vg, clamp(x, x0, xr), Y(level), 0.8f);
			nvgFillColor(vg, INK);
			nvgFill(vg);
		}
	}

	/** One cycle of the LFO's shape, as deep as its amount, with a dot at its phase. */
	void drawLfo(NVGcontext* vg, Track* track, const Track::Snapshot& snap, math::Rect v) {
		int shape = (int) track->params[Track::LFO_SHAPE_PARAM].getValue();
		float amount = track->params[Track::LFO_AMOUNT_PARAM].getValue();
		float depth = std::copysign(std::fmax(std::fabs(amount), 0.1f), amount);
		bool unipolar = Lfo::triggered(shape);
		float mid = unipolar ? v.getBottom() - 4.f : v.getCenter().y;
		float height = unipolar ? v.size.y - 9.f : v.size.y / 2 - 5.f;
		nvgBeginPath(vg);
		nvgMoveTo(vg, v.pos.x + 2.f, mid);
		nvgLineTo(vg, v.getRight() - 2.f, mid);
		stroke(vg, FAINT, 0.25f);

		// The random shapes get a made-up pattern of steps, the external one its current value.
		static const float STEPS[] = {0.7f, 0.2f, 0.9f, 0.45f, 0.05f, 0.6f, 0.3f, 0.8f};
		auto value = [&](float phase) {
			return Lfo::value(shape, phase, STEPS[std::min(int(phase * 8.f), 7)], false, snap.lfoValue);
		};
		float x0 = v.pos.x + 2.f, w = v.size.x - 4.f;
		const int N = 160;
		nvgBeginPath(vg);
		for (int i = 0; i <= N; i++) {
			float p = float(i) / N;
			float x = x0 + w * p, y = mid - value(std::fmin(p, 0.9999f)) * depth * height;
			if (i == 0)
				nvgMoveTo(vg, x, y);
			else
				nvgLineTo(vg, x, y);
		}
		stroke(vg, DIAL_COLORS[3], 0.55f);
		float px = x0 + w * snap.lfoPhase;
		nvgBeginPath(vg);
		nvgMoveTo(vg, px, v.pos.y + 2.f);
		nvgLineTo(vg, px, v.getBottom() - 2.f);
		stroke(vg, nvgTransRGBAf(INK, 0.3f), 0.2f);
		nvgBeginPath(vg);
		nvgCircle(vg, px, mid - snap.lfoValue * depth * height, 1.1f);
		nvgFillColor(vg, INK);
		nvgFill(vg);

		ParamQuantity* speed = track->paramQuantities[Track::LFO_SPEED_PARAM];
		ParamQuantity* target = track->paramQuantities[Track::LFO_TARGET_PARAM];
		text(vg, v.getRight() - 1.8f, v.pos.y + 3.f, 1.7f, DIM, NVG_ALIGN_RIGHT,
			string::uppercase(speed->getDisplayValueString() + " \xc2\xb7 " + target->getDisplayValueString()));
	}

	/** Where the track sits in the stereo field, how loud it is, and what it sends to the effects. */
	void drawMix(NVGcontext* vg, Track* track, const Track::Snapshot& snap, math::Rect v) {
		float left = v.pos.x + 4.f, right = v.getRight() - 16.f;
		auto bar = [&](float y, float from, float to, NVGcolor color, const char* label) {
			nvgBeginPath(vg);
			nvgRoundedRect(vg, left, y - 0.6f, right - left, 1.2f, 0.6f);
			nvgFillColor(vg, FAINT);
			nvgFill(vg);
			nvgBeginPath(vg);
			nvgRoundedRect(vg, left + (right - left) * std::fmin(from, to), y - 0.6f, (right - left) * std::fabs(to - from), 1.2f, 0.6f);
			nvgFillColor(vg, color);
			nvgFill(vg);
			text(vg, left, y - 2.4f, 1.6f, DIM, NVG_ALIGN_LEFT, label);
		};
		float fx1 = track->params[Track::FX1_PARAM].getValue(), fx2 = track->params[Track::FX2_PARAM].getValue();
		bar(v.pos.y + 9.f, 0.f, fx1, DIAL_COLORS[0], "FX 1");
		bar(v.pos.y + 18.f, 0.f, fx2, DIAL_COLORS[1], "FX 2");
		// Pan as a dot on a line from L to R.
		float pan = track->params[Track::PAN_PARAM].getValue();
		float py = v.pos.y + 29.f, px = left + (right - left) * (pan + 1.f) / 2;
		nvgBeginPath(vg);
		nvgMoveTo(vg, left, py);
		nvgLineTo(vg, right, py);
		stroke(vg, FAINT, 0.3f);
		nvgBeginPath(vg);
		nvgCircle(vg, px, py, 1.3f);
		nvgFillColor(vg, DIAL_COLORS[2]);
		nvgFill(vg);
		text(vg, left, py - 2.6f, 1.6f, DIM, NVG_ALIGN_LEFT, "PAN");
		text(vg, left - 1.6f, py, 1.5f, DIM, NVG_ALIGN_RIGHT, "L");
		text(vg, right + 1.6f, py, 1.5f, DIM, NVG_ALIGN_LEFT, "R");

		// Output meters, with the level dial's setting marked beside them.
		float mx = v.getRight() - 9.f, top = v.pos.y + 4.f, bottom = v.getBottom() - 4.f;
		for (int i = 0; i < 2; i++) {
			float x = mx + 3.f * i;
			nvgBeginPath(vg);
			nvgRect(vg, x, top, 1.6f, bottom - top);
			nvgFillColor(vg, FAINT);
			nvgFill(vg);
			float level = clamp(snap.outputs[i] / 2.f, 0.f, 1.f); // 1 is the soft limiter's 5V; the meter goes to 10V
			nvgBeginPath(vg);
			nvgRect(vg, x, bottom - (bottom - top) * level, 1.6f, (bottom - top) * level);
			nvgFillColor(vg, level > 0.5f ? DIAL_COLORS[3] : INK);
			nvgFill(vg);
		}
		float level = track->params[Track::LEVEL_PARAM].getValue();
		float ly = bottom - (bottom - top) * level * level / 2;
		nvgBeginPath(vg);
		nvgMoveTo(vg, mx - 1.6f, ly);
		nvgLineTo(vg, mx + 6.f, ly);
		stroke(vg, DIAL_COLORS[3], 0.4f);
	}

	/** The note style, the portamento as a slide between two notes, and the quick slots as the white keys show them. */
	void drawTrackSettings(NVGcontext* vg, Track* track, math::Rect v) {
		int style = (int) track->params[Track::STYLE_PARAM].getValue();
		static const char* const styles[] = {"POLY", "MONO", "LEGATO"};
		float cw = (v.size.x - 4.f) / 3;
		for (int i = 0; i < 3; i++) {
			float x = v.pos.x + 2.f + cw * (i + 0.5f);
			text(vg, x, v.pos.y + 4.5f, 2.2f, i == style ? INK : DIM, NVG_ALIGN_CENTER, styles[i]);
			if (i == style) {
				nvgBeginPath(vg);
				nvgRoundedRect(vg, x - 4.f, v.pos.y + 6.6f, 8.f, 0.6f, 0.3f);
				nvgFillColor(vg, DIAL_COLORS[1]);
				nvgFill(vg);
			}
		}

		// A step up a fifth, smoothed by the portamento.
		float glide = track->params[Track::GLIDE_PARAM].getValue();
		float x0 = v.pos.x + 4.f, x1 = v.getRight() - 4.f, y0 = v.pos.y + 19.f, y1 = v.pos.y + 11.f;
		nvgBeginPath(vg);
		const int N = 60;
		for (int i = 0; i <= N; i++) {
			float t = float(i) / N;
			float step = t < 0.25f ? 0.f : glide <= 0.f ? 1.f : 1.f - std::exp(-(t - 0.25f) / (0.02f + 0.5f * glide * glide));
			float x = x0 + (x1 - x0) * t, y = y0 + (y1 - y0) * step;
			if (i == 0)
				nvgMoveTo(vg, x, y);
			else
				nvgLineTo(vg, x, y);
		}
		stroke(vg, DIAL_COLORS[3], 0.5f);

		// The slots, two rows of seven like the white keys: filled ones glow faintly, the last one used brightly.
		int filled = track->slotsFilled, current = track->slot;
		float sw = (v.size.x - 6.f) / 7, sy = v.pos.y + 24.f;
		for (int s = 0; s < SLOTS; s++) {
			float x = v.pos.x + 3.f + sw * (s % 7), y = sy + 6.2f * (s / 7);
			nvgBeginPath(vg);
			nvgRoundedRect(vg, x + 0.4f, y, sw - 0.8f, 5.0f, 0.6f);
			if (s == current)
				nvgFillColor(vg, NOTE_COLOR);
			else if ((filled >> s) & 1)
				nvgFillColor(vg, nvgTransRGBAf(INK, 0.35f));
			else
				nvgFillColor(vg, FAINT);
			nvgFill(vg);
		}
		text(vg, v.pos.x + 3.f, sy - 1.6f, 1.5f, DIM, NVG_ALIGN_LEFT, "QUICK SLOTS");
	}

	/** Every dial as an arc in its colour, a row per page, with the page the dials are on lit up. */
	void drawOverview(NVGcontext* vg, Track* track, int shown) {
		float rh = rowHeight(), cw = cellWidth();
		for (int p = 0; p < PAGES_LEN; p++) {
			float y = OVERVIEW_TOP + p * rh;
			bool active = p == shown;
			if (active) {
				nvgBeginPath(vg);
				nvgRoundedRect(vg, PAD - 1.f, y, size().x - 2 * PAD + 2.f, rh - 0.4f, 1.2f);
				nvgFillColor(vg, nvgRGBAf(1.f, 1.f, 1.f, 0.06f));
				nvgFill(vg);
				nvgBeginPath(vg);
				nvgRoundedRect(vg, PAD - 1.f, y + 1.2f, 0.6f, rh - 2.8f, 0.3f);
				nvgFillColor(vg, PAGE_COLORS[p]);
				nvgFill(vg);
			}
			for (int d = 0; d < 4; d++) {
				int param = Track::DIALS[p][d];
				if (param < 0)
					continue;
				ParamQuantity* q = track->paramQuantities[param];
				float cx = PAD + cw * (d + 0.5f), cy = y + 3.4f, r = 2.4f;
				float a0 = 0.75f * M_PI, a1 = 2.25f * M_PI;
				// Bipolar dials fill from their zero, the others from the left.
				float zero = q->minValue < 0.f && q->maxValue > 0.f ? -q->minValue / (q->maxValue - q->minValue) : 0.f;
				float from = a0 + (a1 - a0) * zero, to = a0 + (a1 - a0) * q->getScaledValue();
				bool focused = param == focus;
				nvgBeginPath(vg);
				nvgArc(vg, cx, cy, r, a0, a1, NVG_CW);
				stroke(vg, FAINT, 0.6f);
				if (std::fabs(to - from) > 0.02f) {
					nvgBeginPath(vg);
					nvgArc(vg, cx, cy, r, std::fmin(from, to), std::fmax(from, to), NVG_CW);
					stroke(vg, nvgTransRGBAf(DIAL_COLORS[d], active ? 1.f : 0.4f), focused ? 0.9f : 0.6f);
				}
				text(vg, cx, y + 7.2f, 1.5f, active || focused ? INK : nvgTransRGBAf(DIM, 0.7f), NVG_ALIGN_CENTER, track->dialName(param));
			}
		}
	}

	// Touch: click a dial in the overview to select it and its page, then drag anywhere to change it.

	/** The dial under a point, in mm, or -1. */
	int dialAt(math::Vec mm, int* page) {
		if (mm.y < OVERVIEW_TOP)
			return -1;
		int p = int((mm.y - OVERVIEW_TOP) / rowHeight()), d = int((mm.x - PAD) / cellWidth());
		if (p < 0 || p >= PAGES_LEN || d < 0 || d >= 4)
			return -1;
		*page = p;
		return Track::DIALS[p][d];
	}

	void onButton(const ButtonEvent& e) override {
		Track* track = module ? module->track() : nullptr;
		if (track && e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
			int page, param = dialAt(e.pos.div(mm2px(1.f)), &page);
			if (param >= 0) {
				focus = param;
				if (page == PAGE_TRACK)
					track->setTrackMode(true);
				else if (track->shownPage() != page)
					track->showPage(page);
				lastPage = track->shownPage();
			}
		}
		OpaqueWidget::onButton(e);
	}

	void onDragStart(const DragStartEvent& e) override {
		Track* track = module ? module->track() : nullptr;
		if (!track || e.button != GLFW_MOUSE_BUTTON_LEFT || focus < 0)
			return;
		dragging = true;
		dragStart = track->params[focus].getValue();
		dragValue = track->paramQuantities[focus]->getScaledValue();
	}

	void onDragMove(const DragMoveEvent& e) override {
		Track* track = module ? module->track() : nullptr;
		if (!track || !dragging)
			return;
		// 250 pixels of travel sweep the whole range, or a tenth of it with Ctrl held, like Rack's knobs.
		float delta = -e.mouseDelta.y / 250.f;
		if ((APP->window->getMods() & RACK_MOD_MASK) == RACK_MOD_CTRL)
			delta /= 10.f;
		dragValue = clamp(dragValue + delta, 0.f, 1.f);
		track->paramQuantities[focus]->setScaledValue(dragValue);
	}

	void onDragEnd(const DragEndEvent& e) override {
		Track* track = module ? module->track() : nullptr;
		if (!track || !dragging)
			return;
		dragging = false;
		float value = track->params[focus].getValue();
		if (value != dragStart) {
			history::ParamChange* h = new history::ParamChange;
			h->name = "change " + track->paramQuantities[focus]->getLabel();
			h->moduleId = track->id;
			h->paramId = focus;
			h->oldValue = dragStart;
			h->newValue = value;
			APP->history->push(h);
		}
	}
};


struct ScreenWidget : ModuleWidget {
	ScreenWidget(Screen* module) {
		setModule(module);
		std::string panel = asset::plugin(pluginInstance, "res/Screen.svg");
		setPanel(createPanel(panel, asset::plugin(pluginInstance, "res/Screen-dark.svg")));
		addScrews(this);
		Display* display = createWidget<Display>(math::Vec());
		display->box = PanelLayout(panel).box("DISPLAY_WIDGET");
		display->module = module;
		addChild(display);
	}
};

} // namespace opz

Model* modelScreen = createModel<opz::Screen, opz::ScreenWidget>("Screen");
