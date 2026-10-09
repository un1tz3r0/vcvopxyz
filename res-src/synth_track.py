"""The panel every OP-Z synth track shares, whatever its engine: one row of four dials per page of the hardware's
dials (sound, envelope, LFO, mix), then the jacks. Only the two sound dials' names change from engine to engine."""
from panelkit import HP, Panel

WIDTH, HEIGHT = 14 * HP, 128.5
COLUMNS = [WIDTH * (i + 0.5) / 4 for i in range(4)]
ROWS = [21.0, 39.0, 57.0, 75.0]


def synth_track_panel(slug, p1, p2):
    p = Panel(WIDTH, HEIGHT)
    p.rect(0, 0, WIDTH, HEIGHT, 0, 'panel')
    p.title(slug)

    pages = [
        [('P1', p1), ('P2', p2), ('FILTER', 'FILTER'), ('RESONANCE', 'RESO')],
        [('ATTACK', 'ATTACK'), ('DECAY', 'DECAY'), ('SUSTAIN', 'SUSTAIN'), ('RELEASE', 'RELEASE')],
        [('LFO_AMOUNT', 'AMOUNT'), ('LFO_SPEED', 'SPEED'), ('LFO_TARGET', 'TARGET'), ('LFO_SHAPE', 'SHAPE')],
        [('STYLE', 'STYLE'), ('GLIDE', 'GLIDE'), ('PAN', 'PAN'), ('LEVEL', 'LEVEL')],
    ]
    for y, dials in zip(ROWS, pages):
        for x, (name, label) in zip(COLUMNS, dials):
            p.text(label, x, y - 8.5)
            p.place(f'{name}_PARAM', x, y)
    for y in ROWS[1:]:
        p.line(3.0, y - 12.0, WIDTH - 3.0, y - 12.0)

    # Inputs in two rows on the left, the outputs in their dark box on the right.
    top, bottom = 95.0, 112.5
    p.line(3.0, top - 12.0, WIDTH - 3.0, top - 12.0)
    for y, jacks in [(top, ['VOCT', 'GATE', 'VELOCITY']), (bottom, ['CLOCK', 'RESET', 'EXT'])]:
        for x, name in zip(COLUMNS, jacks):
            p.text({'VOCT': 'V/OCT', 'VELOCITY': 'VEL'}.get(name, name), x, y - 7.0)
            p.place(f'{name}_INPUT', x, y)
    bx, bw = COLUMNS[3] - 7.0, 14.0
    p.rect(bx, top - 10.5, bw, bottom - top + 15.5, 2.0, 'box')
    for y, name, label in [(top, 'LEFT', 'L'), (bottom, 'RIGHT', 'R')]:
        p.text(label, COLUMNS[3], y - 7.0, fill='box_ink')
        p.place(f'{name}_OUTPUT', COLUMNS[3], y)
    return p
