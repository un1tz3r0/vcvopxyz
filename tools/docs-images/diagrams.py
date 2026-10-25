#!/usr/bin/env python3
"""Draws the manual's diagrams as light and dark SVGs: docs/assets/diagrams/<name>-<theme>.svg.

Usage: tools/docs-images/diagrams.py [output dir]
"""
import sys
from html import escape
from pathlib import Path

THEMES = {
    'light': dict(ink='#1f2328', quiet='#59636e', edge='#8c959f', tint='#eef0f2'),
    'dark': dict(ink='#e6edf3', quiet='#9198a1', edge='#6e7681', tint='#1c222b'),
}
# The kind chips' colours from res-src/synth_track.py, so a diagram's breakouts match their panels.
KIND = dict(synth='#2FC6E0', drum='#FF8A3D', fx='#E25BD6', seq='#7A8AA0')
FONT = "Nunito, 'Segoe UI', -apple-system, Helvetica, Arial, sans-serif"


class Svg:
    def __init__(self, width, height, theme, title):
        self.w, self.h, self.c, self.parts = width, height, THEMES[theme], []
        self.text(24, 32, title, size=17, weight=700)

    def text(self, x, y, words, size=13, weight=400, color='ink', anchor='start'):
        fill = self.c.get(color, color)
        self.parts.append(f'<text x="{x}" y="{y}" font-size="{size}" font-weight="{weight}" fill="{fill}" '
                          f'text-anchor="{anchor}">{escape(words)}</text>')

    def box(self, x, y, w, h, kind=None, dashed=False, tint=False):
        stroke = KIND[kind] if kind else self.c['edge']
        fill = f'fill="{KIND[kind]}" fill-opacity="0.13"' if kind else f'fill="{self.c["tint"] if tint else "none"}"'
        dash = ' stroke-dasharray="5 4"' if dashed else ''
        self.parts.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="8" {fill} stroke="{stroke}" '
                          f'stroke-width="{2 if kind else 1.25}"{dash}/>')

    def lines(self, x, y, rows, anchor='start'):
        """Rows of (words, size, weight, color), 17 apart."""
        for i, (words, *style) in enumerate(rows):
            size, weight, color = (style + [13, 400, 'ink'][len(style):])[:3]
            self.text(x, y + 17 * i, words, size, weight, color, anchor)

    def path(self, d, arrow=True, dashed=False):
        dash = ' stroke-dasharray="4 4"' if dashed else ''
        end = ' marker-end="url(#arrow)"' if arrow else ''
        self.parts.append(f'<path d="{d}" fill="none" stroke="{self.c["edge"]}" stroke-width="1.25"{dash}{end}/>')

    def __str__(self):
        marker = (f'<marker id="arrow" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" '
                  f'orient="auto-start-reverse"><path d="M0 0L10 5L0 10z" fill="{self.c["edge"]}"/></marker>')
        return (f'<svg xmlns="http://www.w3.org/2000/svg" width="{self.w}" height="{self.h}" '
                f'viewBox="0 0 {self.w} {self.h}" font-family="{FONT}"><defs>{marker}</defs>\n'
                + '\n'.join(self.parts) + '\n</svg>\n')


def row(theme):
    s = Svg(880, 252, theme, 'Each breakout takes the first free track of its kind, left to right')
    w, y0, h = 108, 96, 112
    x = lambda i: 16 + 124 * i
    s.text(x(0) + w / 2 + 10, 66, 'The row: modules that touch, with no cables between them', 12, color='quiet')
    s.path(f'M{x(0) + w / 2} {y0}V74H{x(4) + w / 2}', arrow=False)
    for i in range(1, 5):
        s.path(f'M{x(i) + w / 2} 74V{y0}')
    modules = [
        ('seq', ['Sequencer'], ['Plays every', 'track.']),
        ('synth', ['Synth', 'Breakout'], ['Takes track', '5 (bass).']),
        ('synth', ['Synth', 'Breakout'], ['Takes track', '6 (lead).']),
        (None, ['Screen'], ['Shows the', 'selected', 'track.']),
        ('drum', ['Drum', 'Breakout'], ['Takes track', '1 (kick).']),
        ('other', ['Another', "plugin's", 'module'], ['Ends the row.']),
        ('synth', ['Synth', 'Breakout'], ['Takes no', 'track. Shows', 'a dash.']),
    ]
    for i, (kind, name, body) in enumerate(modules):
        off = i > 4
        s.box(x(i), y0, w, h, None if kind in (None, 'other') or off else kind, dashed=off)
        color = 'quiet' if off else 'ink'
        s.lines(x(i) + w / 2, y0 + 24, [(n, 14, 700, color) for n in name], 'middle')
        s.lines(x(i) + w / 2, y0 + 78 - 17 * (len(body) > 2), [(b, 13, 400, color) for b in body], 'middle')
    s.text(24, 236, 'A breakout with no Sequencer to its left, or past a module from another plugin, plays nothing.',
           12, color='quiet')
    return s


def track(theme):
    s = Svg(780, 600, theme, 'Each breakout jack you patch takes one stage of the track outside')
    s.text(24, 54, 'A Synth Breakout shown. The Drum and Send FX Breakouts follow the same rule.', 12, color='quiet')
    s.text(40, 90, 'One track, from its notes to the mix', 12, color='quiet')
    s.text(340, 90, 'Patch this breakout jack, and…', 12, color='quiet')
    stages = ['Notes: steps, keys, inputs', 'Plug voices', 'Envelope', 'Filter', 'Level and FX sends', 'Pan',
              'Sequencer mix: L and R']
    jacks = [
        ('V/OCT and GATE out', 'The plug stops. Outside modules play the notes.'),
        ('RET L and RET R in', 'Their sound takes the place of the plug.'),
        ('ENV out', 'The track stops enveloping the return.'),
        ('FILTER and RESO out', 'The track stops filtering the return.'),
        ('FX 1 and FX 2 out', 'The sends go to the jacks, not to tracks 9 and 10.'),
        ('L and R out', 'The plug plays, but out of these jacks, not the mix.'),
    ]
    for i, name in enumerate(stages):
        cy = 128 + 72 * i
        last = i == len(stages) - 1
        s.box(40, cy - 20, 240, 40, 'seq' if last else None)
        s.text(160, cy + 5, name, 14, 700 if last else 400, anchor='middle')
        if not last:
            s.path(f'M160 {cy + 20}V{cy + 51}')
        if i < len(jacks):
            s.path(f'M280 {cy}H340', arrow=False, dashed=True)
            s.box(340, cy - 28, 416, 56, tint=True)
            s.text(356, cy - 4, jacks[i][0], 14, 700)
            s.text(356, cy + 15, jacks[i][1], 13)
        else:
            s.text(356, cy + 5, 'With nothing patched, the whole track stays inside.', 13, color='quiet')
    return s


def holds(theme):
    s = Svg(960, 388, theme, 'A project slot holds 16 patterns, and each track plays one of its plug slots')
    s.box(16, 56, 520, 300)
    s.lines(32, 80, [('Project slot', 14, 700), ('Ten in a Sequencer. Tempo, swing, metronome, mixer, mute groups, '
                                                  'chains.', 12, 400, 'quiet')])
    s.box(32, 116, 488, 224)
    s.lines(48, 140, [('Pattern', 14, 700), ('Sixteen in a project. One plays at a time.', 12, 400, 'quiet')])
    s.box(48, 176, 456, 148)
    s.lines(64, 200, [('Track', 14, 700), ('Eleven: 1 to 10, and 12 (master). Note length, note style, quantize,',
                                           12, 400, 'quiet'),
                      ('step count, step length, the plug slot it plays, and its dials.', 12, 400, 'quiet')])
    s.box(64, 258, 320, 52, tint=True)
    s.lines(80, 280, [('Step', 14, 700), ('Sixteen in a track. Notes, locks, components.', 12, 400, 'quiet')])
    right = [('Plug slots', 'Ten for each track, filled from the context menu.'),
             ('Plug', 'An engine, a kit or an effect.'),
             ('Presets', "Fourteen for each plug, in Rack's user folder.")]
    for i, (name, body) in enumerate(right):
        y = 116 + 84 * i
        s.box(600, y, 344, 56, 'seq' if i == 1 else None)
        s.lines(616, y + 24, [(name, 14, 700), (body, 12, 400, 'quiet')])
        if i:
            s.path(f'M772 {y - 28}V{y}')
    s.path('M504 216H556V144H600')
    s.text(562, 186, 'plays', 12, color='quiet')
    s.text(600, 372, 'The plug slots and presets belong to no project.', 12, color='quiet')
    return s


def main(out):
    out.mkdir(parents=True, exist_ok=True)
    for draw in (row, track, holds):
        for theme in THEMES:
            (out / f'{draw.__name__}-{theme}.svg').write_text(str(draw(theme)), encoding='utf-8')


if __name__ == '__main__':
    root = Path(__file__).resolve().parents[2]
    main(Path(sys.argv[1]) if len(sys.argv) > 1 else root / 'docs/assets/diagrams')
