"""The panel every OP-Z synth track shares, whatever its engine.

Like the hardware, it has four dials whose meaning changes with the page: PAGE steps through the pages, and the
legend below the dials says what each dial does on each one. Then the voice LEDs and the jacks; the notes come from a
Sequencer, which has the keys. Only the names of the two sound dials change from engine to engine, except on
Breakout, which is wider for the jacks that take its voice out to other modules and back."""
from panelkit import HP, LABEL, Panel

HEIGHT = 128.5
LEFT = 7.0                    # the column of page controls
LED_ROW, DIAL_ROW = 14.0, 24.5
LEGEND = [37.5 + 5.0 * i for i in range(5)]
JACK_ROWS = [69.5, 82.5, 97.5, 112.5]
PITCH = 15.5                  # between the jacks' columns
SMALL = 2.0                   # legend font size (mm)
LABELS = dict(VOCT='V/OCT', VELOCITY='VEL', LEFT='L', RIGHT='R', FX1='FX 1', FX2='FX 2', RETURN_L='RET L',
              RETURN_R='RET R', CV_VOCT='V/OCT', CV_GATE='GATE', CV_ENV='ENV', CV_LFO='LFO', CV_FILTER='FILTER',
              CV_RESONANCE='RESO', CV_FX1='FX 1', CV_FX2='FX 2', CV_P1='CV 1', CV_P2='CV 2')


def dials_and_legend(slug, p1, p2, hp):
    """The panel down to the legend: PAGE, the four dials and what they do on every page."""
    width = hp * HP
    p = Panel(width, HEIGHT)
    p.rect(0, 0, width, HEIGHT, 0, 'panel')
    p.title(slug)
    dials = [14.0 + (width - 17.0) * (i + 0.5) / 4 for i in range(4)]

    # PAGE and its LED, level with the dials and theirs.
    p.place('PAGE_LIGHT', LEFT, LED_ROW)
    p.place('PAGE_PARAM', LEFT, DIAL_ROW)
    p.text('PAGE', LEFT, DIAL_ROW + 5.6, size=SMALL)
    for i, x in enumerate(dials):
        p.place(f'DIAL{i}_LIGHT', x, LED_ROW)
        p.place(f'DIAL{i}_WIDGET', x, DIAL_ROW)

    # The legend: each page's name, which selects it, over a bar in its colour, then its four dials.
    pages = [('SOUND', [p1, p2, 'FILTER', 'RESO']),
             ('ENV', ['ATTACK', 'DECAY', 'SUSTAIN', 'RELEASE']),
             ('LFO', ['AMOUNT', 'SPEED', 'TARGET', 'SHAPE']),
             ('MIX', ['FX 1', 'FX 2', 'PAN', 'LEVEL']),
             ('TRACK', ['', 'STYLE', '', 'GLIDE'])]
    p.line(3.0, LEGEND[0] - 4.0, width - 3.0, LEGEND[0] - 4.0)
    p.line(3.0, LEGEND[-1] - 2.5, width - 3.0, LEGEND[-1] - 2.5)
    for i, (y, (page, names)) in enumerate(zip(LEGEND, pages)):
        p.text(page, LEFT, y, size=SMALL)
        p.place(f'SELECT{i}_LIGHT', LEFT - 3.5, y + 1.6, 7.0, 0.6)
        p.place(f'SELECT{i}_PARAM', LEFT - 5.0, y - 2.0, 10.0, 4.3)
        for x, name in zip(dials, names):
            if name:
                p.text(name, x, y, size=SMALL)
    p.line(3.0, LEGEND[-1] + 3.5, width - 3.0, LEGEND[-1] + 3.5)
    return p


def voices(p, cx, cy, pitch, boxed=True):
    """The voice LEDs, two rows of eight, in a box like the Sequencer's channel LEDs."""
    w = 8 * pitch + 6.8
    if boxed:
        p.rect(cx - w / 2, cy - 7.5, w, 15.0, 1.5, 'box')
    p.text('VOICES', cx, cy - 3.6, size=SMALL, fill='box_ink')
    for i in range(16):
        p.place(f'VOICE{i}_LIGHT', cx + pitch * (i % 8 - 3.5), cy + 1.2 + 3.6 * (i // 8))


def jacks(p, rows, columns):
    """Rows of (y, [(name, kind) or None per column]). Each jack has a level LED; outputs print their labels light,
    for the dark boxes behind them."""
    for y, row in rows:
        for x, jack in zip(columns, row):
            if jack:
                name, kind = jack
                p.text(LABELS.get(name, name), x, y - 6.2, size=LABEL if len(LABELS.get(name, name)) < 6 else 2.2,
                       fill='box_ink' if kind == 'OUTPUT' else 'ink')
                p.place(f'{name}_{kind}', x, y)
                p.place(f'{name}_LIGHT', x + 3.9, y - 3.9)


def box(p, columns, first, last, top, bottom=None):
    """A dark box behind the outputs from column `first` to `last`, in the rows from `top` to `bottom`."""
    p.rect(columns[first] - 6.9, top - 8.4, columns[last] - columns[first] + 13.8, (bottom or top) - top + 14.8, 2.0, 'box')


def synth_track_panel(slug, p1, p2):
    p = dials_and_legend(slug, p1, p2, 16)
    width = 16 * HP
    voices(p, width / 2, 75.0, 4.4)
    # Inputs in the top row; EXT, then the outputs in their dark box, in the bottom one.
    columns = [width / 2 + PITCH * (i - 2) for i in range(5)]
    _, _, top, bottom = JACK_ROWS
    box(p, columns, 1, 4, bottom)
    jacks(p, [(top, [(n, 'INPUT') for n in ['VOCT', 'GATE', 'VELOCITY', 'CLOCK', 'RESET']]),
              (bottom, [('EXT', 'INPUT')] + [(n, 'OUTPUT') for n in ['LEFT', 'RIGHT', 'FX1', 'FX2']])], columns)
    return p


def breakout_panel():
    """Four rows of six jacks: the voice going out, in one dark box with the voice LEDs; the notes coming in; the
    return and the outputs."""
    p = dials_and_legend('Breakout', 'CV 1', 'CV 2', 20)
    width = 20 * HP
    columns = [width / 2 + PITCH * (i - 2.5) for i in range(6)]
    first, second, third, fourth = JACK_ROWS
    box(p, columns, 0, 5, first, second)
    box(p, columns, 2, 5, fourth)
    voices(p, (columns[4] + columns[5]) / 2, second - 1.2, 3.4, boxed=False)
    out = lambda *names: [(n, 'OUTPUT') for n in names]
    jacks(p, [(first, out('CV_VOCT', 'CV_GATE', 'CV_ENV', 'CV_LFO', 'CV_P1', 'CV_P2')),
              (second, out('CV_FILTER', 'CV_RESONANCE', 'CV_FX1', 'CV_FX2')),
              (third, [(n, 'INPUT') for n in ['VOCT', 'GATE', 'VELOCITY', 'CLOCK', 'RESET', 'EXT']]),
              (fourth, [('RETURN_L', 'INPUT'), ('RETURN_R', 'INPUT')] + out('LEFT', 'RIGHT', 'FX1', 'FX2'))], columns)
    return p
