"""The sequencer: an OP-Z synth track's 16 steps, the keyboard that fills them, the transport, and the tempo, which
moves to a master module once there is one.

Its face copies the OP-Z's: four dials over 17 columns of round keys. TRACK, REC and −, PLAY and +, STOP and SHIFT
on the left; the steps along the top under the track icons; the numbered black keys on the column lines; the white
keys along the bottom under the step component icons. The speaker becomes the tempo display and the channel LEDs."""
from opz import COMPONENTS, KEY, TRACKS, icon, number_key, round_key
from panelkit import DIALS, HP, LABEL, Panel, num
from synth_track import JACK_ROWS

WIDTH, HEIGHT = 24 * HP, 128.5
SMALL = 2.0
COLUMN = (WIDTH - 6.0) / 17
CELL, ROW = 2 * COLUMN + 4.5, 7.2            # the height of the dials' row, and the keys' row pitch
HEIGHT_OF_FACE = CELL + 7.6 + 2 * ROW + 4.0
FACE = (9.5 + 89.3 - HEIGHT_OF_FACE) / 2     # centred between the title and the jacks' labels
KEYS = FACE + CELL                           # the key block's top
TOP, MID, BOTTOM = KEYS + 7.6, KEYS + 7.6 + ROW, KEYS + 7.6 + 2 * ROW
FACE_END = FACE + HEIGHT_OF_FACE
ICON = 4.1                                   # how far an icon sits above its key
JACKS = [WIDTH / 2 + 15.5 * (i - 2.5) for i in range(6)]
BLACK = [1, 3, 5, 8, 10]                     # the black keys' semitones above F
WHITE = [0, 2, 4, 6, 7, 9, 11]


def x(column):
    """The centre of a column, 1 to 17."""
    return 3.0 + COLUMN * (column - 0.5)


def panel():
    p = Panel(WIDTH, HEIGHT)
    p.rect(0, 0, WIDTH, HEIGHT, 0, 'panel')
    p.title('Sequencer')

    # The face's outline, the line between the dials and the keys, and the lines between the columns.
    right = WIDTH - 3.0
    p.art.append(f'<rect x="3" y="{num(FACE)}" width="{num(right - 3.0)}" height="{num(FACE_END - FACE)}" rx="1.2" '
                 f'fill="none" stroke="{{line}}" stroke-width="0.35"/>')
    p.line(3.0, KEYS, right, KEYS)
    for k in range(1, 17):
        p.line(3.0 + COLUMN * k, KEYS, 3.0 + COLUMN * k, FACE_END)
    for k in (9, 11, 13, 15):
        p.line(3.0 + COLUMN * k, FACE, 3.0 + COLUMN * k, KEYS)

    # Where the OP-Z's speaker is: the tempo display, INT and EXT, and the channel LEDs as its grille.
    p.rect(6.0, FACE + 3.5, 31.0, CELL - 7.0, 1.5, 'box')
    p.place('TEMPO_WIDGET', 7.0, FACE + 4.5, 20.5, CELL - 9.0)
    for y, name, label in [(FACE + CELL / 2 - 2.6, 'INTERNAL', 'INT'), (FACE + CELL / 2 + 2.6, 'EXTERNAL', 'EXT')]:
        p.place(f'{name}_LIGHT', 29.6, y)
        p.text(label, 33.6, y, size=1.8, fill='box_ink')
    for i in range(16):
        p.place(f'CHANNEL{i}_LIGHT', 50.7 + 2.6 * (i % 8 - 3.5), FACE + CELL / 2 + 2.6 * (i // 8 - 0.5))

    # The dials, each with its LED in the corner of its cell.
    for d, (name, label, color) in enumerate(zip(['TEMPO', 'SWING', 'LENGTH', 'QUANTIZE'], ['TEMPO', 'SWING', 'LENGTH', 'QUANT'], DIALS)):
        left = 3.0 + COLUMN * (9 + 2 * d)
        p.place(f'{name}_PARAM', left + COLUMN, FACE + COLUMN + 0.6)
        p.place(f'{name}_LIGHT', left + 1.4, FACE + CELL - 2.0)
        p.text(label, left + COLUMN, FACE + CELL - 2.0, size=SMALL)

    # The keys.
    for (column, y), (name, symbol) in {(1, TOP): ('TRACK', 'track'), (1, MID): ('REC', 'rec'), (1, BOTTOM): ('OCTAVE_DOWN', 'minus'),
                                        (2, MID): ('PLAY', 'play'), (2, BOTTOM): ('OCTAVE_UP', 'plus'),
                                        (3, MID): ('STOP', 'stop'), (3, BOTTOM): ('SHIFT', 'shift')}.items():
        round_key(p, f'{name}_PARAM', x(column), y, symbol)
    for i, track in enumerate(TRACKS):
        p.art.append(icon(track, x(i + 2), TOP - ICON, 2.2, '{ink}'))
        round_key(p, f'STEP{i}_PARAM', x(i + 2), TOP)
    for i, component in enumerate(COMPONENTS):
        p.art.append(icon(component, x(i + 4), BOTTOM - ICON, 2.2, '{ink}'))
        round_key(p, f'KEY{12 * (i // 7) + WHITE[i % 7]}_PARAM', x(i + 4), BOTTOM)
    lines = [4, 5, 6, 8, 9, 11, 12, 13, 15, 16]  # the column line each black key sits on, after that column
    for value, line in enumerate(lines):
        number_key(p, f'KEY{12 * (value // 5) + BLACK[value % 5]}_PARAM', 3.0 + COLUMN * line, MID, str((value + 1) % 10))

    # Inputs on the left: the clock's, then the notes to record. The outputs on the right, in their dark box.
    top, bottom = JACK_ROWS
    box = JACKS[3] - 6.9, JACKS[5] + 6.9
    p.rect(box[0], top - 8.6, box[1] - box[0], bottom - top + 15.0, 2.0, 'box')
    rows = [(top, ['CLOCK', 'RESET', 'ADVANCE'], ['CLOCK', 'RESET', None]),
            (bottom, ['VOCT', 'GATE', 'VELOCITY'], ['VOCT', 'GATE', 'VELOCITY'])]
    labels = dict(VOCT='V/OCT', VELOCITY='VEL', ADVANCE='ADV')
    for y, ins, outs in rows:
        for jx, name, kind in zip(JACKS, ins + outs, ['INPUT'] * 3 + ['OUTPUT'] * 3):
            if name:
                p.text(labels.get(name, name), jx, y - 6.2, size=LABEL, fill='box_ink' if kind == 'OUTPUT' else 'ink')
                p.place(f'{name}_{kind}', jx, y)
                p.place(f'{name}_{kind}_LIGHT', jx + 3.9, y - 3.9)
    return p
