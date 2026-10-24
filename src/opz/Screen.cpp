/*
Screen: the OP-Z's phone app as an expander. Placed anywhere on the Sequencer's row, it shows the selected track: a
picture of the page the dials are on (the filter over a scope, the envelope with a dot per voice, the LFO or the arp, the mix, the track settings), the
value of the dial last touched, and every dial of every page. Click a dial on it to select it and its page, and drag
anywhere on it to change the selected dial, like the app's touch pad. On a drum track, KIT swaps the picture for a kit
editor, as teenage engineering's drum utility does it: the whole kit, the slice played last up close with its ends to
drag, and its tuning, level, play mode and direction.
*/
#include "Hub.hpp"
#include "../ui.hpp"

namespace opz {

/** The module only carries the bus on, which takes nothing a sample; its widget reads the track to its left. */
struct Screen : Module {
	Screen() {
		config(0, 0, 0, 0);
	}

	void onExpanderChange(const ExpanderChangeEvent& e) override {
		busMoved();
	}

	/** For the UI thread: the core playing the Sequencer's selected track, or null off a row or on the master track;
	and what it plays. */
	TrackCore* track() {
		Hub* h = findHub(this);
		return h ? h->trackCore(h->selectedTrack()) : nullptr;
	}

	std::string plugName() {
		Hub* h = findHub(this);
		return h ? h->plugName(h->selectedTrack()) : "";
	}
};


/** A slice's settings changed on the Screen, for the undo history. */
struct SliceChange : history::ModuleAction {
	int track = 0, slice = 0;
	Kit::Slice before, after;

	void apply(const Kit::Slice& s) {
		if (Hub* h = dynamic_cast<Hub*>(APP->engine->getModule(moduleId)))
			if (TrackCore* t = h->trackCore(track))
				t->editSlice(slice, s);
	}
	void undo() override {
		apply(before);
	}
	void redo() override {
		apply(after);
	}
};


static const NVGcolor INK = nvgRGB(0xe8, 0xe7, 0xe3), DIM = nvgRGB(0x8c, 0x8b, 0x88), FAINT = nvgRGB(0x3a, 0x3b, 0x3f),
	WELL = nvgRGB(0x0d, 0x0e, 0x10);

/** Draws in millimetres, laid out like the mockup: a status bar, the selected dial's value, the page's picture, and
an overview of every dial with a row per page. */
struct Display : widget::OpaqueWidget {
	static constexpr float PAD = 2.6f, VISUAL_TOP = 19.f, VISUAL_HEIGHT = 38.f, OVERVIEW_TOP = 60.5f;

	Screen* module = nullptr;
	const TrackCore::Snapshot* snapshot = nullptr;
	int focus = TrackCore::P1_PARAM;          // the dial the readout shows and a drag changes
	float last[TrackCore::PARAMS_LEN] = {};   // dial values last frame, to see which one was touched
	int lastPage = -1;
	TrackCore* lastTrack = nullptr;
	float dragValue = 0.f, dragStart = 0.f;
	bool dragging = false;
	// Dummy snapshot for before the first one arrives.
	TrackCore::Snapshot empty = {};

	// The kit editor
	static constexpr int PEAKS = 256;
	static constexpr const char* FIELDS[4] = {"TUNE", "LEVEL", "PLAY", "REVERSE"};
	bool kitMode = false;
	int slice = 0, seenSlice = -1;      // the slice shown, and the last one played when it was picked
	const Kit* peaksOf = nullptr;       // the kit, and its length, that `peaks` are of
	size_t peaksSize = 0;
	std::vector<float> peaks;           // the loudest sample in each of PEAKS stretches of the kit, for the whole of it
	enum { NO_EDIT, START, END, TUNE, LEVEL } kitDrag = NO_EDIT;
	Kit::Slice before;                  // the slice as a drag found it
	double dragged = 0.0;               // where the drag has the end, tuning or level it moves
	double viewFrom = 0.0, viewTo = 1.0; // the samples the close-up shows, held still while an end is dragged
	math::Vec pressed;                  // where the last press was, in mm

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
	static bool locate(TrackCore* track, int param, int* page, int* dial) {
		for (int p = 0; p < PAGES_LEN; p++)
			for (int d = 0; d < 4; d++)
				if (track->dial(p, d) == param) {
					*page = p;
					*dial = d;
					return true;
				}
		return false;
	}

	static int firstDial(TrackCore* track, int page) {
		for (int d = 0; d < 4; d++)
			if (track->dial(page, d) >= 0)
				return track->dial(page, d);
		return -1;
	}

	void step() override {
		TrackCore* track = module ? module->track() : nullptr;
		if (track) {
			if (track->snapshots.update() || !snapshot || track != lastTrack)
				snapshot = &track->snapshots.read();
			// The readout follows whichever dial moves, and the page when it changes.
			for (int i = 0; i < TrackCore::PARAMS_LEN; i++) {
				float v = track->params[i].getValue();
				if (v != last[i] && !dragging && track == lastTrack)
					focus = i;
				last[i] = v;
			}
			if (track != lastTrack)
				seenSlice = -1;
			if (track->lastSlice() != seenSlice)
				slice = seenSlice = track->lastSlice();
			lastTrack = track;
			int shown = track->shownPage(), page, dial;
			if (shown != lastPage && !(locate(track, focus, &page, &dial) && page == shown))
				focus = firstDial(track, shown);
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
				if (TrackCore* track = module ? module->track() : nullptr)
					drawTrack(vg, track, module->plugName(), snapshot ? *snapshot : empty);
				else
					drawHint(vg);
				nvgRestore(vg);
			}
		}
		OpaqueWidget::drawLayer(args, layer);
	}

	/** Off a Sequencer's row, or on the master track, the screen says where it goes. */
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
		text(vg, cx, cy + 15.f, 2.4f, INK, NVG_ALIGN_CENTER, "place me to the right of");
		text(vg, cx, cy + 18.5f, 2.4f, INK, NVG_ALIGN_CENTER, "the sequencer, on a track");
	}

	void drawTrack(NVGcontext* vg, TrackCore* track, const std::string& name, const TrackCore::Snapshot& snap) {
		math::Vec s = size();
		int shown = track->shownPage();

		// Status bar
		text(vg, PAD, 4.2f, 2.8f, INK, NVG_ALIGN_LEFT, string::lowercase(name));
		text(vg, s.x / 2, 4.2f, 2.3f, PAGE_COLORS[shown], NVG_ALIGN_CENTER, track->pageName(shown));
		text(vg, s.x - PAD, 4.2f, 2.1f, DIM, NVG_ALIGN_RIGHT, string::f("%.0f BPM", 60.f / track->secondsPerBeat));
		nvgBeginPath(vg);
		nvgMoveTo(vg, PAD, 7.4f);
		nvgLineTo(vg, s.x - PAD, 7.4f);
		stroke(vg, FAINT, 0.25f);

		// The selected dial's name in its colour, and its value; or in the kit editor, the slice's name and length.
		const Kit* kit = track->kit();
		bool editing = kit && kitMode;
		int page, dial;
		if (kit) {
			math::Rect chip = kitChip();
			nvgBeginPath(vg);
			nvgRoundedRect(vg, RECT_ARGS(chip), 1.f);
			if (editing) {
				nvgFillColor(vg, INK);
				nvgFill(vg);
			}
			else
				stroke(vg, DIM, 0.25f);
			text(vg, chip.getCenter().x, chip.getCenter().y, 1.8f, editing ? WELL : INK, NVG_ALIGN_CENTER, "KIT");
		}
		if (editing) {
			const Kit::Slice& s = kit->slices[slice];
			text(vg, PAD, 10.6f, 2.0f, DIM, NVG_ALIGN_LEFT, string::f("sound %d", slice + 1));
			text(vg, PAD, 15.0f, 3.2f, INK, NVG_ALIGN_LEFT, s.empty() ? "no sound" : !s.name.empty() ? s.name : kit->name);
			text(vg, kitChip().pos.x - 1.5f, 10.6f, 2.0f, DIM, NVG_ALIGN_RIGHT, string::f("%.2f s", (s.end - s.start) / kit->rate));
		}
		else if (focus >= 0 && locate(track, focus, &page, &dial)) {
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
		switch (editing ? -1 : shown) {
			case -1: drawKit(vg, *kit, v); break;
			case PAGE_SOUND: drawSound(vg, track, snap, v); break;
			case PAGE_ENVELOPE: drawEnvelope(vg, track, snap, v); break;
			case PAGE_LFO: track->arp() ? drawArp(vg, track, v) : drawLfo(vg, track, snap, v); break;
			case PAGE_MIX: drawMix(vg, track, snap, v); break;
			default: drawTrackSettings(vg, track, v); break;
		}
		nvgRestore(vg);

		drawOverview(vg, track, shown);
	}

	// Kit editor layout, in mm: KIT beside the readout, then in the picture's place the whole kit, the slice up close,
	// and its four settings.

	math::Rect kitChip() const {
		return math::Rect(size().x - PAD - 8.f, 9.f, 8.f, 3.6f);
	}

	math::Rect kitWhole() const {
		math::Rect v = visual();
		return math::Rect(v.pos.x + 1.5f, v.pos.y + 1.5f, v.size.x - 3.f, 7.f);
	}

	math::Rect kitClose() const {
		math::Rect v = visual();
		return math::Rect(v.pos.x + 1.5f, v.pos.y + 10.5f, v.size.x - 3.f, 17.f);
	}

	math::Rect kitField(int i) const {
		math::Rect v = visual();
		float w = (v.size.x - 3.f) / 4;
		return math::Rect(v.pos.x + 1.5f + w * i, v.pos.y + 29.5f, w, 7.5f);
	}

	/** The close-up: the slice with a sixth of its length either side, or a little of the kit where it's empty. */
	void frame(const Kit::Slice& s) {
		double length = std::max(s.end - s.start, 2048);
		viewFrom = s.start - length / 6;
		viewTo = s.start + length * 7 / 6;
	}

	static std::string fieldValue(int i, const Kit::Slice& s) {
		switch (i) {
			case 0: return string::f("%+.1f st", 12.f * std::log2(s.speed));
			case 1: return string::f("%.0f%%", 100.f * s.gain);
			case 2: return s.mode == Kit::GATE ? "GATE" : s.mode == Kit::LOOP ? "LOOP" : "ONE SHOT";
			default: return s.reverse ? "ON" : "OFF";
		}
	}

	/** The whole kit with the slice shaded, the slice up close between its ends, and its settings. */
	void drawKit(NVGcontext* vg, const Kit& k, math::Rect v) {
		size_t n = k.samples.size();
		if (&k != peaksOf || n != peaksSize) {
			peaks.assign(PEAKS, 0.f);
			for (size_t i = 0; i < n; i++)
				peaks[i * PEAKS / n] = std::fmax(peaks[i * PEAKS / n], std::fabs(k.samples[i]));
			float most = *std::max_element(peaks.begin(), peaks.end());
			for (float& p : peaks)
				p /= std::fmax(most, 1e-6f);
			peaksOf = &k;
			peaksSize = n;
		}
		const Kit::Slice& s = k.slices[slice];

		math::Rect w = kitWhole();
		float mid = w.getCenter().y;
		auto wholeX = [&](double at) { return w.pos.x + w.size.x * float(at / std::max<size_t>(n, 1)); };
		nvgBeginPath(vg);
		nvgRect(vg, wholeX(s.start), w.pos.y, std::fmax(wholeX(s.end) - wholeX(s.start), 0.3f), w.size.y);
		nvgFillColor(vg, nvgTransRGBAf(DIAL_COLORS[0], 0.25f));
		nvgFill(vg);
		nvgBeginPath(vg);
		for (int b = 0; b < PEAKS; b++) {
			float x = w.pos.x + w.size.x * (b + 0.5f) / PEAKS, h = std::fmax(peaks[b] * w.size.y / 2, 0.1f);
			nvgMoveTo(vg, x, mid - h);
			nvgLineTo(vg, x, mid + h);
		}
		stroke(vg, DIM, 0.15f);
		nvgBeginPath(vg);
		for (const Kit::Slice& other : k.slices)
			if (!other.empty()) {
				nvgMoveTo(vg, wholeX(other.start), w.getBottom() - 1.f);
				nvgLineTo(vg, wholeX(other.start), w.getBottom());
			}
		stroke(vg, INK, 0.2f);

		math::Rect c = kitClose();
		if (kitDrag != START && kitDrag != END)
			frame(s);
		auto closeX = [&](double at) { return c.pos.x + c.size.x * float((at - viewFrom) / (viewTo - viewFrom)); };
		const int COLUMNS = 120;
		float cmid = c.getCenter().y;
		for (int pass = 0; pass < 2; pass++) {
			nvgBeginPath(vg);
			for (int col = 0; col < COLUMNS; col++) {
				double from = viewFrom + (viewTo - viewFrom) * col / COLUMNS, to = viewFrom + (viewTo - viewFrom) * (col + 1) / COLUMNS;
				bool inside = from >= s.start && from < s.end;
				if (inside != (pass == 1))
					continue;
				long a = std::max(0L, (long) std::floor(from)), b = std::min((long) n, (long) std::ceil(to));
				float lo = 0.f, hi = 0.f;
				for (long i = a, stride = std::max(1L, (b - a) / 64); i < b; i += stride) {
					lo = std::fmin(lo, k.samples[i]);
					hi = std::fmax(hi, k.samples[i]);
				}
				float x = c.pos.x + c.size.x * (col + 0.5f) / COLUMNS;
				nvgMoveTo(vg, x, cmid - clamp(hi, -1.f, 1.f) * c.size.y / 2 - 0.05f);
				nvgLineTo(vg, x, cmid - clamp(lo, -1.f, 1.f) * c.size.y / 2 + 0.05f);
			}
			stroke(vg, pass ? nvgTransRGBAf(INK, 0.8f) : nvgTransRGBAf(DIM, 0.35f), 0.25f);
		}
		for (int e = 0; e < 2; e++) {
			float x = closeX(e ? s.end : s.start);
			nvgBeginPath(vg);
			nvgMoveTo(vg, x, c.pos.y);
			nvgLineTo(vg, x, c.getBottom());
			stroke(vg, DIAL_COLORS[e], 0.35f);
			nvgBeginPath(vg);
			nvgRoundedRect(vg, e ? x - 1.6f : x, c.pos.y, 1.6f, 2.2f, 0.4f);
			nvgFillColor(vg, DIAL_COLORS[e]);
			nvgFill(vg);
		}
		if (s.empty())
			text(vg, c.getCenter().x, c.getCenter().y, 1.8f, DIM, NVG_ALIGN_CENTER, "drag the ends apart for a sound");

		for (int i = 0; i < 4; i++) {
			math::Rect f = kitField(i);
			float cx = f.getCenter().x;
			text(vg, cx, f.pos.y + 1.6f, 1.4f, DIM, NVG_ALIGN_CENTER, FIELDS[i]);
			text(vg, cx, f.pos.y + 4.4f, 2.0f, INK, NVG_ALIGN_CENTER, fieldValue(i, s));
			nvgBeginPath(vg);
			nvgRoundedRect(vg, cx - 4.f, f.pos.y + 6.6f, 8.f, 0.5f, 0.25f);
			nvgFillColor(vg, DIAL_COLORS[i]);
			nvgFill(vg);
		}
	}

	/** A slice's new settings, and the undo history's step for them when `from` is what they were. */
	void editSlice(TrackCore* track, const Kit::Slice& to, const Kit::Slice* from = nullptr) {
		track->editSlice(slice, to);
		if (!from)
			return;
		const Kit::Slice& now = track->kit()->slices[slice];
		if (now.start == from->start && now.end == from->end && now.speed == from->speed && now.gain == from->gain
			&& now.mode == from->mode && now.reverse == from->reverse)
			return;
		SliceChange* h = new SliceChange;
		h->name = string::f("edit sound %d", slice + 1);
		h->moduleId = track->paramQuantities[TrackCore::P1_PARAM]->module->id; // the Sequencer, whose params it plays by
		h->track = track->busTrack;
		h->slice = slice;
		h->before = *from;
		h->after = now;
		APP->history->push(h);
	}

	/** A press in the kit editor: KIT itself, a slice picked on the whole kit, or the play mode or direction. */
	bool pressKit(TrackCore* track) {
		const Kit* k = track->kit();
		if (!k)
			return false;
		if (kitChip().contains(pressed)) {
			kitMode = !kitMode;
			return true;
		}
		if (!kitMode)
			return false;
		math::Rect w = kitWhole();
		if (w.contains(pressed)) {
			double at = (pressed.x - w.pos.x) / w.size.x * k->samples.size();
			for (int i = 0; i < Kit::SLICES; i++)
				if (!k->slices[i].empty() && at >= k->slices[i].start && at < k->slices[i].end)
					slice = i;
			return true;
		}
		Kit::Slice s = k->slices[slice], was = s;
		if (kitField(2).contains(pressed))
			s.mode = s.mode == Kit::ONESHOT ? Kit::GATE : s.mode == Kit::GATE ? Kit::LOOP : Kit::ONESHOT;
		else if (kitField(3).contains(pressed))
			s.reverse = !s.reverse;
		else
			return kitClose().contains(pressed) || kitField(0).contains(pressed) || kitField(1).contains(pressed);
		editSlice(track, s, &was);
		return true;
	}

	/** A scope of the output, and the filter's response over it. */
	void drawSound(NVGcontext* vg, TrackCore* track, const TrackCore::Snapshot& snap, math::Rect v) {
		// Start the trace on a rising zero crossing, so a steady note stands still.
		const int N = TrackCore::Snapshot::SCOPE / 2;
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
		float filter = track->params[TrackCore::FILTER_PARAM].getValue();
		float q = 1.f / (2.f - 1.9f * track->params[TrackCore::RESONANCE_PARAM].getValue());
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
		// An FX module's time, as its notes set it.
		if (track->effect && track->timeKey >= 0)
			text(vg, v.getRight() - 1.5f, v.pos.y + 3.2f, 1.8f, INK, NVG_ALIGN_RIGHT, track->soundName(FX_LOWEST + track->timeKey));
		text(vg, v.pos.x + 1.5f, v.pos.y + v.size.y - 1.6f, 1.6f, DIM, NVG_ALIGN_LEFT, "20 Hz");
		text(vg, v.getRight() - 1.5f, v.pos.y + v.size.y - 1.6f, 1.6f, DIM, NVG_ALIGN_RIGHT, "20 kHz");
	}

	/** The envelope, each stage in its dial's colour, with a dot riding it for each sounding voice. */
	void drawEnvelope(NVGcontext* vg, TrackCore* track, const TrackCore::Snapshot& snap, math::Rect v) {
		float a = 0.1f + track->params[TrackCore::ATTACK_PARAM].getValue();
		float d = 0.1f + track->params[TrackCore::DECAY_PARAM].getValue();
		float sustain = track->params[TrackCore::SUSTAIN_PARAM].getValue();
		float r = 0.1f + track->params[TrackCore::RELEASE_PARAM].getValue();
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
	void drawLfo(NVGcontext* vg, TrackCore* track, const TrackCore::Snapshot& snap, math::Rect v) {
		int shape = (int) track->params[TrackCore::LFO_SHAPE_PARAM].getValue();
		float amount = track->params[TrackCore::LFO_AMOUNT_PARAM].getValue();
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

		ParamQuantity* speed = track->paramQuantities[TrackCore::LFO_SPEED_PARAM];
		ParamQuantity* target = track->paramQuantities[TrackCore::LFO_TARGET_PARAM];
		text(vg, v.getRight() - 1.8f, v.pos.y + 3.f, 1.7f, DIM, NVG_ALIGN_RIGHT,
			string::uppercase(speed->getDisplayValueString() + " \xc2\xb7 " + target->getDisplayValueString()));
	}

	/** Sixteen beats of the arp over a triad: each note as high as its place in the pattern, as big as its accent. */
	void drawArp(NVGcontext* vg, TrackCore* track, math::Rect v) {
		auto value = [&](int param) { return track->params[param].getValue(); };
		Arp a{(int) value(TrackCore::ARP_SPEED_PARAM), (int) value(TrackCore::ARP_PATTERN_PARAM), (int) value(TrackCore::ARP_STYLE_PARAM),
			value(TrackCore::ARP_RANGE_PARAM)};
		const int BEATS = 16, m = 3 * a.octaves();
		static const int RANDOM[BEATS] = {5, 1, 9, 4, 0, 7, 11, 2, 8, 3, 10, 6, 1, 9, 4, 7};
		float x0 = v.pos.x + 4.f, w = v.size.x - 8.f, bottom = v.getBottom() - 4.f, h = v.size.y - 11.f;
		auto y = [&](int j) { return bottom - h * j / std::max(m - 1, 1); };
		for (int j = 0; j < m; j++) {
			nvgBeginPath(vg);
			nvgMoveTo(vg, x0, y(j));
			nvgLineTo(vg, x0 + w, y(j));
			stroke(vg, j % 3 ? nvgTransRGBAf(FAINT, 0.5f) : FAINT, 0.25f);
		}
		const char* rhythm = ARP_RHYTHMS[a.style];
		NVGcolor color = a.speed > 0 ? DIAL_COLORS[3] : DIM;
		for (long b = 0, k = 0; b < BEATS; b++) {
			char beat = rhythm[b % std::strlen(rhythm)];
			if (beat == '-')
				continue;
			int j = a.pattern == ARP_RANDOM ? RANDOM[k % BEATS] % m : Arp::pick(a.pattern, m, k);
			k++;
			nvgBeginPath(vg);
			nvgCircle(vg, x0 + w * (b + 0.5f) / BEATS, y(j), beat == 'H' ? 1.1f : beat == 'M' ? 0.85f : 0.6f);
			nvgFillColor(vg, color);
			nvgFill(vg);
		}
		std::string label = a.speed > 0 ? track->paramQuantities[TrackCore::ARP_SPEED_PARAM]->getDisplayValueString() + " \xc2\xb7 "
			+ track->paramQuantities[TrackCore::ARP_PATTERN_PARAM]->getDisplayValueString() : "OFF";
		text(vg, v.getRight() - 1.8f, v.pos.y + 3.f, 1.7f, DIM, NVG_ALIGN_RIGHT, string::uppercase(label));
	}

	/** Where the track sits in the stereo field, how loud it is, and what it sends to the effects. */
	void drawMix(NVGcontext* vg, TrackCore* track, const TrackCore::Snapshot& snap, math::Rect v) {
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
		float fx1 = track->params[TrackCore::FX1_PARAM].getValue(), fx2 = track->params[TrackCore::FX2_PARAM].getValue();
		bar(v.pos.y + 9.f, 0.f, fx1, DIAL_COLORS[0], "FX 1");
		bar(v.pos.y + 18.f, 0.f, fx2, DIAL_COLORS[1], "FX 2");
		// Pan as a dot on a line from L to R.
		float pan = track->params[TrackCore::PAN_PARAM].getValue();
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
		float level = track->params[TrackCore::LEVEL_PARAM].getValue();
		float ly = bottom - (bottom - top) * level * level / 2;
		nvgBeginPath(vg);
		nvgMoveTo(vg, mx - 1.6f, ly);
		nvgLineTo(vg, mx + 6.f, ly);
		stroke(vg, DIAL_COLORS[3], 0.4f);
	}

	/** The note style, and the portamento, where the track has one, as a slide between two notes. */
	void drawTrackSettings(NVGcontext* vg, TrackCore* track, math::Rect v) {
		int style = (int) track->params[TrackCore::STYLE_PARAM].getValue();
		const std::vector<std::string>& styles = static_cast<SwitchQuantity*>(track->paramQuantities[TrackCore::STYLE_PARAM])->labels;
		int n = styles.size();
		float cw = (v.size.x - 4.f) / n;
		for (int i = 0; i < n; i++) {
			float x = v.pos.x + 2.f + cw * (i + 0.5f);
			text(vg, x, v.pos.y + 4.5f, 2.2f, i == style ? INK : DIM, NVG_ALIGN_CENTER, string::uppercase(styles[i]));
			if (i == style) {
				nvgBeginPath(vg);
				nvgRoundedRect(vg, x - 4.f, v.pos.y + 6.6f, 8.f, 0.6f, 0.3f);
				nvgFillColor(vg, DIAL_COLORS[1]);
				nvgFill(vg);
			}
		}

		if (track->dial(PAGE_TRACK, 3) < 0)
			return;
		// A step up a fifth, smoothed by the portamento.
		float glide = track->params[TrackCore::GLIDE_PARAM].getValue();
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
	}

	/** Every dial as an arc in its colour, a row per page, with the page the dials are on lit up. */
	void drawOverview(NVGcontext* vg, TrackCore* track, int shown) {
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
				int param = track->dial(p, d);
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
	int dialAt(TrackCore* track, math::Vec mm, int* page) {
		if (mm.y < OVERVIEW_TOP)
			return -1;
		int p = int((mm.y - OVERVIEW_TOP) / rowHeight()), d = int((mm.x - PAD) / cellWidth());
		if (p < 0 || p >= PAGES_LEN || d < 0 || d >= 4)
			return -1;
		*page = p;
		return track->dial(p, d);
	}

	void onButton(const ButtonEvent& e) override {
		TrackCore* track = module ? module->track() : nullptr;
		if (track && e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
			pressed = e.pos.div(mm2px(1.f));
			int page, param = pressKit(track) ? -1 : dialAt(track, pressed, &page);
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
		TrackCore* track = module ? module->track() : nullptr;
		const Kit* k = track && kitMode ? track->kit() : nullptr;
		if (k && e.button == GLFW_MOUSE_BUTTON_LEFT) {
			before = k->slices[slice];
			math::Rect c = kitClose();
			if (c.contains(pressed)) {
				double at = viewFrom + (viewTo - viewFrom) * (pressed.x - c.pos.x) / c.size.x;
				kitDrag = std::fabs(at - before.start) <= std::fabs(at - before.end) ? START : END;
				dragged = kitDrag == START ? before.start : before.end;
			}
			else if (kitField(0).contains(pressed)) {
				kitDrag = TUNE;
				dragged = 12.0 * std::log2(before.speed);
			}
			else if (kitField(1).contains(pressed)) {
				kitDrag = LEVEL;
				dragged = before.gain;
			}
			if (kitDrag != NO_EDIT)
				return;
		}
		if (!track || e.button != GLFW_MOUSE_BUTTON_LEFT || focus < 0)
			return;
		dragging = true;
		dragStart = track->params[focus].getValue();
		dragValue = track->paramQuantities[focus]->getScaledValue();
	}

	void onDragMove(const DragMoveEvent& e) override {
		TrackCore* track = module ? module->track() : nullptr;
		bool fine = (APP->window->getMods() & RACK_MOD_MASK) == RACK_MOD_CTRL;
		if (track && kitDrag != NO_EDIT && track->kit()) {
			// The ends follow the pointer across the close-up; tuning and level go 250 pixels up for 48 semitones or 200%.
			Kit::Slice s = track->kit()->slices[slice];
			float across = e.mouseDelta.x / getAbsoluteZoom() / mm2px(1.f) / kitClose().size.x, up = -e.mouseDelta.y / 250.f;
			dragged += (kitDrag == START || kitDrag == END ? across * (viewTo - viewFrom) : kitDrag == TUNE ? 48.0 * up : 2.0 * up) * (fine ? 0.1 : 1.0);
			if (kitDrag == START || kitDrag == END)
				(kitDrag == START ? s.start : s.end) = (int) std::lround(dragged);
			else if (kitDrag == TUNE)
				s.speed = (float) std::exp2((dragged = std::clamp(dragged, -24.0, 24.0)) / 12.0);
			else
				s.gain = float(dragged = std::clamp(dragged, 0.0, 4.0));
			return editSlice(track, s);
		}
		if (!track || !dragging)
			return;
		// 250 pixels of travel sweep the whole range, or a tenth of it with Ctrl held, like Rack's knobs.
		float delta = -e.mouseDelta.y / 250.f;
		if (fine)
			delta /= 10.f;
		dragValue = clamp(dragValue + delta, 0.f, 1.f);
		track->paramQuantities[focus]->setScaledValue(dragValue);
	}

	void onDragEnd(const DragEndEvent& e) override {
		TrackCore* track = module ? module->track() : nullptr;
		if (kitDrag != NO_EDIT) {
			kitDrag = NO_EDIT;
			if (track && track->kit())
				editSlice(track, track->kit()->slices[slice], &before);
			return;
		}
		if (!track || !dragging)
			return;
		dragging = false;
		float value = track->params[focus].getValue();
		if (value != dragStart) {
			ParamQuantity* q = track->paramQuantities[focus];
			history::ParamChange* h = new history::ParamChange;
			h->name = "change " + q->getLabel();
			h->moduleId = q->module->id;
			h->paramId = q->paramId;
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
