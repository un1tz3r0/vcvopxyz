#!/usr/bin/env python3
"""Expands image shorthands in Markdown files in place into <picture>s with light and dark versions:

    [[key:track-track|TRACK]]           docs/assets/keys/track-track-{light,dark}.png
    [[key:track-track|TRACK|36]]        the same, 36 px high
    [[face:off|Alt]]                    docs/assets/keys/face-off-*.png
    [[module:Digital-small|Digital]]    docs/assets/modules/Digital-small-*.png
    [[guide:bass|Alt]]                  docs/assets/guide/bass-*.png
    [[plug:Saw|Alt|36]]                 docs/assets/plugs/Saw-*.png

and with --prune, deletes the images under docs/assets/{keys,modules,guide,plugs} that no Markdown file refers to.

    expand.py <plugin dir> [--prune] file.md ...
"""
import os, re, sys
from pathlib import Path

DIRS = dict(key='keys', dial='keys', face='keys', module='modules', guide='guide', plug='plugs')
SHORTHAND = re.compile(r'\[\[(\w+):([\w-]+)\|([^|\]]+)(?:\|(\d+))?\]\]')

plugin = Path(sys.argv[1])
prune = '--prune' in sys.argv
files = [Path(f) for f in sys.argv[2:] if f != '--prune']
assets = plugin / 'docs/assets'


def picture(md, kind, name, alt, height):
    stem = f'face-{name}' if kind == 'face' else name
    base = assets / DIRS[kind] / stem
    for theme in ('light', 'dark'):
        if not base.with_name(f'{stem}-{theme}.png').exists():
            sys.exit(f'{md}: no {base}-{theme}.png')
    rel = os.path.relpath(base, md.parent)
    size = f' height="{height}"' if height else ''
    return (f'<picture><source media="(prefers-color-scheme: dark)" srcset="{rel}-dark.png">'
            f'<img alt="{alt}" src="{rel}-light.png"{size}></picture>')


CODE = re.compile(r'(```.*?```|`[^`\n]*`)', re.S)

for md in files:
    text = md.read_text()
    # Shorthands inside code are left alone, so the manual can show them.
    expanded = ''.join(part if i % 2 else SHORTHAND.sub(lambda m: picture(md, *m.groups()), part)
                       for i, part in enumerate(CODE.split(text)))
    if expanded != text:
        md.write_text(expanded)
        print(md, flush=True)

if prune:
    used = set()
    for md in plugin.glob('**/*.md'):
        used |= {m for m in re.findall(r'assets/((?:keys|modules|guide|plugs)/[\w-]+\.png)', md.read_text())}
    for d in ('keys', 'modules', 'guide', 'plugs'):
        for png in (assets / d).glob('*.png'):
            if f'{d}/{png.name}' not in used:
                png.unlink()
    print(f'kept {len(used)} images')
