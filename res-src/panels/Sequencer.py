"""The sequencer: the OP-Z's face, and the master of the engines to its right. Under the face, the keys that turn the
page over two rows of jacks, the transport in and out and the notes to record; a named key per track, and one for the
master track; and the engines' mix.

The face copies the OP-Z's: four dials over 17 columns of round keys. TRACK, REC and −, PLAY and +, STOP and SHIFT
on the left; the steps along the top under the track icons; the numbered black keys on the column lines; the white
keys along the bottom under the step component icons. The speaker becomes the tempo display and the channel LEDs.
It is drawn at the size of the 24 HP panel it was first made for, then scaled up to fill the space above the jacks."""
from opz import COMPONENTS, SCALE, TRACKS, icon, number_key, round_key
from panelkit import HP, LABEL, NUNITO, Panel, num

WIDTH, HEIGHT = 40 * HP, 128.5
COLUMN = (24 * HP - 6.0) / 17                # the face's key columns
CELL, ROW = 2 * COLUMN + 4.5, 7.2            # the height of the dials' row, and the keys' row pitch
FACE_WIDTH, FACE_HEIGHT = 17 * COLUMN, CELL + 7.6 + 2 * ROW + 4.0
TOP, MID, BOTTOM = CELL + 7.6, CELL + 7.6 + ROW, CELL + 7.6 + 2 * ROW
ICON = 4.1                                   # how far an icon sits above its key
BLACK = [1, 3, 5, 8, 10]                     # the black keys' semitones above F
WHITE = [0, 2, 4, 6, 7, 9, 11]

KEY_ROW, JACK_ROWS = 91.0, (104.5, 117.5)
PITCH, GAP = 11.6, 4.0                       # between the jacks' columns, and between their groups
KEY_PITCH = 10.4                             # between the track keys


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


def face(f):
    # The outline, the line between the dials and the keys, and the lines between the columns.
    f.art.append(f'<rect x="0" y="0" width="{num(FACE_WIDTH)}" height="{num(FACE_HEIGHT)}" rx="1.2" fill="none" '
                 f'stroke="{{line}}" stroke-width="0.25"/>')
    f.line(0, CELL, FACE_WIDTH, CELL)
    for k in range(1, 17):
        f.line(COLUMN * k, CELL, COLUMN * k, FACE_HEIGHT)
    for k in (9, 11, 13, 15):
        f.line(COLUMN * k, 0, COLUMN * k, CELL)

    # Along the top, the four index keys: project, mixer, tempo and screen. Under them, where the OP-Z's speaker is,
    # the tempo display, INT and EXT, and the channel LEDs as its grille.
    for i, (name, symbol) in enumerate([('PROJECT', 'project'), ('MIXER', 'mixer'), ('METRONOME', 'metronome'), ('SCREEN', 'screen')]):
        round_key(f, f'{name}_PARAM', 9.0 + 13.5 * i, 3.6, symbol)
    display = 7.6                            # the display box's top
    f.art.append(f'<rect x="3" y="{num(display)}" width="25" height="{num(CELL - display - 1.4)}" rx="1.2" fill="{{box}}"/>')
    f.place('TEMPO_WIDGET', 3.6, display + 0.6, 14.4, CELL - display - 2.6)
    for y, name, label in [(display + 2.6, 'INTERNAL', 'INT'), (CELL - 4.0, 'EXTERNAL', 'EXT')]:
        f.place(f'{name}_LIGHT', 20.0, y)
        f.art.append(f'<path d="{NUNITO.path(label, 24.2, y, 1.8)}" fill="{{box_ink}}"/>')
    for i in range(16):
        f.place(f'CHANNEL{i}_LIGHT', 44.4 + 2.6 * (i % 8 - 3.5), (display + CELL - 1.4) / 2 + 2.6 * (i // 8 - 0.5))

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


def key(p, name, cx, cy, symbol='dot'):
    """A round key under the face, the size of the face's."""
    with Scaled(p, cx, cy) as k:
        round_key(k, name, 0, 0, symbol)


def panel():
    p = Panel(WIDTH, HEIGHT)
    p.rect(0, 0, WIDTH, HEIGHT, 0, 'panel')
    p.title('Sequencer')
    with Scaled(p, (WIDTH - SCALE * FACE_WIDTH) / 2, 10.5) as f:
        face(f)

    # Four groups of columns: the transport's jacks, the notes to record, the track keys and the master's, and the mix.
    groups = [(4, PITCH), (2, PITCH), (9, KEY_PITCH), (1, PITCH)]
    left = (WIDTH - sum(n * pitch for n, pitch in groups) - GAP * (len(groups) - 1)) / 2
    columns = []
    for n, pitch in groups:
        columns.append([left + pitch * (i + 0.5) for i in range(n)])
        left += n * pitch + GAP
    transport, notes, tracks, mix = columns
    top, bottom = JACK_ROWS

    # The four page keys over the transport and the notes, and the track keys level with the top row of jacks, named,
    # the master track's last.
    span = notes[-1] - transport[0]
    for i in range(4):
        key(p, f'PAGE{i}_PARAM', transport[0] + span * (i + 0.5) / 4, KEY_ROW)
    for t, cx in enumerate(tracks):
        track = TRACKS[t] if t < 8 else 'master'
        key(p, f'SELECT{t}_PARAM' if t < 8 else 'MASTER_PARAM', cx, top, track)
        p.text(track.upper(), cx, bottom - 6.2, size=LABEL if len(track) < 5 else 2.3)

    # Outputs sit in dark boxes: the transport's along the bottom row, and the mix in both.
    def box(x1, x2, y1, y2):
        p.rect(x1 - PITCH / 2 + 0.6, y1 - 8.6, x2 - x1 + PITCH - 1.2, y2 - y1 + 15.0, 2.0, 'box')
    box(transport[0], transport[-1], bottom, bottom)
    box(mix[0], mix[0], top, bottom)

    def jack(name, kind, cx, y, label):
        p.text(label, cx, y - 6.2, size=LABEL if len(label) < 5 else 2.3, fill='box_ink' if kind == 'OUTPUT' else 'ink')
        p.place(f'{name}_{kind}', cx, y)
        p.place(f'{name}_{kind}_LIGHT', cx + 3.9, y - 3.9)

    for cx, name in zip(transport, ['CLOCK', 'RUN', 'RESET', 'CONT']):
        jack(name, 'INPUT', cx, top, name)
        jack(name, 'OUTPUT', cx, bottom, name)
    for (cx, y), name, label in zip([(notes[0], top), (notes[1], top), (notes[0], bottom), (notes[1], bottom)],
                                    ['VOCT', 'GATE', 'VELOCITY', 'ADVANCE'], ['V/OCT', 'GATE', 'VEL', 'ADV']):
        jack(name, 'INPUT', cx, y, label)
    jack('LEFT', 'OUTPUT', mix[0], top, 'L')
    jack('RIGHT', 'OUTPUT', mix[0], bottom, 'R')
    return p
