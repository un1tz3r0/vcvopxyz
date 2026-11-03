"""Song Editor: a big screen for the Sequencer's songs, on its left. MODE, SONG and ROW sit down the screen's right
side (ROW the biggest, with no label), and the PAT, LEN and MUTE wheels under the columns they turn; a line runs from each control to the edge of the
screen beside what it changes. Round the ROW wheel, OP-Z keys take the row selected: + copies it, - deletes it, and the
arrows move it."""
from opz import KEY, RING, SCALE
from panelkit import HP, Panel, num

WIDTH, HEIGHT = 24 * HP, 128.5
SCREEN = (3.0, 11.0, 95.0, 93.0)        # x, y, width, height of the bezel
SIDE = 110.0                            # the right-hand controls' column
COLUMNS = dict(PATTERN=31.0, LENGTH=53.0, MUTES=75.0)
LABELS = dict(PATTERN='PAT', LENGTH='LEN', MUTES='MUTE')
SIDE_ROWS = dict(MODE=17.0, SONG=29.5)
ROW, ROW_WHEEL = 70.0, 13.0             # the ROW wheel, bigger than the rest, unlabelled between its keys
WHEEL_ROW = 115.5
ROW_KEYS = dict(INSERT=43.0, UP=55.5, DOWN=84.5, DELETE=97.0) # keys above and below ROW, at the Sequencer's key size


def panel():
    p = Panel(WIDTH, HEIGHT)
    p.rect(0, 0, WIDTH, HEIGHT, 0, 'panel')
    p.title('SongEditor')
    x, y, w, h = SCREEN
    p.rect(x, y, w, h, 3.0, 'bezel')
    p.rect(x + 1.0, y + 1.0, w - 2.0, h - 2.0, 2.4, 'glass')
    p.place('SCREEN_WIDGET', x + 1.0, y + 1.0, w - 2.0, h - 2.0)
    for name, cy in SIDE_ROWS.items():
        p.line(x + w, cy, SIDE - 5.6, cy)
        p.text(name, SIDE, cy - 6.4, size=2.4)
        p.place(f'{name}_{"PARAM" if name == "MODE" else "WHEEL_WIDGET"}', SIDE, cy)
    p.line(x + w, ROW, SIDE - ROW_WHEEL / 2 - 1.1, ROW)
    p.place('ROW_WHEEL_WIDGET', SIDE - ROW_WHEEL / 2, ROW - ROW_WHEEL / 2, ROW_WHEEL, ROW_WHEEL)
    for name, cx in COLUMNS.items():
        p.line(cx, y + h, cx, WHEEL_ROW - 5.6)
        p.text(LABELS[name], cx - 9.5, WHEEL_ROW + 0.8, size=2.4)
        p.place(f'{name}_WHEEL_WIDGET', cx, WHEEL_ROW)
    d = KEY * SCALE
    for name, cy in ROW_KEYS.items():
        p.art.append(f'<circle cx="{num(SIDE)}" cy="{num(cy)}" r="{num((d - RING * SCALE) / 2)}" fill="none" '
                     f'stroke="{{ink}}" stroke-width="{num(RING * SCALE)}"/>')
        p.place(f'{name}_PARAM', SIDE - d / 2, cy - d / 2, d, d)
    return p
