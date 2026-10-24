#!/usr/bin/env python3
"""The manual's scenes, light and dark: each is a row of modules after a Sequencer, an Audio-2 at the end of the row
taking the mix, and the clicks that set it up.

    docs/assets/guide/<scene>-<theme>.png

    render_guide.py <rack-headless dir> <plugin dir> [scene ...]
"""
import json, os, subprocess, sys, tempfile
from pathlib import Path
from PIL import Image

sys.path.insert(0, str(Path(__file__).parent))
from render_modules import HP, PX_MM, RENDER, ROW, X0, Y0, clicks, hp, plugin, rack, save, track_param, version  # noqa: E402

out = plugin / 'docs/assets/guide'
out.mkdir(parents=True, exist_ok=True)

EMPTY = [[]] * 16
KICK = [[[53, 100, 0, 0]] if s % 4 == 0 else [] for s in range(16)]
SNARE = [[[54, 100, 0, 0]] if s % 8 == 4 else [] for s in range(16)]
BASS = [[[n, 100, 0, 0]] if n else [] for n in (45, 0, 0, 45, 0, 0, 57, 0, 43, 0, 0, 43, 0, 55, 0, 48)]
CHORDS = [[[57, 100, 0, 96], [60, 100, 0, 96], [64, 100, 0, 96]]] + [[]] * 7 + \
         [[[55, 100, 0, 96], [59, 100, 0, 96], [62, 100, 0, 96]]] + [[]] * 7
ARP = [[[57, 100, 0, 384], [60, 100, 0, 384], [64, 100, 0, 384]]] + [[]] * 15
FOREIGN_HP = dict(AudioInterface2=5, VCO=9)


def sequencer(track, params=(), **patterns):
    """A Sequencer with `track` selected, the named tracks' patterns (kick, snare, bass, lead, arp, chord) and params
    as (lane, param, value)."""
    names = ['kick', 'snare', 'perc', 'sample', 'bass', 'lead', 'arp', 'chord']
    channels = [2, 2, 2, 2, 1, 3, 1, 4]
    tracks = [{'pattern': patterns.get(n, EMPTY), 'channels': c} for n, c in zip(names, channels)]
    return {'plugin': 'vcvopxyz', 'model': 'Sequencer', 'params': [{'id': track_param(t, p), 'value': v} for t, p, v in params],
            'data': {'tracks': tracks, 'track': track, 'ppqn': 4, 'longPressHolds': True, 'ctrlClickHolds': True}}


def module(slug, plugin='vcvopxyz'):
    return {'plugin': plugin, 'model': slug, 'params': []}


FX1, ARP_SPEED = 12, 18       # a track's FX 1 send and arp speed, among its params
DIGITAL = 'KEY8'              # the bass track's fourth slot, Digital, on the fourth black key

# scene: (Sequencer, the modules after it, keys to click, scale of the picture, options): `shown`, the first and last
# module in the picture; `cables`, (module, output, module, input) by their place in the row.
SCENES = {
    'audio': (sequencer(4), [], [], 1.0, {}),
    'bass': (sequencer(4, bass=BASS), [], ['PLAY'], 1.0, {}),
    'slots': (sequencer(4, bass=BASS), [], ['PLAY', 'TRACK', DIGITAL], 1.0, {}),
    'drum': (sequencer(0, kick=KICK, bass=BASS), [], ['PLAY'], 1.0, {}),
    'delay': (sequencer(4, [(4, FX1, 0.6)], kick=KICK, bass=BASS), [], ['PLAY', 'PAGE3'], 1.0, {}),
    'breakout': (sequencer(4, kick=KICK, bass=BASS), [module('Breakout'), module('VCO', 'Fundamental')], ['PLAY'], 1.0,
                 dict(cables=[(1, 4, 2, 0), (2, 2, 1, 6)])),
    'band': (sequencer(7, kick=KICK, snare=SNARE, bass=BASS, chord=CHORDS), [module('Screen')], ['PLAY'], 0.75, {}),
    'screen': (sequencer(4, bass=BASS), [module('Screen')], ['PLAY'], 1.0, dict(shown=(1, 1))),
    'arp-screen': (sequencer(6, [(6, ARP_SPEED, 4)], arp=ARP), [module('Screen')], ['PLAY', 'PAGE2'], 1.0,
                   dict(shown=(1, 1))),
    # KIT, on the Screen just after the Sequencer
    'kit-screen': (sequencer(0, kick=KICK), [module('Screen')], ['PLAY', (hp('Sequencer'), 50.86, 22.3)], 1.0,
                   dict(shown=(1, 1))),
}


def render(scene):
    seq, rest, keys, scale, options = SCENES[scene]
    row = [seq] + rest + [module('AudioInterface2', 'Core')]
    first, last = options.get('shown', (0, len(row) - 1))
    x = 0
    for i, m in enumerate(row):
        ours = m['plugin'] == 'vcvopxyz'
        m.update(id=i + 1, version=version if ours else '2.6.6' if m['plugin'] == 'Core' else '2.6.4', pos=[x, 0])
        x += hp(m['model']) if ours else FOREIGN_HP[m['model']]
    width = x * HP
    left, right = row[first]['pos'][0] * HP, (row[last + 1]['pos'][0] if last + 1 < len(row) else x) * HP
    # The mix into the Audio-2, then the scene's own cables.
    wires = [(0, 4, len(row) - 1, 0, '#f3374b'), (0, 5, len(row) - 1, 1, '#0986ad')]
    wires += [(*c, colour) for c, colour in zip(options.get('cables', []), ('#ffb437', '#a34cc4', '#2cb64e', '#e9536a'))]
    cables = [{'id': i + 1, 'outputModuleId': a + 1, 'outputId': o, 'inputModuleId': b + 1, 'inputId': n, 'color': c}
              for i, (a, o, b, n, c) in enumerate(wires)]
    window = f'{round((width + 10) * PX_MM)}x{round(ROW * PX_MM + 140)}'
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        (tmp / 'patch.json').write_text(json.dumps({'version': '2.6.6', 'modules': row, 'cables': cables}))
        (tmp / 'actions.sh').write_text('\n'.join(['sleep 4'] + clicks(keys) + ['xdotool mousemove 3 3']) + '\n')
        for theme, dark in (('light', '0'), ('dark', '1')):
            subprocess.run([rack / 'shot.sh', plugin, tmp / 'patch.json', tmp / 'shot.png', '2', window], check=True,
                           env=dict(os.environ, ZOOM=str(RENDER), DARK=dark, CABLES='0' if 'shown' in options else '0.8',
                                    ACTIONS=str(tmp / 'actions.sh')),
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            shot = Image.open(tmp / 'shot.png').convert('RGB')
            image = shot.crop((round(X0 + left * PX_MM), round(Y0), round(X0 + right * PX_MM), round(Y0 + ROW * PX_MM)))
            zoom = scale / RENDER
            save(image.resize((round(image.width * zoom), round(image.height * zoom)), Image.LANCZOS),
                 out / f'{scene}-{theme}.png')
    print(scene, flush=True)


for scene in sys.argv[3:] or SCENES:
    render(scene)
