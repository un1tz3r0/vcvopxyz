#!/usr/bin/env python3
"""The patch recipes' scenes, light and dark, and their demo patches: each plays the song of Your first song, from
song.py, with the recipe's modules patched in.

    docs/assets/recipes/<recipe>-<theme>.png
    patches/<recipe>.vcv

    render_recipes.py <rack-headless dir> <plugin dir> [recipe ...]
"""
import math, sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import song  # noqa: E402
from render_guide import (ARP, AUDIO, BASS, LEAD, MIX, SCENES as GUIDE, fundamental, layout, module, plugin,  # noqa: E402
                          render, sequencer, track_param)

out = plugin / 'docs/assets/recipes'
out.mkdir(parents=True, exist_ok=True)

PERC, CHORD = 2, 7                                   # lanes
ATTACK, SUSTAIN, RELEASE, FILTER = 4, 6, 7, 2        # among a track's params
LEAD_STEP_LENGTH, MASTER_LEVEL, PER_ADV = 31, 37, 9  # Sequencer params, and the step length that waits for ADV
seconds = lambda s: math.log(s / 1e-3) / math.log(1e4)  # an envelope dial, an ADSR's or a Delay's time, at s seconds
# Breakout jacks: inputs, then outputs (TrackIds in TrackCore.hpp).
B_VOCT, B_GATE, B_RET_L = 0, 1, 6
B_L, B_R, B_GATE_OUT, B_ENV, B_SOUND, B_SEND_L = 0, 1, 5, 6, 15, 17
# Sequencer jacks.
CLOCK_IN, ADV_IN, VOCT_IN, GATE_IN, VEL_IN = 0, 4, 5, 6, 7
CLOCK_OUT, RESET_OUT, L_OUT, R_OUT = 0, 2, 4, 5


def chord(seq, notes, channels=1, params=()):
    """The Sequencer with notes on track 8 (chord), and that track's params as (index, value)."""
    seq['data']['tracks'][CHORD] = {'pattern': song.pattern(None, notes), 'channels': channels}
    seq['params'] += [{'id': track_param(CHORD, i), 'value': v} for i, v in params]
    return seq


def seq3(steps=8, gates=(1,) * 8, cv=()):
    """A SEQ3 that keeps running, with its step count, gates and first row of CVs."""
    return module('SEQ3', 'Fundamental', data={'running': True, 'gates': list(gates)}, p3=steps,
                  **{f'p{4 + i}': v for i, v in enumerate(cv)})


MAPS = [(1, track_param(BASS, FILTER)), (74, track_param(ARP, FILTER)), (7, MASTER_LEVEL)]   # (CC, Sequencer param)
MELODY = [0.5, 0.75, 13 / 12, 0.75, 11 / 12, 4 / 3, 13 / 12, 2 / 3]   # F#4 A4 C#5 A4 B4 E5 C#5 G#4, at 0 V for C4
PUMP = {s: (60, 90) for s in (1, 5, 9, 13)}                          # a note a beat, ending just before the next

RECIPES = {
    'midi-keyboard': (sequencer(LEAD, MIX), [module('MIDIToCVInterface', 'Core', data={'channels': 3, 'pwRange': 2})],
                      ['PLAY'], 0.75, dict(cables=[(1, 0, 0, VOCT_IN), (1, 1, 0, GATE_IN), (1, 2, 0, VEL_IN)])),
    'midi-map': (sequencer(BASS, MIX), [module('MIDI-Map', 'Core', data={
                     'maps': [{'cc': cc, 'moduleId': 1, 'paramId': p} for cc, p in MAPS], 'smooth': True})],
                 ['PLAY'], 0.75, {}),
    'follow-clock': (sequencer(ARP, MIX), [fundamental('LFO', p2=math.log2(song.TEMPO / 60 * 4))], ['PLAY'], 0.75,
                     dict(cables=[(1, 3, 0, CLOCK_IN)])),
    'lead-clock': (chord(sequencer(CHORD, MIX), {}, 4, [(RELEASE, seconds(0.6))]),
                   [module('Breakout', p0=4), seq3(gates=(1, 1, 1, 0, 1, 1, 1, 0), cv=MELODY)], ['PLAY'], 0.75,
                   dict(cables=[(0, CLOCK_OUT, 2, 1), (0, RESET_OUT, 2, 2), (2, 1, 1, B_VOCT), (2, 0, 1, B_GATE)])),
    'stems': (sequencer(BASS, MIX), [module('DrumBreakout'), module('Breakout'), module('Breakout')], ['PLAY'], 0.75,
              dict(audio='AudioInterface16', cables=[(m, side, AUDIO, 2 * m + side) for m in (1, 2, 3) for side in (B_L, B_R)])),
    'own-voice': (*GUIDE['breakout-env'][:4], dict(GUIDE['breakout-env'][4], picture=False)),
    'drum-modules': (sequencer(PERC, MIX), [module('DrumBreakout', p0=3), fundamental('VCO', p2=-12),
                     fundamental('ADSR', p0=0, p1=seconds(0.15), p2=0, p3=seconds(0.15)), fundamental('VCA-1')],
                     ['PLAY'], 0.75, dict(cables=[(1, B_GATE_OUT, 3, 4), (1, B_SOUND, 2, 0), (2, 0, 4, 1), (3, 0, 4, 0),
                                                  (4, 0, 1, B_RET_L)])),
    'outside-effect': (sequencer(ARP, MIX), [module('FxBreakout'), fundamental('Delay', p0=seconds(0.75 * 60 / song.TEMPO), p1=0.45)],
                       ['PLAY'], 0.75, dict(cables=[(1, B_SEND_L, 2, 4), (2, 1, 1, B_RET_L)])),
    'pump': (chord(sequencer(CHORD, MIX), PUMP, 1, [(ATTACK, seconds(0.25)), (SUSTAIN, 1), (RELEASE, seconds(0.01))]),
             [module('Breakout', p0=4), fundamental('VCA'), fundamental('Scope')], ['PLAY'], 0.75,
             dict(mix=False, cables=[(0, L_OUT, 2, 2), (0, R_OUT, 2, 5), (2, 0, AUDIO, 0), (2, 1, AUDIO, 1),
                                     (1, B_GATE_OUT, 3, 0), (1, B_ENV, 3, 1), (1, B_ENV, 2, 1), (1, B_ENV, 2, 4)])),
    'advance': (sequencer(LEAD, MIX + [(LEAD_STEP_LENGTH, PER_ADV)]), [seq3(steps=3, gates=(1, 0, 0, 0, 0, 0, 0, 0))],
                ['PLAY'], 0.75, dict(cables=[(0, CLOCK_OUT, 1, 1), (0, RESET_OUT, 1, 2), (1, 0, 0, ADV_IN)])),
}

for name in sys.argv[3:] or RECIPES:
    scene = RECIPES[name]
    song.write(plugin / f'patches/{name}.vcv', *layout(scene)[:2])
    if scene[4].get('picture', True):
        render(name, scene, out)
