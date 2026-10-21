#!/usr/bin/env python3
"""The first-sound guide's screenshots, light and dark: each scene is a row of modules after a Sequencer, an Audio-2
at the end of the row taking the mix, and the clicks that set it up.

    docs/assets/guide/<scene>-<theme>.png

    render_guide.py <rack-headless dir> <plugin dir> [scene ...]
"""
import json, os, subprocess, sys, tempfile
from pathlib import Path
from PIL import Image

sys.path.insert(0, str(Path(__file__).parent))
from render_modules import HP, PX_MM, RENDER, ROW, X0, Y0, clicks, hp, plugin, rack, save, version  # noqa: E402

out = plugin / 'docs/assets/guide'
out.mkdir(parents=True, exist_ok=True)

EMPTY = [[]] * 16
KICK = [[[53, 100, 0, 0]] if s % 4 == 0 else [] for s in range(16)]
SNARE = [[[54, 100, 0, 0]] if s % 8 == 4 else [] for s in range(16)]
BASS = [[[n, 100, 0, 0]] if n else [] for n in (45, 0, 0, 45, 0, 0, 57, 0, 43, 0, 0, 43, 0, 55, 0, 48)]
CHORDS = [[[57, 100, 0, 96], [60, 100, 0, 96], [64, 100, 0, 96]]] + [[]] * 7 + \
         [[[55, 100, 0, 96], [59, 100, 0, 96], [62, 100, 0, 96]]] + [[]] * 7


def sequencer(track, **patterns):
    """A Sequencer with `track` selected and the named tracks' patterns (kick, snare, bass, lead, arp, chord)."""
    names = ['kick', 'snare', 'perc', 'sample', 'bass', 'lead', 'arp', 'chord']
    channels = [2, 2, 2, 2, 1, 3, 1, 4]
    tracks = [{'pattern': patterns.get(n, EMPTY), 'channels': c} for n, c in zip(names, channels)]
    return {'plugin': 'vcvopxyz', 'model': 'Sequencer', 'params': [],
            'data': {'tracks': tracks, 'track': track, 'ppqn': 4, 'longPressHolds': True, 'ctrlClickHolds': True}}


def engine(slug, fx1=0.0):
    return {'plugin': 'vcvopxyz', 'model': slug, 'params': [{'id': 12, 'value': fx1}] if fx1 else []}


def screen():
    return {'plugin': 'vcvopxyz', 'model': 'Screen', 'params': []}


ARP = [[[57, 100, 0, 384], [60, 100, 0, 384], [64, 100, 0, 384]]] + [[]] * 15
KIT = (51 + 6, 50.86, 22.3)  # the Screen's KIT, beside a Drum after the Sequencer

# scene: (Sequencer, the modules after it, keys to click, scale of the picture[, the first and last module shown])
SCENES = {
    'audio': (sequencer(4), [], [], 1.0),
    'digital': (sequencer(4), [engine('Digital')], [], 1.0),
    'bass': (sequencer(4, bass=BASS), [engine('Digital')], ['PLAY'], 1.0),
    'drum': (sequencer(0, kick=KICK, bass=BASS), [engine('Drum'), engine('Digital')], ['PLAY'], 1.0),
    'delay': (sequencer(4, kick=KICK, bass=BASS), [engine('Drum'), engine('Digital', 0.6), engine('Delay')],
              ['PLAY', 'PAGE3'], 1.0),
    'voices': (sequencer(7, kick=KICK, snare=SNARE, bass=BASS, chord=CHORDS),
               [engine('Drum'), engine('Drum'), engine('Digital', 0.4), engine('Analog'), engine('Organ'),
                engine('EP', 0.5), engine('Delay'), engine('Reverb')], ['PLAY'], 0.75),
    'digital-screen': (sequencer(4, bass=BASS), [engine('Digital'), screen()], ['PLAY'], 1.0, (1, 2)),
    'arp-screen': (sequencer(6, arp=ARP), [engine('Digital'), engine('Analog'),
                                           {'plugin': 'vcvopxyz', 'model': 'Cluster', 'params': [{'id': 18, 'value': 4}]},
                                           screen()], ['PLAY', 'PAGE2'], 1.0, (3, 4)),
    'kit-screen': (sequencer(0, kick=KICK), [engine('Drum'), screen()], ['PLAY', KIT], 1.0, (1, 2)),
}


def render(scene):
    seq, rest, keys, scale, *shown = SCENES[scene]
    row = [seq] + rest + [{'plugin': 'Core', 'model': 'AudioInterface2', 'params': []}]
    first, last = shown[0] if shown else (0, len(row) - 1)
    x = 0
    for i, m in enumerate(row):
        m.update(id=i + 1, version=version if m['plugin'] == 'vcvopxyz' else '2.6.6', pos=[x, 0])
        x += hp(m['model']) if m['plugin'] == 'vcvopxyz' else 5
    width = x * HP
    left, right = row[first]['pos'][0] * HP, (row[last + 1]['pos'][0] if last + 1 < len(row) else x) * HP
    cables = [{'id': i + 1, 'outputModuleId': 1, 'outputId': 4 + i, 'inputModuleId': len(row), 'inputId': i,
               'color': c} for i, c in enumerate(('#f3374b', '#0986ad'))]
    window = f'{round((width + 10) * PX_MM)}x{round(ROW * PX_MM + 140)}'
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        (tmp / 'patch.json').write_text(json.dumps({'version': '2.6.6', 'modules': row, 'cables': cables}))
        (tmp / 'actions.sh').write_text('\n'.join(['sleep 4'] + clicks(keys) + ['xdotool mousemove 3 3']) + '\n')
        for theme, dark in (('light', '0'), ('dark', '1')):
            subprocess.run([rack / 'shot.sh', plugin, tmp / 'patch.json', tmp / 'shot.png', '2', window], check=True,
                           env=dict(os.environ, ZOOM=str(RENDER), DARK=dark, CABLES='0' if shown else '0.8', ACTIONS=str(tmp / 'actions.sh')),
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            shot = Image.open(tmp / 'shot.png').convert('RGB')
            image = shot.crop((round(X0 + left * PX_MM), round(Y0), round(X0 + right * PX_MM), round(Y0 + ROW * PX_MM)))
            zoom = scale / RENDER
            save(image.resize((round(image.width * zoom), round(image.height * zoom)), Image.LANCZOS),
                 out / f'{scene}-{theme}.png')
    print(scene, flush=True)


for scene in sys.argv[3:] or SCENES:
    render(scene)
