#!/usr/bin/env python3
"""Generates every module's panels (light and dark) and the shared knob and button graphics into ../res.

A module's panel is the Panel returned by `panel()` in panels/<slug>.py, written to res/<slug>.svg and
res/<slug>-dark.svg. Move a control there, re-run, `make install`, and the artwork, the widget and its screen
readout all follow without touching C++.

Needs fontTools:  python3 -m pip install fonttools
"""
import importlib

from panelkit import (BLACK_KEY, BUTTON_R, DIAL_R, DIALS, FUNCTION_KEY, KNOB_R, PAGE_KEY, PASTELS, ROOT, SOFT_R, STEP_KEY,
                      THEMES, WHITE_KEY, button, dial_bg, dial_pointer, key, knob_bg, knob_pointer, write)

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
    for name, color in DIALS.items():
        write(components / f'Dial_bg-{name}.svg', dial_bg(DIAL_R, color))
    write(components / 'Dial.svg', dial_pointer(DIAL_R))
    white = '#EDECE8'
    keys = dict(WhiteKey=(WHITE_KEY, white, None, None), BlackKey=(BLACK_KEY, '#4A4B4F', None, None),
                PageKey=(PAGE_KEY, white, None, None), OctaveDown=(FUNCTION_KEY, white, '\u2212', None),
                OctaveUp=(FUNCTION_KEY, white, '+', None), StepKey=(STEP_KEY, white, None, None),
                FunctionKey=(FUNCTION_KEY, white, None, None), RecKey=(FUNCTION_KEY, white, None, 'rec'),
                PlayKey=(FUNCTION_KEY, white, None, 'play'), StopKey=(FUNCTION_KEY, white, None, 'stop'))
    for name, (size, face, glyph, icon) in keys.items():
        for pressed in range(2):
            write(components / f'{name}_{pressed}.svg', key(size, face, pressed, glyph, icon))
