#!/usr/bin/env python3
"""The manual's scenes, light and dark: each is a row of modules after a Sequencer, an audio module at the end of the
row taking the mix, and the clicks that set it up.

    docs/assets/guide/<scene>-<theme>.png

It also writes the song they play, from song.py, as patches/first-song.vcv.

    render_guide.py <rack-headless dir> <plugin dir> [scene ...]
"""
import json, math, os, subprocess, sys, tempfile
from pathlib import Path
from PIL import Image

sys.path.insert(0, str(Path(__file__).parent))
import song  # noqa: E402
from render_modules import HP, PX_MM, RENDER, ROW, X0, Y0, clicks, hp, plugin, rack, save, track_param, version  # noqa: E402

out = plugin / 'docs/assets/guide'
out.mkdir(parents=True, exist_ok=True)

FOREIGN_HP = dict(AudioInterface2=5, AudioInterface16=19, MIDIToCVInterface=8, VCO=9, VCF=7, VCA=5, Scope=13, Delay=9,
                  LFO=9, ADSR=9, SEQ3=22, **{'VCA-1': 3, 'MIDI-Map': 10})
FX1, ARP_SPEED = 12, 18       # a track's FX 1 send and arp speed, among its params
BASS, LEAD, ARP = 4, 5, 6     # lanes
SONG = ('kick', 'snare', 'perc', 'bass', 'lead', 'arp')
MIX = [(0, song.TEMPO), (track_param(ARP, FX1), song.FX1_SEND)]


def sequencer(track, params=(), names=SONG, octave=0):
    """A Sequencer with `track` selected, the song's named tracks, params as (id, value) and the keyboard's octave."""
    return {'plugin': 'vcvopxyz', 'model': 'Sequencer', 'params': [{'id': i, 'value': v} for i, v in params],
            'data': {'tracks': song.tracks(*names), 'track': track, 'octave': octave, 'ppqn': 4,
                     'longPressHolds': True, 'ctrlClickHolds': True}}


def module(slug, plugin='vcvopxyz', data=None, **params):
    m = {'plugin': plugin, 'model': slug, 'params': [{'id': int(i[1:]), 'value': v} for i, v in params.items()]}
    return dict(m, data=data) if data else m


def fundamental(slug, **params):
    return module(slug, 'Fundamental', **params)


DIGITAL = 'KEY8'              # the bass track's fourth slot, Digital, on the fourth black key
# The VCF with its cutoff fully down and its FREQ and RES inputs' dials fully up.
VCF = fundamental('VCF', p0=(math.log2(261.626 / 8000) + 5) / 10, p3=1.0, p5=1.0)
# Your first patch, a stage at a time: (module, output, module, input) by place in the row, the Sequencer 0.
VOCT, ENV, FILTER, RESO, SEND_L, RET_L = 4, 6, 10, 11, 17, 6
SAW, LPF, VCA_OUT, WET = 2, 0, 0, 1
VCO_ONLY = [(1, VOCT, 2, 0), (2, SAW, 1, RET_L)]
FILTERED = [(1, VOCT, 2, 0), (2, SAW, 3, 3), (3, LPF, 1, RET_L), (1, FILTER, 3, 0), (1, RESO, 3, 2)]
ENVELOPED = FILTERED[:2] + FILTERED[3:] + [(3, LPF, 4, 1), (4, VCA_OUT, 1, RET_L), (1, ENV, 4, 0)]
# With a second breakout at 2, the VCO and the rest move one place along.
moved = lambda cables, by: [(a + by * (a > 1), o, b + by * (b > 1), i) for a, o, b, i in cables]
VOICE = [module('Breakout'), fundamental('VCO'), VCF, fundamental('VCA-1')]

# scene: (Sequencer, the modules after it, keys to click, scale of the picture, options): `shown`, the first and last
# module in the picture; `cables`, (module, output, module, input) by their place in the row, AUDIO for the audio module;
# `audio`, that module's slug; `mix=False` leaves the Sequencer's outputs to the scene's cables.
SCENES = {
    # Your first song
    'audio': (sequencer(BASS, names=()), [], [], 1.0, {}),
    'bass': (sequencer(BASS, names=('bass',), octave=-1), [], ['PLAY'], 1.0, {}),
    'slots': (sequencer(BASS, names=('bass',), octave=-1), [], ['PLAY', 'TRACK', DIGITAL], 1.0, {}),
    'drums': (sequencer(2, names=('kick', 'snare', 'perc', 'bass')), [], ['PLAY'], 1.0, {}),
    'length': (sequencer(ARP), [], ['~STEP0'], 1.0, {}),
    'delay': (sequencer(ARP, MIX[1:]), [], ['PLAY', 'PAGE3'], 1.0, {}),
    'tempo': (sequencer(ARP, MIX), [], ['PLAY', 'METRONOME'], 1.0, {}),
    'song': (sequencer(ARP, MIX), [], ['PLAY'], 0.75, {}),
    'band': (sequencer(ARP, MIX), [module('Screen')], ['PLAY'], 0.75, {}),
    # Your first patch
    'breakout': (sequencer(BASS, MIX), VOICE[:2], ['PLAY'], 0.75, dict(cables=VCO_ONLY)),
    'breakout-filter': (sequencer(BASS, MIX), VOICE[:3], ['PLAY'], 1.0, dict(shown=(1, 4), cables=FILTERED)),
    'breakout-env': (sequencer(BASS, MIX), VOICE, ['PLAY'], 1.0, dict(shown=(1, 5), cables=ENVELOPED)),
    'breakout-out': (sequencer(BASS, MIX), VOICE[:1] + [module('Breakout')] + VOICE[1:] + [fundamental('Scope')],
                     ['PLAY'], 0.75, dict(shown=(1, 8), cables=moved(ENVELOPED, 1) + [(2, 0, 7, 0)])),
    'send': (sequencer(BASS, MIX), VOICE[:1] + [module('Breakout'), module('FxBreakout')] + VOICE[1:] +
             [fundamental('Scope'), fundamental('Delay')], ['PLAY'], 0.75,
             dict(shown=(1, 10), cables=moved(ENVELOPED, 2) + [(3, SEND_L, 9, 4), (9, WET, 3, RET_L)])),
    # The Screen
    'screen': (sequencer(BASS, names=('bass',)), [module('Screen')], ['PLAY'], 1.0, dict(shown=(1, 1))),
    'arp-screen': (sequencer(ARP, [(track_param(ARP, ARP_SPEED), 4)], names=('arp',)), [module('Screen')],
                   ['PLAY', 'PAGE2'], 1.0, dict(shown=(1, 1))),
    # KIT, on the Screen just after the Sequencer
    'kit-screen': (sequencer(0, names=('kick',)), [module('Screen')], ['PLAY', (hp('Sequencer'), 50.86, 22.3)], 1.0,
                   dict(shown=(1, 1))),
}


AUDIO = -1


def layout(scene):
    """A scene's patch: its row, with ids and places, and its cables; and the row's width in HP."""
    seq, rest, keys, scale, options = scene
    row = [seq] + rest + [module(options.get('audio', 'AudioInterface2'), 'Core')]
    x = 0
    for i, m in enumerate(row):
        ours = m['plugin'] == 'vcvopxyz'
        m = row[i] = dict(m, id=i + 1, version=version if ours else '2.6.6' if m['plugin'] == 'Core' else '2.6.4', pos=[x, 0])
        x += hp(m['model']) if ours else FOREIGN_HP[m['model']]
    # The mix into the audio module, then the scene's own cables.
    wires = [] if options.get('mix') is False else [(0, 4, AUDIO, 0, '#f3374b'), (0, 5, AUDIO, 1, '#0986ad')]
    colours = ('#ffb437', '#a34cc4', '#2cb64e', '#e9536a', '#3695ef')
    wires += [(*c, colours[i % len(colours)]) for i, c in enumerate(options.get('cables', []))]
    cables = [{'id': i + 1, 'outputModuleId': a % len(row) + 1, 'outputId': o, 'inputModuleId': b % len(row) + 1,
               'inputId': n, 'color': c} for i, (a, o, b, n, c) in enumerate(wires)]
    return row, cables, x


def render(name, scene, out=out):
    seq, rest, keys, scale, options = scene
    row, cables, x = layout(scene)
    first, last = options.get('shown', (0, len(row) - 1))
    width = x * HP
    left, right = row[first]['pos'][0] * HP, (row[last + 1]['pos'][0] if last + 1 < len(row) else x) * HP
    window = f'{round((width + 10) * PX_MM)}x{round(ROW * PX_MM + 140)}'
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        (tmp / 'patch.json').write_text(json.dumps({'version': '2.6.6', 'modules': row, 'cables': cables}))
        (tmp / 'actions.sh').write_text('\n'.join(['sleep 4'] + clicks(keys) + ['xdotool mousemove 3 3']) + '\n')
        for theme, dark in (('light', '0'), ('dark', '1')):
            subprocess.run([rack / 'shot.sh', plugin, tmp / 'patch.json', tmp / 'shot.png', '2', window], check=True,
                           env=dict(os.environ, ZOOM=str(RENDER), DARK=dark, CABLES='0' if 'shown' in options and 'cables' not in options else '0.8',
                                    ACTIONS=str(tmp / 'actions.sh')),
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            shot = Image.open(tmp / 'shot.png').convert('RGB')
            image = shot.crop((round(X0 + left * PX_MM), round(Y0), round(X0 + right * PX_MM), round(Y0 + ROW * PX_MM)))
            zoom = scale / RENDER
            save(image.resize((round(image.width * zoom), round(image.height * zoom)), Image.LANCZOS),
                 out / f'{name}-{theme}.png')
    print(name, flush=True)


if __name__ == '__main__':
    song.write(plugin / 'patches/first-song.vcv', *layout((sequencer(ARP, MIX), [], [], 1.0, {}))[:2])
    for scene in sys.argv[3:] or SCENES:
        render(scene, SCENES[scene])
