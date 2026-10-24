#!/usr/bin/env python3
"""Close-ups of the Sequencer's keys and dials for the manual, light and dark, in the states the text talks about.
Each state is one run of Rack with clicks played into it; every key named for the state is cut out with its icon:

    docs/assets/keys/<key>-<state>-<theme>.png     at 125%, or 160% for the display's keys
    docs/assets/keys/face-<state>-<theme>.png      the whole face, at 100%
    docs/assets/plugs/<slug>-<theme>.png           the display showing each plug, at 125% (state "plugs")

    render_keys.py <rack-headless dir> <plugin dir> [state ...]
"""
import json, os, subprocess, sys, tempfile
from pathlib import Path
from PIL import Image, ImageStat

sys.path.insert(0, str(Path(__file__).parent))
from render_modules import BOXES, CENTRES, HP, PX_MM, RENDER, ROW, X0, Y0, at, clicks, hp, save, sequencer  # noqa: E402

rack, plugin = Path(sys.argv[1]), Path(sys.argv[2])
out = plugin / 'docs/assets/keys'
out.mkdir(parents=True, exist_ok=True)
version = json.loads((plugin / 'plugin.json').read_text())['version']
ZOOM, DISPLAY_ZOOM = 1.25, 1.6

STEPS = [f'STEP{i}' for i in range(16)]
WHITE = [f'KEY{k}' for k in (0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23)]
BLACK = [f'KEY{k}' for k in (1, 3, 5, 8, 10, 13, 15, 17, 20, 22)]
FUNCTIONS = ['TRACK', 'REC', 'PLAY', 'STOP', 'OCTAVE_DOWN', 'OCTAVE_UP', 'SHIFT']
INDEX = ['PROJECT', 'MIXER', 'METRONOME', 'SCREEN']
PAGES = [f'PAGE{i}' for i in range(4)]
TRACKS = [f'SELECT{i}' for i in range(8)] + ['FX_SELECT0', 'FX_SELECT1', 'MASTER']
DIALS = [f'DIAL{d}' for d in range(4)]
DISPLAY = PAGES + TRACKS
ALL = INDEX + PAGES + TRACKS + FUNCTIONS + STEPS + WHITE + BLACK + DIALS

# state: (clicks, keys to cut out, playing, empty patterns). A click is a key, '~KEY' a long press.
STATES = {
    'off': ([], ALL, False, True),
    'stopped': ([], STEPS + TRACKS + DIALS, False, False),
    'playing': ([], ['PLAY'] + STEPS + TRACKS, True, False),
    'rec': (['REC'], ['REC'], False, False),
    'track': (['TRACK'], ['TRACK'] + STEPS + WHITE + BLACK + DIALS, False, False),
    'shift': (['~SHIFT'], ['SHIFT'] + STEPS + WHITE, False, False),
    'held': (['~STEP0'], STEPS + WHITE + BLACK + DIALS, False, False),
    'project': (['PROJECT'], ['PROJECT'] + STEPS + WHITE + BLACK, False, False),
    'mixer': (['MIXER'], ['MIXER'] + STEPS + BLACK + DIALS, False, False),
    'tempo': (['METRONOME'], ['METRONOME'] + DIALS, False, False),
    'page': (['PAGE1'], PAGES + DIALS, False, False),
    'master': (['MASTER'], ['MASTER'] + WHITE + BLACK, False, False),
    'components': (['~SHIFT', 'STEP0', 'STEP4', 'STEP8', 'KEY4', 'KEY5', 'KEY14'], STEPS + WHITE + BLACK, False, False),
}


def box(name):
    """A key with its icon, or a dial with its LED, in panel mm."""
    if name.startswith('DIAL'):
        cx, cy = CENTRES[f'{name}_WIDGET']
        return cx - 14.5, cy - 14, cx + 14.5, cy + 21
    x, y, w, h = BOXES[f'{name}_PARAM']
    if name in DISPLAY:
        return x - 0.3, y - 0.3, x + w + 0.3, y + h + 0.3
    cx, cy = x + w / 2, y + h / 2
    if name in STEPS or name in WHITE:
        return cx - 6.3, cy - 10.8, cx + 6.3, cy + 6
    return cx - 6, cy - 6, cx + 6, cy + 6


def actions(keys):
    lines = ['sleep 1.5'] + clicks(keys)
    # Two frames apart, for keys that blink: each key comes from the frame it's brighter in.
    lines.append(f'xdotool mousemove 5 {round(Y0 + ROW * PX_MM + 40)}; sleep 0.6; snap state.png; sleep 0.3; snap state2.png')
    return '\n'.join(lines) + '\n'


def crop(shot, box):
    l, t, r, b = box
    return shot.crop((round(X0 + l * PX_MM), round(Y0 + t * PX_MM), round(X0 + r * PX_MM), round(Y0 + b * PX_MM)))


def scaled(image, zoom):
    scale = zoom / RENDER
    return image.resize((round(image.width * scale), round(image.height * scale)), Image.LANCZOS)


def run(seq, playing, script, shot):
    """Rack on a patch of the Sequencer alone, with an ACTIONS script, in both themes: shot(theme, folder) after each."""
    seq.update(id=1, version=version, pos=[0, 0])
    row, cables = [seq], []
    if playing:
        row.append({'id': 99, 'plugin': 'Fundamental', 'model': '8vert', 'version': '2.6.4', 'pos': [0, 1],
                    'params': [{'id': 0, 'value': 1.0}]})
        cables.append({'id': 1, 'outputModuleId': 99, 'outputId': 0, 'inputModuleId': 1, 'inputId': 1, 'color': '#ff9352'})
    window = f'{round(hp("Sequencer") * HP * PX_MM + 100)}x{round(ROW * PX_MM + 140)}'
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        (tmp / 'patch.json').write_text(json.dumps({'version': '2.6.6', 'modules': row, 'cables': cables}))
        (tmp / 'actions.sh').write_text(script)
        for theme, dark in (('light', '0'), ('dark', '1')):
            subprocess.run([rack / 'shot.sh', plugin, tmp / 'patch.json', tmp / 'end.png', '2', window], check=True,
                           env=dict(os.environ, ZOOM=str(RENDER), DARK=dark, CABLES='0', ACTIONS=str(tmp / 'actions.sh')),
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            shot(theme, tmp)


def render(state):
    presses, keys, playing, empty = STATES[state]
    seq = sequencer()
    if empty:
        for t in seq['data']['tracks']:
            t['pattern'] = [[]] * 16

    def shot(theme, tmp):
        shots = [Image.open(tmp / f).convert('RGB') for f in ('state.png', 'state2.png')]
        for name in keys:
            image = max((crop(s, box(name)) for s in shots), key=lambda c: sum(ImageStat.Stat(c).mean))
            save(scaled(image, DISPLAY_ZOOM if name in DISPLAY else ZOOM), out / f'{name.lower()}-{state}-{theme}.png')
        face = crop(shots[0], (0, 0, hp('Sequencer') * HP, ROW))
        save(scaled(face, 1.0), out / f'face-{state}-{theme}.png')

    run(seq, playing, actions(presses), shot)
    print(state, flush=True)


# The plugs a new Sequencer has where: the bass track's ten synth plugs on its black keys, Shade first on the chord
# track, the six effects on FX 1's and the OP-Z kit first on the kick track.
PLUGS = [('SELECT4', ['Saw', 'Uranus', 'Electric', 'Digital', 'Volt', 'Bow', 'Analog', 'Organ', 'EP', 'Cluster']),
         ('SELECT7', ['Shade']),
         ('FX_SELECT0', ['Delay', 'Reverb', 'Rymd', 'Chorus', 'Crush', 'Dist']),
         ('SELECT0', ['Drum'])]


def render_plugs():
    """The display's plug readout for each plug, picked with TRACK and the black keys."""
    lines = ['sleep 1.5'] + clicks(['TRACK'])
    for select, slugs in PLUGS:
        lines += clicks([select])
        for i, slug in enumerate(slugs):
            if len(slugs) > 1:
                lines += clicks([BLACK[i]])
            lines.append(f'xdotool mousemove 5 {round(Y0 + ROW * PX_MM + 40)}; sleep 0.5; snap plug-{slug}.png')
    plugs = plugin / 'docs/assets/plugs'
    plugs.mkdir(exist_ok=True)
    x, y, w, h = BOXES['PLUG_WIDGET']

    def shot(theme, tmp):
        for _, slugs in PLUGS:
            for slug in slugs:
                image = crop(Image.open(tmp / f'plug-{slug}.png').convert('RGB'), (x - 0.8, y + 1.5, x + w * 0.68, y + h - 1.4))
                save(scaled(image, ZOOM), plugs / f'{slug}-{theme}.png')

    run(sequencer(), False, '\n'.join(lines) + '\n', shot)
    print('plugs', flush=True)


for state in sys.argv[3:] or list(STATES) + ['plugs']:
    render_plugs() if state == 'plugs' else render(state)
