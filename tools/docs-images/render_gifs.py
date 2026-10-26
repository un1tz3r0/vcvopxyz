#!/usr/bin/env python3
"""The manual's three gestures as short loops, light and dark, recorded from a running Rack with the pointer in view:

    docs/assets/gifs/<gesture>-<theme>.gif

    render_gifs.py <rack-headless dir> <plugin dir> [gesture ...]
"""
import json, os, subprocess, sys, tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from render_guide import BASS, MIX, layout, plugin, rack, sequencer  # noqa: E402
from render_modules import BOXES, HP, MENU, ROW  # noqa: E402

ZOOM, FPS, SCALE = 1.5, 12, 0.75
PX_MM = 2.953 * ZOOM
X0, Y0 = 0.5 * HP * PX_MM, MENU + 0.02 * ROW * PX_MM
out = plugin / 'docs/assets/gifs'
out.mkdir(parents=True, exist_ok=True)

# gesture: (the part of the face shown, as x, y, width, height in mm; the actions). An action is a key to click,
# '~KEY:s' to hold it for s seconds, or a pause in seconds.
GESTURES = {
    'long-press': ((0, 51, 136, 55), [1.0, '~STEP0:1.2', 2.0, 'STEP0', 1.2]),
    'click-latch': ((0, 17, 136, 88), [1.0, 'TRACK', 1.5, 'KEY8', 1.5, 'KEY1', 1.5, 'TRACK', 1.2]),
    'hold-stop': ((0, 51, 260, 55), [1.0, 'TRACK', 1.0, '~STOP:1.4', 1.5, 'TRACK', 1.0]),
}


# The X server's default pointer is a cross. This gives Rack's window, which sets none, the standard arrow.
ARROW = '''python3 - <<'END'
import ctypes
x = ctypes.CDLL('libX11.so.6')
x.XOpenDisplay.restype = x.XDefaultRootWindow.restype = x.XCreateFontCursor.restype = ctypes.c_void_p
x.XDefaultRootWindow.argtypes = x.XFlush.argtypes = [ctypes.c_void_p]
x.XCreateFontCursor.argtypes = [ctypes.c_void_p, ctypes.c_uint]
x.XDefineCursor.argtypes = [ctypes.c_void_p] * 3
d = x.XOpenDisplay(None)
x.XDefineCursor(d, x.XDefaultRootWindow(d), x.XCreateFontCursor(d, 68))
x.XFlush(d)
END'''


def at(key):
    x, y, w, h = BOXES[f'{key}_PARAM']
    return round(X0 + (x + w / 2) * PX_MM), round(Y0 + (y + h / 2) * PX_MM)


def script(box, actions, video):
    """An ACTIONS script that records the box while it does the actions."""
    x, y, w, h = (round(v * PX_MM) for v in box)
    x, y, w, h = round(X0) + x, round(Y0) + y, w - w % 2, h - h % 2
    length = sum(a if isinstance(a, float) else float(a.split(':')[1]) + 0.3 if a[0] == '~' else 0.3 for a in actions)
    lines = [ARROW, f'ffmpeg -loglevel error -y -f x11grab -draw_mouse 1 -framerate {FPS} -video_size {w}x{h} '
             f'-i "$DISPLAY+{x},{y}" -t {length + 0.5:.1f} -c:v ffv1 {video} &', 'recording=$!', 'sleep 0.3']
    for a in actions:
        if isinstance(a, float):
            lines.append(f'sleep {a}')
            continue
        key, _, hold = a.lstrip('~').partition(':')
        lines.append('xdotool mousemove {} {}; sleep 0.3'.format(*at(key)))
        lines.append(f'xdotool mousedown 1; sleep {hold}; xdotool mouseup 1' if hold else 'xdotool click 1')
    return '\n'.join(lines + ['wait $recording']) + '\n'


def render(name):
    box, actions = GESTURES[name]
    row, cables, x = layout((sequencer(BASS, MIX), [], [], 1.0, {}))
    window = f'{round((x * HP + 10) * PX_MM)}x{round(ROW * PX_MM + 140)}'
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        (tmp / 'patch.json').write_text(json.dumps({'version': '2.6.6', 'modules': row, 'cables': cables}))
        for theme, dark in (('light', '0'), ('dark', '1')):
            video = tmp / f'{theme}.mkv'
            (tmp / 'actions.sh').write_text(script(box, actions, video))
            subprocess.run([rack / 'shot.sh', plugin, tmp / 'patch.json', tmp / 'shot.png', '0', window], check=True,
                           env=dict(os.environ, ZOOM=str(ZOOM), DARK=dark, CABLES='0', ACTIONS=str(tmp / 'actions.sh')),
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            palette = f'fps={FPS},scale=iw*{SCALE}:-1:flags=lanczos,split[a][b];[a]palettegen=stats_mode=diff[p];' \
                      '[b][p]paletteuse=dither=bayer:bayer_scale=4:diff_mode=rectangle'
            subprocess.run(['ffmpeg', '-loglevel', 'error', '-y', '-i', video, '-vf', palette, out / f'{name}-{theme}.gif'],
                           check=True)
    print(name, flush=True)


for name in sys.argv[3:] or GESTURES:
    render(name)
