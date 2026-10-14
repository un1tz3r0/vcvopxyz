"""Prints the centre (mm) of each named component on a panel's hidden components layer, for click scripts:
python3 ids.py res/Sequencer.svg STEP3_PARAM PLAY_PARAM"""
import re, sys
svg = open(sys.argv[1]).read()
for name in sys.argv[2:]:
    m = re.search(rf'<rect id="{name}"[^>]* x="([\d.]+)" y="([\d.]+)" width="([\d.]+)" height="([\d.]+)"', svg)
    if m:
        x, y, w, h = map(float, m.groups())
        print(x + w / 2, y + h / 2)
    else:
        print(*re.search(rf'<circle id="{name}"[^>]* cx="([\d.]+)" cy="([\d.]+)"', svg).groups())
