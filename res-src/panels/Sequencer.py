"""The sequencer: the OP-Z's face, and the master of the engines to its right. Under the face, the jacks: the transport
in and out, the notes to record, and the engines' mix.

The face copies the OP-Z's: four dials over 17 columns of round keys. TRACK, REC and −, PLAY and +, STOP and SHIFT
on the left; the steps along the top under the track icons; the numbered black keys on the column lines; the white
keys along the bottom under the step component icons. The index keys sit on its top edge, as the OP-Z's do on top of
it. Beside the dials, where the OP-Z's speaker is, goes the display the hardware has no room for, standing in for the
phone app: the track's plug, the tempo, a key per track and the four pages. It is drawn at the size of the 24 HP panel it was first made for, then scaled up by SCALE."""
from opz import COMPONENTS, KEY, SCALE, TRACKS, icon, number_key, round_key
from panelkit import HP, LABEL, NUNITO, Panel, num

WIDTH, HEIGHT = 51 * HP, 128.5
COLUMN = (24 * HP - 6.0) / 17                # the face's key columns
CELL, ROW = 2 * COLUMN + 4.5, 7.2            # the height of the dials' row, and the keys' row pitch
FACE_WIDTH, FACE_HEIGHT = 17 * COLUMN, CELL + 7.6 + 2 * ROW + 4.0
FACE_TOP = 10.5
TOP, MID, BOTTOM = CELL + 7.6, CELL + 7.6 + ROW, CELL + 7.6 + 2 * ROW
BLANK = 8 * COLUMN                           # the blank space left of the dials
ICON = 4.1                                   # how far an icon sits above its key
KEY_PITCH = 5.2                              # between the index keys
BLACK = [1, 3, 5, 8, 10]                     # the black keys' semitones above F
WHITE = [0, 2, 4, 6, 7, 9, 11]
DISPLAY_LEFT, DISPLAY_TOP = 1.6, 3.2         # the display's box, in the face's millimetres
DISPLAY_W, DISPLAY_H = BLANK - 2 * DISPLAY_LEFT, CELL - 1.4 - DISPLAY_TOP
PAGE_LEFT, PAGE_ROW = 39.6, 3.1              # the page list's left edge, from the display's, and its rows' pitch
TRACK_ROW, TRACK_PITCH, TRACK_KEY = 10.9, 3.55, 3.3   # the track keys' row, from the display's top, pitch and size

JACK_ROW = 117.5
PITCH = 11.6                                 # between the jacks' columns
MARGIN = 6.0                                 # from the panel's edges to the outermost jacks' columns' edges


class Scaled:
    """Draws on a panel in the face's own millimetres, scaled up by SCALE from (x, y)."""

    def __init__(self, panel, x, y):
        self.panel, self.x, self.y, self.art = panel, x, y, []

    def place(self, name, x, y, w=0, h=0):
        self.panel.place(name, self.x + SCALE * x, self.y + SCALE * y, SCALE * w, SCALE * h)

    def line(self, x1, y1, x2, y2):
        self.art.append(f'<path d="M{num(x1)} {num(y1)}L{num(x2)} {num(y2)}" stroke="{{line}}" stroke-width="0.25"/>')

    def __exit__(self, *_):
        self.panel.art.append(f'<g transform="translate({num(self.x)} {num(self.y)}) scale({num(SCALE)})">'
                              + ''.join(self.art) + '</g>')

    def __enter__(self):
        return self


def x(column):
    """The centre of a key column, 1 to 17."""
    return COLUMN * (column - 0.5)


def display(f, left, top):
    """The display: the plug top left, the tempo with INT and EXT under it beside that, the track keys along the
    bottom and the pages down the right. The module draws all of it but the box and the INT and EXT labels; the keys
    and pages are boxes for it to take clicks in."""
    f.art.append(f'<rect x="{num(left)}" y="{num(top)}" width="{num(DISPLAY_W)}" height="{num(DISPLAY_H)}" rx="1.2" fill="{{box}}"/>')
    f.place('PLUG_WIDGET', left + 0.8, top + 0.6, 22.4, 7.6)
    f.place('TEMPO_WIDGET', left + 24.2, top + 0.6, 12.6, 5.8)
    for x, name, label in [(left + 25.0, 'INTERNAL', 'INT'), (left + 31.4, 'EXTERNAL', 'EXT')]:
        f.place(f'{name}_LIGHT', x, top + 7.6)
        f.art.append(f'<path d="{NUNITO.path(label, x + 2.6, top + 7.6, 1.5)}" fill="{{box_ink}}"/>')
    keys = [f'SELECT{t}_PARAM' for t in range(8)] + ['FX_SELECT0_PARAM', 'FX_SELECT1_PARAM', 'MASTER_PARAM']
    for i, name in enumerate(keys):
        cx = left + 2.2 + TRACK_PITCH * i
        f.place(name, cx - TRACK_KEY / 2, top + TRACK_ROW - TRACK_KEY / 2, TRACK_KEY, TRACK_KEY)
    for i in range(4):
        cy = top + 2.0 + PAGE_ROW * i
        f.place(f'PAGE{i}_PARAM', left + PAGE_LEFT, cy - PAGE_ROW / 2, DISPLAY_W - PAGE_LEFT - 0.6, PAGE_ROW)


def face(f):
    # The outline, the line between the dials and the keys, and the lines between the columns.
    f.art.append(f'<rect x="0" y="0" width="{num(FACE_WIDTH)}" height="{num(FACE_HEIGHT)}" rx="1.2" fill="none" '
                 f'stroke="{{line}}" stroke-width="0.25"/>')
    f.line(0, CELL, FACE_WIDTH, CELL)
    for k in range(1, 17):
        f.line(COLUMN * k, CELL, COLUMN * k, FACE_HEIGHT)
    for k in (9, 11, 13, 15):
        f.line(COLUMN * k, 0, COLUMN * k, CELL)

    # The index keys, project, mixer, tempo and screen, straddle the top edge in the corner, each in a gap in it.
    for i, (name, symbol) in enumerate([('PROJECT', 'project'), ('MIXER', 'mixer'), ('METRONOME', 'metronome'), ('SCREEN', 'screen')]):
        cx = 4.0 + KEY_PITCH * i
        f.art.append(f'<circle cx="{num(cx)}" cy="0" r="{num(KEY / 2 + 0.5)}" fill="{{panel}}"/>')
        round_key(f, f'{name}_PARAM', cx, 0, symbol)

    # Beside the dials, where the OP-Z's speaker is: the display, its track keys in the OP-Z's order, the eight tracks,
    # the two FX tracks, then the master track.
    display(f, DISPLAY_LEFT, DISPLAY_TOP)

    # The dials, each with its LED in the corner of its cell.
    for d in range(4):
        left = COLUMN * (9 + 2 * d)
        f.place(f'DIAL{d}_WIDGET', left + COLUMN, COLUMN + 0.6)
        f.place(f'DIAL{d}_LIGHT', left + 1.6, CELL - 1.8)

    # The keys.
    for (column, y), (name, symbol) in {(1, TOP): ('TRACK', 'track'), (1, MID): ('REC', 'rec'), (1, BOTTOM): ('OCTAVE_DOWN', 'minus'),
                                        (2, MID): ('PLAY', 'play'), (2, BOTTOM): ('OCTAVE_UP', 'plus'),
                                        (3, MID): ('STOP', 'stop'), (3, BOTTOM): ('SHIFT', 'shift')}.items():
        round_key(f, f'{name}_PARAM', x(column), y, symbol)
    for i, track in enumerate(TRACKS):
        f.art.append(icon(track, x(i + 2), TOP - ICON, 2.2, '{ink}'))
        round_key(f, f'STEP{i}_PARAM', x(i + 2), TOP)
    for i, component in enumerate(COMPONENTS):
        f.art.append(icon(component, x(i + 4), BOTTOM - ICON, 2.2, '{ink}'))
        round_key(f, f'KEY{12 * (i // 7) + WHITE[i % 7]}_PARAM', x(i + 4), BOTTOM)
    lines = [4, 5, 6, 8, 9, 11, 12, 13, 15, 16]  # the column line each black key sits on, after that column
    for value, line in enumerate(lines):
        number_key(f, f'KEY{12 * (value // 5) + BLACK[value % 5]}_PARAM', COLUMN * line, MID, str((value + 1) % 10))


def panel():
    p = Panel(WIDTH, HEIGHT)
    p.rect(0, 0, WIDTH, HEIGHT, 0, 'panel')
    p.title('Sequencer')
    with Scaled(p, (WIDTH - SCALE * FACE_WIDTH) / 2, FACE_TOP) as f:
        face(f)

    def jack(name, kind, cx, label):
        y = JACK_ROW
        p.text(label, cx, y - 6.2, size=LABEL if len(label) < 5 else 2.3, fill='box_ink' if kind == 'OUTPUT' else 'ink')
        p.place(f'{name}_{kind}', cx, y)
        p.place(f'{name}_{kind}_LIGHT', cx + 3.9, y - 3.9)

    def box(x1, x2):
        """A dark box behind the outputs from column x1 to x2."""
        p.rect(x1 - PITCH / 2 + 0.6, JACK_ROW - 8.6, x2 - x1 + PITCH - 1.2, 15.0, 2.0, 'box')

    def columns(groups, left, right):
        """The columns' centres of groups of `n` jacks, spread from `left` to `right` with equal gaps between."""
        gap = (right - left - PITCH * sum(groups)) / (len(groups) - 1)
        out = []
        for n in groups:
            out.append([left + PITCH * (i + 0.5) for i in range(n)])
            left += PITCH * n + gap
        return out

    # One row: the transport's and the notes' inputs, then the transport's outputs and the mix in dark boxes.
    transport = ['CLOCK', 'RUN', 'RESET', 'CONT']
    ins, notes, outs, mix = columns([4, 4, 4, 2], MARGIN, WIDTH - MARGIN)
    box(outs[0], outs[-1])
    box(mix[0], mix[-1])
    for cx, name in zip(ins, transport):
        jack(name, 'INPUT', cx, name)
    for cx, (name, label) in zip(notes, [('VOCT', 'V/OCT'), ('GATE', 'GATE'), ('VELOCITY', 'VEL'), ('ADVANCE', 'ADV')]):
        jack(name, 'INPUT', cx, label)
    for cx, name in zip(outs, transport):
        jack(name, 'OUTPUT', cx, name)
    jack('LEFT', 'OUTPUT', mix[0], 'L')
    jack('RIGHT', 'OUTPUT', mix[1], 'R')
    return p
