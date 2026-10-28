"""Song Editor: a big screen for the Sequencer's songs, on its left. MODE, SONG and ROW sit down the screen's right
side, and the PAT, LEN and MUTE wheels under the columns they turn; a line runs from each control to the edge of the
screen beside what it changes. INSERT and DELETE take the row selected."""
from panelkit import HP, Panel

WIDTH, HEIGHT = 24 * HP, 128.5
SCREEN = (3.0, 11.0, 95.0, 93.0)        # x, y, width, height of the bezel
SIDE = 110.0                            # the right-hand controls' column
COLUMNS = dict(PATTERN=31.0, LENGTH=53.0, MUTES=75.0)
LABELS = dict(PATTERN='PAT', LENGTH='LEN', MUTES='MUTE')
SIDE_ROWS = dict(MODE=19.0, SONG=36.0, ROW=76.0)
WHEEL_ROW = 115.5


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
    for name, cx in COLUMNS.items():
        p.line(cx, y + h, cx, WHEEL_ROW - 5.6)
        p.text(LABELS[name], cx - 8.0, WHEEL_ROW + 0.8, size=2.4)
        p.place(f'{name}_WHEEL_WIDGET', cx, WHEEL_ROW)
    for name, cx in [('INSERT', 103.0), ('DELETE', 115.0)]:
        p.text(name, cx, WHEEL_ROW - 4.6, size=2.0)
        p.place(f'{name}_PARAM', cx, WHEEL_ROW)
    return p
