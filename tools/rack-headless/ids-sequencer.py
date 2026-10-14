"""Prints the centre (mm) of a component on the Sequencer panel: ids.py STEP3_PARAM"""
import re, sys
svg = open('/home/claude/vcvopxyz/res/Sequencer.svg').read()
for name in sys.argv[1:]:
    m = re.search(rf'<rect id="{name}"[^>]* x="([\d.]+)" y="([\d.]+)" width="([\d.]+)" height="([\d.]+)"', svg)
    if m:
        x, y, w, h = map(float, m.groups()); print(x + w / 2, y + h / 2)
    else:
        m = re.search(rf'<circle id="{name}"[^>]* cx="([\d.]+)" cy="([\d.]+)"', svg); print(*m.groups())
