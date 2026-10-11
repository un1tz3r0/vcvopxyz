#!/usr/bin/env python3
"""Generates every module's panels (light and dark) and the shared knob and button graphics into ../res.

A module's panel is the Panel returned by `panel()` in panels/<slug>.py, written to res/<slug>.svg and
res/<slug>-dark.svg. Move a control there, re-run, `make install`, and the artwork, the widget and its screen
readout all follow without touching C++.

Needs fontTools:  python3 -m pip install fonttools
"""
import importlib

from opz import SYMBOLS, symbol_svg
from panelkit import (BUTTON_R, DIAL_R, DIALS, KNOB_R, PAGE_KEY, PASTELS, ROOT, SMALL_DIAL_R, SOFT_R,
                      THEMES, button, dial_bg, dial_pointer, key, knob_bg, knob_pointer, write)

if __name__ == '__main__':
    for path in sorted((ROOT / 'res-src/panels').glob('*.py')):
        panel = importlib.import_module(f'panels.{path.stem}').panel()
        for suffix, theme in THEMES.items():
            write(ROOT / f'res/{path.stem}{suffix}.svg', panel.svg(theme))
    components = ROOT / 'res/components'
    for name, color in PASTELS.items():
        write(components / f'PastelKnob_bg-{name}.svg', knob_bg(KNOB_R, color))
    write(components / 'PastelKnob.svg', knob_pointer(KNOB_R, (0.25, 0.7), 0.8))
    write(components / 'SoftKnob_bg.svg', knob_bg(SOFT_R, '#E2E1DC'))
    write(components / 'SoftKnob.svg', knob_pointer(SOFT_R, (0.25, 0.72), 0.6))
    write(components / 'PushButton_0.svg', button(BUTTON_R, '#FAFAF7'))
    write(components / 'PushButton_1.svg', button(BUTTON_R, '#CFCEC9'))
    for kind, r in dict(Dial=DIAL_R, SmallDial=SMALL_DIAL_R).items():
        for name, color in DIALS.items():
            write(components / f'{kind}_bg-{name}.svg', dial_bg(r, color))
        write(components / f'{kind}.svg', dial_pointer(r))
    for name in [*SYMBOLS, *'0123456789']:
        write(components / f'symbols/{name}.svg', symbol_svg(name))
    for pressed in range(2):
        write(components / f'PageKey_{pressed}.svg', key(PAGE_KEY, '#EDECE8', pressed))
