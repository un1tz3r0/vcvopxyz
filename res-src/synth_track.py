"""The panel every OP-Z synth track shares, whatever its engine.

Like the hardware, it has four dials whose meaning changes with the page: PAGE steps through the pages, and the
legend below the dials says what each dial does on each one. Then a two-octave keyboard, whose white keys hold the
quick-save slots in track mode, the voice LEDs, and the jacks. Only the names of the two sound dials change from
engine to engine."""
from panelkit import FUNCTION_KEY, HP, LABEL, Panel

WIDTH, HEIGHT = 16 * HP, 128.5
LEFT = 7.0                                                         # the column of page controls
DIALS = [14.0 + (WIDTH - 17.0) * (i + 0.5) / 4 for i in range(4)]  # the four dials, right of it
LED_ROW, DIAL_ROW = 14.0, 24.5
LEGEND = [37.5 + 5.0 * i for i in range(5)]
VOICE_ROW, BLACK_ROW, WHITE_ROW = 65.5, 72.6, 81.4
JACK_ROWS = [97.5, 112.5]
JACKS = [WIDTH / 2 + 15.5 * (i - 2) for i in range(5)]
SMALL = 2.0  # legend font size (mm)


def synth_track_panel(slug, p1, p2):
    p = Panel(WIDTH, HEIGHT)
    p.rect(0, 0, WIDTH, HEIGHT, 0, 'panel')
    p.title(slug)

    # PAGE and its LED, level with the dials and theirs.
    p.place('PAGE_LIGHT', LEFT, LED_ROW)
    p.place('PAGE_PARAM', LEFT, DIAL_ROW)
    p.text('PAGE', LEFT, DIAL_ROW + 5.6, size=SMALL)
    for i, x in enumerate(DIALS):
        p.place(f'DIAL{i}_LIGHT', x, LED_ROW)
        p.place(f'DIAL{i}_WIDGET', x, DIAL_ROW)

    # The legend: each page's name, which selects it, over a bar in its colour, then its four dials.
    pages = [('SOUND', [p1, p2, 'FILTER', 'RESO']),
             ('ENV', ['ATTACK', 'DECAY', 'SUSTAIN', 'RELEASE']),
             ('LFO', ['AMOUNT', 'SPEED', 'TARGET', 'SHAPE']),
             ('MIX', ['FX 1', 'FX 2', 'PAN', 'LEVEL']),
             ('TRACK', ['', 'STYLE', '', 'GLIDE'])]
    p.line(3.0, LEGEND[0] - 4.0, WIDTH - 3.0, LEGEND[0] - 4.0)
    p.line(3.0, LEGEND[-1] - 2.5, WIDTH - 3.0, LEGEND[-1] - 2.5)
    for i, (y, (page, dials)) in enumerate(zip(LEGEND, pages)):
        p.text(page, LEFT, y, size=SMALL)
        p.place(f'SELECT{i}_LIGHT', LEFT - 3.5, y + 1.6, 7.0, 0.6)
        p.place(f'SELECT{i}_PARAM', LEFT - 5.0, y - 2.0, 10.0, 4.3)
        for x, name in zip(DIALS, dials):
            if name:
                p.text(name, x, y, size=SMALL)
    p.line(3.0, LEGEND[-1] + 3.5, WIDTH - 3.0, LEGEND[-1] + 3.5)

    # Voice LEDs between the octave keys, then two octaves of keys: the black ones in a row above the white.
    p.place('OCTAVE_DOWN_PARAM', 3.0 + FUNCTION_KEY[0] / 2, VOICE_ROW)
    p.place('OCTAVE_UP_PARAM', WIDTH - 3.0 - FUNCTION_KEY[0] / 2, VOICE_ROW)
    for i in range(16):
        p.place(f'VOICE{i}_LIGHT', WIDTH / 2 + 3.4 * (i - 7.5), VOICE_ROW)
    pitch = (WIDTH - 6.0) / 14
    white = [0, 2, 4, 5, 7, 9, 11]
    for octave in range(2):
        for i, semitone in enumerate(white):
            x = 3.0 + pitch * (7 * octave + i + 0.5)
            p.place(f'KEY{12 * octave + semitone}_PARAM', x, WHITE_ROW)
            if semitone not in (4, 11):  # a black key between this white key and the next
                p.place(f'KEY{12 * octave + semitone + 1}_PARAM', x + pitch / 2, BLACK_ROW)

    # Inputs in the top row; EXT, then the outputs in their dark box, in the bottom one. Each jack has a level LED.
    top, bottom = JACK_ROWS
    box = JACKS[1] - 6.9, JACKS[4] + 6.9
    p.rect(box[0], bottom - 8.6, box[1] - box[0], 15.0, 2.0, 'box')
    jacks = [(top, ['VOCT', 'GATE', 'VELOCITY', 'CLOCK', 'RESET'], 'INPUT'),
             (bottom, ['EXT'], 'INPUT'),
             (bottom, [None, 'LEFT', 'RIGHT', 'FX1', 'FX2'], 'OUTPUT')]
    labels = dict(VOCT='V/OCT', VELOCITY='VEL', LEFT='L', RIGHT='R', FX1='FX 1', FX2='FX 2')
    for y, names, kind in jacks:
        for x, name in zip(JACKS, names):
            if name:
                p.text(labels.get(name, name), x, y - 6.2, size=LABEL, fill='box_ink' if kind == 'OUTPUT' else 'ink')
                p.place(f'{name}_{kind}', x, y)
                p.place(f'{name}_LIGHT', x + 3.9, y - 3.9)
    return p
