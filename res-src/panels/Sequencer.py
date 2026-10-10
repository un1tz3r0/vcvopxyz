"""The sequencer: an OP-Z synth track's 16 steps, the keyboard that fills them, the transport, and the tempo, which
moves to a master module once there is one. Its keyboard and jacks sit exactly where an engine's do, so the two line
up side by side."""
from panelkit import FUNCTION_KEY, HP, LABEL, Panel
from synth_track import BLACK_ROW, JACK_ROWS, VOICE_ROW, WHITE_ROW

WIDTH, HEIGHT = 16 * HP, 128.5
SMALL = 2.0
KNOB_ROW, KNOBS = 17.5, [42.5, 52.5, 64.0, 74.0]
LED_ROW, FUNCTION_ROW = 33.8, 38.4
STEP_ROW = 53.0
JACKS = [WIDTH / 2 + 12.6 * (i - 2.5) for i in range(6)]


def panel():
    p = Panel(WIDTH, HEIGHT)
    p.rect(0, 0, WIDTH, HEIGHT, 0, 'panel')
    p.title('Sequencer')

    # The tempo: a display that reads INT or EXT, its knob and swing. Then the track's note length and quantize.
    p.rect(3.0, 11.0, 33.0, 15.5, 2.0, 'box')
    p.place('TEMPO_WIDGET', 4.5, 12.5, 21.0, 12.5)
    for y, name in [(15.6, 'INT'), (21.9, 'EXT')]:
        p.place(f'{"INTERNAL" if name == "INT" else "EXTERNAL"}_LIGHT', 28.2, y)
        p.text(name, 32.4, y, size=1.8, fill='box_ink')
    for x, name, label in zip(KNOBS, ['TEMPO', 'SWING', 'LENGTH', 'QUANTIZE'], ['TEMPO', 'SWING', 'LENGTH', 'QUANT']):
        p.place(f'{name}_PARAM', x, KNOB_ROW)
        p.text(label, x, KNOB_ROW + 6.6, size=SMALL)
    p.line(58.25, 12.0, 58.25, 26.0)
    p.line(3.0, 29.5, WIDTH - 3.0, 29.5)

    # TRACK on the left, the transport on the right, each with its LED above.
    keys = [(8.0, 'TRACK'), (WIDTH - 31.0, 'REC'), (WIDTH - 20.0, 'PLAY'), (WIDTH - 9.0, 'STOP')]
    for x, name in keys:
        if name != 'STOP':
            p.place(f'{name}_LIGHT', x, LED_ROW)
        p.place(f'{name}_PARAM', x, FUNCTION_ROW)
        p.text(name, x, FUNCTION_ROW + 4.8, size=SMALL)

    # The 16 steps, numbered on the beats.
    pitch = (WIDTH - 6.0) / 16
    for i in range(16):
        x = 3.0 + pitch * (i + 0.5)
        p.place(f'STEP{i}_PARAM', x, STEP_ROW)
        if i % 4 == 0:
            p.text(str(i + 1), x, STEP_ROW + 4.2, size=1.8)

    # The channel LEDs between the octave keys, then two octaves of keys, laid out exactly as on an engine.
    p.place('OCTAVE_DOWN_PARAM', 3.0 + FUNCTION_KEY[0] / 2, VOICE_ROW)
    p.place('OCTAVE_UP_PARAM', WIDTH - 3.0 - FUNCTION_KEY[0] / 2, VOICE_ROW)
    for i in range(16):
        p.place(f'CHANNEL{i}_LIGHT', WIDTH / 2 + 3.4 * (i - 7.5), VOICE_ROW)
    pitch = (WIDTH - 6.0) / 14
    for octave in range(2):
        for i, semitone in enumerate([0, 2, 4, 5, 7, 9, 11]):
            x = 3.0 + pitch * (7 * octave + i + 0.5)
            p.place(f'KEY{12 * octave + semitone}_PARAM', x, WHITE_ROW)
            if semitone not in (4, 11):
                p.place(f'KEY{12 * octave + semitone + 1}_PARAM', x + pitch / 2, BLACK_ROW)

    # Inputs on the left: the clock's, then the notes to record. The outputs on the right, in their dark box.
    top, bottom = JACK_ROWS
    box = JACKS[3] - 6.0, JACKS[5] + 6.0
    p.rect(box[0], top - 8.6, box[1] - box[0], bottom - top + 15.0, 2.0, 'box')
    rows = [(top, ['CLOCK', 'RESET', 'ADVANCE'], ['CLOCK', 'RESET', None]),
            (bottom, ['VOCT', 'GATE', 'VELOCITY'], ['VOCT', 'GATE', 'VELOCITY'])]
    labels = dict(VOCT='V/OCT', VELOCITY='VEL', ADVANCE='ADV')
    for y, ins, outs in rows:
        for x, name, kind in zip(JACKS, ins + outs, ['INPUT'] * 3 + ['OUTPUT'] * 3):
            if name:
                p.text(labels.get(name, name), x, y - 6.2, size=LABEL, fill='box_ink' if kind == 'OUTPUT' else 'ink')
                p.place(f'{name}_{kind}', x, y)
                p.place(f'{name}_{kind}_LIGHT', x + 3.9, y - 3.9)
    return p
