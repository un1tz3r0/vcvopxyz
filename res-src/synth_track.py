"""The panel every OP-Z track shares, drum or synth, whatever its engine.

The engines have no dials of their own: the Sequencer's four turn them, and a Screen beside one shows every page.
So a track is a slim strip: a status screen (the Sequencer track it plays, its icon, a scope, its voices and level),
an LED for when the Sequencer has it selected and one for when it plays, then its jacks. Breakout and Drum Breakout
are wider, for the jacks that take their voice out to other modules and back. The FX modules have the same strip, with
the jacks of a send effect."""
from panelkit import HP, LABEL, Panel

HEIGHT = 128.5
SCREEN_TOP, SCREEN_HEIGHT = 12.6, 30.0
LED_ROW = 47.2
PITCH = 12.5                  # between the jacks' columns
LABELS = dict(VOCT='V/OCT', VELOCITY='VEL', LEFT='L', RIGHT='R', FX1='FX 1', FX2='FX 2', RETURN_L='RET L',
              RETURN_R='RET R', CV_VOCT='V/OCT', CV_GATE='GATE', CV_ENV='ENV', CV_LFO='LFO', CV_FILTER='FILTER',
              CV_RESONANCE='RESO', CV_FX1='FX 1', CV_FX2='FX 2', CV_P1='CV 1', CV_P2='CV 2', CV_TRIGGER='TRIG',
              CV_SOUND='SOUND', CV_VELOCITY='VEL')


def status(slug, hp, active='NOTE'):
    """The panel down to the LEDs: the title, the status screen, and SEL and NOTE (or `active`) under it."""
    width = hp * HP
    p = Panel(width, HEIGHT)
    p.rect(0, 0, width, HEIGHT, 0, 'panel')
    p.title(slug)
    p.rect(2.2, SCREEN_TOP, width - 4.4, SCREEN_HEIGHT, 1.5, 'box')
    p.place('STATUS_WIDGET', 2.2, SCREEN_TOP, width - 4.4, SCREEN_HEIGHT)
    for x, name, label in [(width / 2 - 6.0, 'SELECTED', 'SEL'), (width / 2 + 6.0, 'ACTIVE', active)]:
        p.place(f'{name}_LIGHT', x - 3.0, LED_ROW)
        p.text(label, x + 1.4, LED_ROW, size=2.2, anchor='middle')
    return p


def jacks(p, rows, columns, labels={}):
    """Rows of (y, [(name, kind) or None per column]). Each jack has a level LED; outputs print their labels light,
    for the dark boxes behind them. `labels` overrides LABELS."""
    for y, row in rows:
        for x, jack in zip(columns, row):
            if jack:
                name, kind = jack
                label = labels.get(name, LABELS.get(name, name))
                p.text(label, x, y - 6.2, size=LABEL if len(label) < 5 else 2.2, fill='box_ink' if kind == 'OUTPUT' else 'ink')
                p.place(f'{name}_{kind}', x, y)
                p.place(f'{name}_LIGHT', x + 3.9, y - 3.9)


def box(p, columns, first, last, top, bottom=None):
    """A dark box behind the outputs from column `first` to `last`, in the rows from `top` to `bottom`."""
    p.rect(columns[first] - 5.9, top - 8.4, columns[last] - columns[first] + 11.8, (bottom or top) - top + 14.8, 2.0, 'box')


def synth_track_panel(slug):
    """6 HP: the inputs in three rows of two, the outputs in a dark box below."""
    p = status(slug, 6)
    columns = [3 * HP - PITCH / 2, 3 * HP + PITCH / 2]
    inputs = [('VOCT', 'GATE'), ('VELOCITY', 'CLOCK'), ('RESET', 'EXT')]
    outputs = [('LEFT', 'RIGHT'), ('FX1', 'FX2')]
    rows = [59.5, 72.5, 85.5, 102.0, 115.0]
    box(p, columns, 0, 1, rows[3], rows[4])
    jacks(p, [(y, [(n, 'INPUT') for n in names]) for y, names in zip(rows, inputs)]
          + [(y, [(n, 'OUTPUT') for n in names]) for y, names in zip(rows[3:], outputs)], columns)
    return p


def fx_track_panel(slug):
    """6 HP: IN for use off the bus, the notes that set the time, the LFO's clock, reset and EXT, then the outputs
    in a dark box."""
    p = status(slug, 6, active='OUT')
    columns = [3 * HP - PITCH / 2, 3 * HP + PITCH / 2]
    inputs = [('RETURN_L', 'RETURN_R'), ('VOCT', 'GATE'), ('CLOCK', 'RESET'), ('EXT', None)]
    rows = [59.0, 71.5, 84.0, 96.5, 113.0]
    box(p, columns, 0, 1, rows[4])
    jacks(p, [(y, [(n, 'INPUT') if n else None for n in names]) for y, names in zip(rows, inputs)]
          + [(rows[4], [('LEFT', 'OUTPUT'), ('RIGHT', 'OUTPUT')])], columns, labels=dict(RETURN_L='IN L', RETURN_R='IN R'))
    return p


def breakout_panel():
    """12 HP: the voice going out in a dark box, the notes coming in with the return, then the outputs."""
    p = status('Breakout', 12)
    columns = [6 * HP + 11.2 * (i - 2) for i in range(5)]
    rows = [59.5, 72.5, 87.5, 100.5, 115.0]
    box(p, columns, 0, 4, rows[0], rows[1])
    box(p, columns, 1, 4, rows[4])
    out = lambda *names: [(n, 'OUTPUT') for n in names]
    jacks(p, [(rows[0], out('CV_VOCT', 'CV_GATE', 'CV_ENV', 'CV_LFO', 'CV_P1')),
              (rows[1], out('CV_FILTER', 'CV_RESONANCE', 'CV_FX1', 'CV_FX2', 'CV_P2')),
              (rows[2], [(n, 'INPUT') for n in ['VOCT', 'GATE', 'VELOCITY', 'CLOCK', 'RESET']]),
              (rows[3], [('EXT', 'INPUT'), ('RETURN_L', 'INPUT'), ('RETURN_R', 'INPUT')]),
              (rows[4], [None] + out('LEFT', 'RIGHT', 'FX1', 'FX2'))], columns)
    return p


def drum_breakout_panel():
    """14 HP: Breakout's layout, six jacks wide for the drum track's extra outputs."""
    p = status('DrumBreakout', 14)
    columns = [7 * HP + 10.8 * (i - 2.5) for i in range(6)]
    rows = [59.5, 72.5, 89.5, 107.0]
    box(p, columns, 0, 5, rows[0], rows[1])
    box(p, columns, 2, 5, rows[3])
    out = lambda *names: [(n, 'OUTPUT') for n in names]
    jacks(p, [(rows[0], out('CV_TRIGGER', 'CV_GATE', 'CV_SOUND', 'CV_VELOCITY', 'CV_ENV', 'CV_LFO')),
              (rows[1], out('CV_P1', 'CV_P2', 'CV_FILTER', 'CV_RESONANCE', 'CV_FX1', 'CV_FX2')),
              (rows[2], [(n, 'INPUT') for n in ['VOCT', 'GATE', 'VELOCITY', 'CLOCK', 'RESET', 'EXT']]),
              (rows[3], [('RETURN_L', 'INPUT'), ('RETURN_R', 'INPUT')] + out('LEFT', 'RIGHT', 'FX1', 'FX2'))],
          columns, labels=dict(CV_P1='PITCH', CV_P2='REV'))
    return p
