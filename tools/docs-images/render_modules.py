#!/usr/bin/env python3
"""Renders every vcvopxyz module in a running Rack for the manual, light and dark, each beside a playing Sequencer so
its screen shows a track:

    docs/assets/modules/<slug>-large-<theme>.png   the whole panel at 125%
    docs/assets/modules/<slug>-small-<theme>.png   the whole panel at 50%, for tables
    docs/assets/modules/<slug>-label-<theme>.png   a square off the panel's top (at most 42 mm), its name, at 175%

Rack draws at RENDER zoom once per module and theme; the sizes are cut and scaled from that.

    render_modules.py <rack-headless dir> <plugin dir> [slug ...]
"""
import json, os, re, subprocess, sys, tempfile
from pathlib import Path
from PIL import Image

RENDER = 3.0                 # Rack zoom to draw at
PX_MM = 2.953 * RENDER       # pixels per mm at that zoom
MENU = 32.2                  # the menu bar's height (px)
HP, ROW = 5.08, 128.5
SIZES = dict(large=1.25, small=0.5, label=1.75)
SOUND_PARAM = 123             # the Sequencer's first track param (SOUND_PARAM in Sequencer.cpp)

rack, plugin = Path(sys.argv[1]), Path(sys.argv[2])
out = plugin / 'docs/assets/modules'
out.mkdir(parents=True, exist_ok=True)
manifest = json.loads((plugin / 'plugin.json').read_text())
version = manifest['version']


def save(image, path):
    image.save(path)
    subprocess.run(['pngquant', '--force', '--skip-if-larger', '--quality', '80-95', '--output', path, path])


def hp(slug):
    svg = (plugin / f'res/{slug}.svg').read_text()
    return round(float(svg.split('width="', 1)[1].split('mm"', 1)[0]) / HP)


def sequencer():
    """A Sequencer with chords on the synth tracks, four on the floor on the kick and offbeats on the snare."""
    chords = [[[48, 100, 0, 72], [55, 100, 0, 72], [60, 100, 0, 72]], [], [], [[48, 100, 0, 0]], [[55, 100, 0, 0]], [],
              [[58, 100, 8, 0]], [], [[53, 100, 0, 0], [60, 100, 0, 0]], [], [[53, 100, 0, 0]], [[55, 100, 0, 0]],
              [[57, 100, 0, 0]], [], [[60, 100, 0, 0]], []]
    kick = [[[53, 100, 0, 0]] if s % 4 == 0 else [] for s in range(16)]
    snare = [[[53, 100, 0, 0]] if s % 8 == 4 else [] for s in range(16)]
    tracks = [{'pattern': p, 'channels': 2} for p in (kick, snare, [[]] * 16, [[]] * 16)]
    tracks += [{'pattern': chords, 'channels': c} for c in (1, 3, 1, 4)]
    return {'plugin': 'vcvopxyz', 'model': 'Sequencer', 'params': [],
            'data': {'tracks': tracks, 'track': 4, 'ppqn': 4, 'longPressHolds': True, 'ctrlClickHolds': True}}


def context(slug):
    """The row to draw `slug` in, and its place there: a Sequencer first, then the module."""
    row = [sequencer()] + ([] if slug == 'Sequencer' else [{'plugin': 'vcvopxyz', 'model': slug, 'params': []}])
    x = 0
    for i, m in enumerate(row):
        m.update(id=i + 1, version=version, pos=[x, 0])
        x += hp(m['model'])
    # Off to the side, a constant 10V into RUN keeps the Sequencer playing.
    row.append({'id': 99, 'plugin': 'Fundamental', 'model': '8vert', 'version': '2.6.4', 'pos': [0, 1],
                'params': [{'id': 0, 'value': 1.0}]})
    cables = [{'id': 1, 'outputModuleId': 99, 'outputId': 0, 'inputModuleId': 1, 'inputId': 1, 'color': '#ff9352'}]
    return {'version': '2.6.6', 'modules': row, 'cables': cables}, row[-2]['pos'][0]


def track_param(track, param):
    """The Sequencer's param id of a track's param: lanes 0 to 7 for the eight tracks, 9 and 10 for FX 1 and 2 (8, the
    master track, has none), and the param's index among a track's 22 (P1 0 ... FX 1 12 ... LEVEL 15)."""
    return SOUND_PARAM + (track if track < 8 else track - 1) * 22 + param


# The Sequencer's keys (rect boxes) and dials (circle centres), in panel mm, and where a key is on screen with the
# Sequencer leftmost in the top row.
svg = (plugin / 'res/Sequencer.svg').read_text()
BOXES = {m[0]: tuple(map(float, m[1:])) for m in re.findall(
    r'<rect id="(\w+)"[^>]* x="([\d.]+)" y="([\d.]+)" width="([\d.]+)" height="([\d.]+)"', svg)}
CENTRES = {m[0]: tuple(map(float, m[1:])) for m in re.findall(r'<circle id="(\w+)"[^>]* cx="([\d.]+)" cy="([\d.]+)"', svg)}
X0, Y0 = 0.5 * HP * PX_MM, MENU + 0.02 * ROW * PX_MM


def at(name):
    x, y, w, h = BOXES[f'{name}_PARAM']
    return round(X0 + (x + w / 2) * PX_MM), round(Y0 + (y + h / 2) * PX_MM)


def clicks(names):
    """xdotool lines for an ACTIONS script: click each key, or long-press it if it starts with '~'. A (hp, x, y)
    clicks x, y mm into the top row's module that starts hp from the left."""
    lines = []
    for c in names:
        if isinstance(c, tuple):
            hp_, x, y = c
            x, y = round(X0 + (hp_ * HP + x) * PX_MM), round(Y0 + y * PX_MM)
            c = ''
        else:
            x, y = at(c.lstrip('~'))
        lines.append(f'xdotool mousemove {x} {y}; sleep 0.2')
        lines.append('xdotool mousedown 1; sleep 0.8; xdotool mouseup 1' if c.startswith('~') else 'xdotool click 1')
        lines.append('sleep 0.4')
    return lines



def render(slug):
    patch, left = context(slug)
    width = hp(slug) * HP
    window = (round((left * HP + width + 10) * PX_MM), round(ROW * PX_MM + 120))
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        (tmp / 'patch.json').write_text(json.dumps(patch))
        for theme, dark in (('light', '0'), ('dark', '1')):
            shot = tmp / f'{theme}.png'
            subprocess.run([rack / 'shot.sh', plugin, tmp / 'patch.json', shot, '3', f'{window[0]}x{window[1]}'], check=True,
                           env=dict(os.environ, ZOOM=str(RENDER), DARK=dark, CABLES='0'),
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            x0, y0 = (0.5 + left) * HP * PX_MM, MENU + 0.02 * ROW * PX_MM
            panel = Image.open(shot).convert('RGB').crop((round(x0), round(y0), round(x0 + width * PX_MM), round(y0 + ROW * PX_MM)))
            for size, zoom in SIZES.items():
                image = panel
                if size == 'label':
                    side = min(width, 42.0) * PX_MM
                    image = panel.crop((round((panel.width - side) / 2), 0, round((panel.width + side) / 2), round(side)))
                scale = zoom / RENDER
                image = image.resize((round(image.width * scale), round(image.height * scale)), Image.LANCZOS)
                save(image, out / f'{slug}-{size}-{theme}.png')
    print(slug, flush=True)


if __name__ == '__main__':
    for slug in sys.argv[3:] or [m['slug'] for m in manifest['modules']]:
        render(slug)
