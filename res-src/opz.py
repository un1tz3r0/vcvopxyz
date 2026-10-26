"""The OP-Z's key symbols as line art, after the device's own: the track icons over the step keys, the step component
icons over the white keys, and the symbols on the function keys. Each is drawn in a 2 x 2 box around (0, 0), y down,
and `icon()` scales it into place in one colour. The same drawing is printed on a panel and, for the symbols that are
also LEDs, written white to res/components/symbols so the module can light it up in any colour."""
from panelkit import NUNITO, document, num

STROKE = 0.2  # line weight (mm), whatever the icon's size


def dot(x, y, r):
    return f'<circle cx="{num(x)}" cy="{num(y)}" r="{num(r)}" fill="@" stroke="none"/>'


def line(d):
    return f'<path d="{d}"/>'


def solid(d):
    return f'<path d="{d}" fill="@" stroke="none"/>'


def text(s, x, y, size):
    """Text from the panel font; a knocked-out letter is filled with % for the background instead."""
    return solid(NUNITO.path(s, x, y, size))


def boxed(s):
    return [line('M-1 0H-0.6M0.6 0H1'), line('M-0.6 -0.75H0.6V0.75H-0.6Z'), text(s, 0, 0, 1.1)]


def note(x, y):
    return [f'<ellipse cx="{num(x)}" cy="{num(y)}" rx="0.34" ry="0.25" transform="rotate(-25 {num(x)} {num(y)})" fill="@" stroke="none"/>',
            line(f'M{num(x + 0.3)} {num(y - 0.08)}V{num(y - 1.3)}')]


ICONS = {
    # The tracks, over the step keys.
    'kick': ['<circle cx="0" cy="0.2" r="0.72"/>', line('M0 -1V0.2'), dot(0, 0.2, 0.14)],
    'snare': [line('M-0.75 -0.95H0.75V-0.25H-0.75Z'), line('M0 -0.25V1')],
    'perc': [line('M0 -1V1'), line('M-0.85 -0.25H0.85'), line('M-0.35 -0.25Q0 -0.7 0.35 -0.25'), line('M-0.85 0.15H0.85')],
    'sample': [line('M-1 0H-0.7M0.7 0H1'), line('M-0.4 -0.45V0.45M0 -0.85V0.85M0.4 -0.6V0.6')],
    'bass': [text('B', 0, 0, 1.9)],
    'lead': note(-0.25, 0.65),
    'arp': [dot(-0.7, 0.4, 0.17), dot(-0.05, 0.0, 0.17), dot(0.62, -0.38, 0.23)],
    'chord': [text('CH', 0, 0, 1.6)],
    'fx1': boxed('1'),
    'fx2': boxed('2'),
    'tape': ['<circle cx="-0.5" cy="-0.05" r="0.38"/>', '<circle cx="0.5" cy="-0.05" r="0.38"/>', line('M-0.5 0.33H0.5')],
    'master': [text('M', 0, 0, 1.8)],
    'perform': ['<circle cx="0" cy="-0.75" r="0.22"/>', line('M-0.75 -0.3H0.75M0 -0.3V0.3L-0.45 1M0 0.3L0.45 1')],
    'module': [line('M-0.7 -0.75H0.2L0.7 -0.25V0.9H-0.7Z'), line('M0.2 -0.75V-0.25H0.7'), text('C', -0.05, 0.15, 1.0)],
    'lights': ['<circle cx="0" cy="-0.35" r="0.5"/>', line('M-0.22 0.15V0.5H0.22V0.15M-0.15 0.85H0.15')],
    'motion': [solid('M-0.95 -0.5H0.3V0.5H-0.95Z'), solid('M0.3 0L0.95 -0.45V0.45Z')],
    # The kinds of track module, on their panels' tags: a synth voice's wave, a drum, and a send out and back.
    'synth': [line('M-1 0C-0.75 -1.1 -0.25 -1.1 0 0S0.75 1.1 1 0')],
    'drum': [line('M-0.85 -0.35V0.45A0.85 0.35 0 0 0 0.85 0.45V-0.35'), '<ellipse cx="0" cy="-0.35" rx="0.85" ry="0.35"/>'],
    'send': [line('M-1 0H-0.6M0.6 0H1'), line('M-0.6 -0.75H0.6V0.75H-0.6Z')],
    # The plugs, on the Sequencer's display: each synth engine's sound, the drum, each effect, and the breakouts' jack.
    'plug_Saw': [line('M-1 0.6L0 -0.6V0.6L1 -0.6')],
    'plug_Volt': [line('M-1 0.6V-0.6H-0.2V0.6H0.6V-0.6H1')],
    'plug_Digital': [line('M-1 0.6H-0.6V0H-0.2V-0.6H0.2V0H0.6V0.6H1')],
    'plug_Analog': [line('M-1 -0.3H-0.1Q0.2 -0.3 0.3 -0.65Q0.45 -0.1 1 0.75')],
    'plug_Cluster': [line(f'M-1 {y + 0.25}L-0.3 {y - 0.25}V{y + 0.25}L0.4 {y - 0.25}V{y + 0.25}L1 {y - 0.25}') for y in (-0.6, 0, 0.6)],
    'plug_Organ': [line('M-0.75 1V-0.1M-0.25 1V-0.8M0.25 1V0.2M0.75 1V-0.45'),
                   dot(-0.75, -0.1, 0.17), dot(-0.25, -0.8, 0.17), dot(0.25, 0.2, 0.17), dot(0.75, -0.45, 0.17)],
    'plug_Bow': [line('M-0.95 0.75Q-0.1 -0.25 0.75 -0.95'), line('M-0.75 0.95L0.95 -0.75')],
    'plug_Electric': [line('M-1 0Q-0.83 -0.8 -0.67 0T-0.33 0T0 0T0.33 0T0.67 0T1 0')],
    'plug_Shade': ['<circle cx="0" cy="0" r="0.75"/>', solid('M0 -0.75A0.75 0.75 0 0 1 0 0.75Z')],
    'plug_Uranus': ['<circle cx="0" cy="0" r="0.5"/>', '<ellipse cx="0" cy="0" rx="1" ry="0.28" transform="rotate(-20)"/>'],
    'plug_EP': [line('M-0.9 -0.7H0.9V0.7H-0.9Z'), line('M-0.3 -0.7V0.7M0.3 -0.7V0.7'),
                solid('M-0.42 -0.7H-0.18V0.1H-0.42Z'), solid('M0.18 -0.7H0.42V0.1H0.18Z')],
    'plug_Drum': [line('M-0.85 -0.35V0.45A0.85 0.35 0 0 0 0.85 0.45V-0.35'), '<ellipse cx="0" cy="-0.35" rx="0.85" ry="0.35"/>'],
    'plug_Delay': [dot(-0.68, 0, 0.3), dot(0, 0, 0.22), dot(0.52, 0, 0.15), dot(0.88, 0, 0.09)],
    'plug_Reverb': [dot(-0.75, 0, 0.17), line('M-0.35 -0.35A0.5 0.5 0 0 1 -0.35 0.35'), line('M0.05 -0.65A0.92 0.92 0 0 1 0.05 0.65'),
                    line('M0.45 -0.9A1.27 1.27 0 0 1 0.45 0.9')],
    'plug_Rymd': [solid('M0 -0.95Q0.12 -0.12 0.95 0Q0.12 0.12 0 0.95Q-0.12 0.12 -0.95 0Q-0.12 -0.12 0 -0.95Z')],
    'plug_Chorus': [line('M-1 -0.2C-0.75 -0.9 -0.25 -0.9 0 -0.2S0.75 0.5 1 -0.2'), line('M-1 0.3C-0.75 -0.4 -0.25 -0.4 0 0.3S0.75 1 1 0.3')],
    'plug_Crush': [line('M-1 0.8H-0.5V0.3H0V-0.2H0.5V-0.7H1')],
    'plug_Dist': [line('M-1 0.3L-0.65 -0.55H-0.1L0.25 0.55H0.75L1 0.1')],
    'plug_Breakout': ['<circle cx="-0.3" cy="0.3" r="0.55"/>', dot(-0.3, 0.3, 0.2), line('M0.1 -0.1L0.95 -0.95')],
    # The step components, over the white keys.
    'pulse': [dot(-0.35, 0, 0.18), dot(0.35, 0, 0.18)],
    'pulse_hold': [line('M-0.75 0H0.75')],
    'multiply': [solid('M-0.95 -0.55H0.95V0.55H-0.95Z'), text('X2', 0, 0, 0.9).replace('fill="@"', 'fill="%"')],
    'velocity': [dot(-0.7, 0.35, 0.17), line('M-0.3 0.5L0.9 -0.35M-0.3 0.5H0.9')],
    'ramp_up': [line('M-0.95 0.55H0.95V-0.55Z')],
    'ramp_down': [line('M-0.95 -0.55V0.55H0.95Z')],
    'random': [line('M-0.95 0.55H-0.15V-0.55H0.75')],
    'portamento': [dot(-0.72, 0, 0.17), line('M-0.5 0H0.5'), dot(0.72, 0, 0.17)],
    'sweep': [line('M-0.45 0.9V0.0A0.45 0.45 0 0 1 0.45 0.0V0.25')],
    'tonality': note(-0.6, 0.7) + note(0.25, 0.55),
    'jump': [line('M-0.7 0.3A0.7 0.7 0 0 1 0.7 0.3'), dot(-0.7, 0.62, 0.16), dot(0.7, 0.62, 0.16)],
    'parameter_spark': [line('M0.25 -1L-0.35 0.1H0.3L-0.2 1')],
    'component_spark': [line('M-0.95 0.5H-0.25V-0.2H0.8M0.5 -0.5L0.8 -0.2L0.5 0.1')],
    'trigger_spark': [line('M-0.6 -0.7H0.25V0.75M-0.05 0.45L0.25 0.75L0.55 0.45')],
    # The function keys' symbols, which are also their LEDs.
    'track': [line('M-0.55 -0.7H0.55M-0.55 0.7H0.55M0 -0.7V0.7')],
    'rec': [dot(0, 0, 0.32)],
    'minus': [line('M-0.65 0H0.65')],
    'play': [line('M-0.45 -0.65L0.65 0L-0.45 0.65Z')],
    'plus': [line('M-0.65 0H0.65M0 -0.65V0.65')],
    'stop': [line('M-0.55 -0.55H0.55V0.55H-0.55Z')],
    'shift': [line('M0 0.7V-0.7M-0.5 -0.2L0 -0.7L0.5 -0.2')],
    'dot': [dot(0, 0, 0.4)],
    'metronome': [line('M-0.55 0.9L-0.22 -0.9H0.22L0.55 0.9Z'), line('M-0.4 0.45H0.4'), line('M0 0.45L0.65 -0.55')],
    # The index keys over the display: project, mixer, tempo (the metronome) and screen, which shows the battery.
    'project': [line('M-0.85 -0.55H-0.2L0.05 -0.8H0.85V0.75H-0.85Z'), line('M-0.85 -0.3H0.85')],
    'mixer': [line('M-0.75 -0.25V0.25M-0.25 -0.75V0.75M0.25 -0.75V0.75M0.75 -0.25V0.25')],
    'screen': [line('M-0.9 -0.45H0.65V0.45H-0.9Z'), solid('M0.75 -0.2H0.95V0.2H0.75Z'), solid('M-0.7 -0.25H0.2V0.25H-0.7Z')],
}
PLUGS = ['Saw', 'Volt', 'Digital', 'Analog', 'Cluster', 'Organ', 'Bow', 'Electric', 'Shade', 'Uranus', 'EP', 'Drum', 'Delay',
         'Reverb', 'Rymd', 'Chorus', 'Crush', 'Dist', 'Breakout']
TRACKS = ['kick', 'snare', 'perc', 'sample', 'bass', 'lead', 'arp', 'chord',
          'fx1', 'fx2', 'tape', 'master', 'perform', 'module', 'lights', 'motion']
COMPONENTS = ['pulse', 'pulse_hold', 'multiply', 'velocity', 'ramp_up', 'ramp_down', 'random',
              'portamento', 'sweep', 'tonality', 'jump', 'parameter_spark', 'component_spark', 'trigger_spark']


def icon(name, x, y, size, color, background='{panel}', edge=0.0):
    """ICONS[name], `size` mm across, centred on (x, y). `edge` grows its lines and shapes by that many mm all round."""
    scale = size / 2
    body = ''.join(ICONS[name]).replace('@', color).replace('%', background)
    if edge:
        body = body.replace(' stroke="none"', '')
    return (f'<g transform="translate({num(x)} {num(y)}) scale({num(scale)})" fill="none" stroke="{color}" '
            f'stroke-width="{num((STROKE + 2 * edge) / scale)}" stroke-linecap="round" stroke-linejoin="round">{body}</g>')


SCALE = 2.16    # how much bigger than the OP-Z's own face the Sequencer draws it, keys and lit symbols included
KEY = 4.2       # a key's diameter (mm), before scaling
RING = 0.3      # its ring's line weight (mm)
NUMERAL = 2.7   # the black keys' numbers (mm)
# The symbols that are also LEDs, and how big each is drawn (mm). The keys draw them, off and lit, so that their look
# can follow the Sequencer's Appearance menu; the step and white keys' dots are circles drawn in code.
SYMBOLS = dict(rec=2.4, track=2.2, minus=2.2, plus=2.2, play=2.0, stop=1.9, shift=2.2, metronome=2.3,
               project=2.2, mixer=2.2, screen=2.3,
               kick=2.3, snare=2.3, perc=2.3, sample=2.3, bass=2.3, lead=2.3, arp=2.3, chord=2.3, master=2.3, fx1=2.3,
               fx2=2.3, **{f'plug_{name}': 3.6 for name in PLUGS})
# The round keys' symbols, which also get an edge: the symbol grown by EDGE (mm, on the panel), for the dark keyline
# some LED styles draw behind a lit one.
KEY_SYMBOLS = ['track', 'rec', 'minus', 'play', 'plus', 'stop', 'shift', 'project', 'mixer', 'metronome', 'screen',
               *'0123456789']
EDGE = 0.3


def symbol(name, x, y, color, edge=0.0):
    """A key's symbol: a number for a black key, else one of SYMBOLS, its outline grown by `edge` mm."""
    if name.isdigit():
        grow = f' stroke="{color}" stroke-width="{num(2 * edge)}" stroke-linejoin="round"' if edge else ''
        return f'<path d="{NUNITO.path(name, x, y, NUMERAL)}" fill="{color}"{grow}/>'
    return icon(name, x, y, SYMBOLS[name], color, edge=edge)


def symbol_svg(name, edge=0.0):
    """A symbol, white on a key-sized canvas at the Sequencer's scale, for the module to draw in any colour. `edge` is
    in the panel's mm."""
    body = symbol(name, KEY / 2, KEY / 2, '#FFFFFF', edge / SCALE)
    return document(KEY * SCALE, KEY * SCALE, f'  <g transform="scale({num(SCALE)})">{body}</g>')


def round_key(p, name, x, y):
    """A key's ring, and the box its widget takes. The widget draws the symbol inside."""
    p.art.append(f'<circle cx="{num(x)}" cy="{num(y)}" r="{num((KEY - RING) / 2)}" fill="none" stroke="{{ink}}" '
                 f'stroke-width="{num(RING)}"/>')
    p.place(name, x - KEY / 2, y - KEY / 2, KEY, KEY)


def number_key(p, name, x, y):
    """A black key's disc, and the box its widget takes. The widget draws the number on it."""
    p.art.append(f'<circle cx="{num(x)}" cy="{num(y)}" r="{num(KEY / 2)}" fill="{{ink}}"/>')
    p.place(name, x - KEY / 2, y - KEY / 2, KEY, KEY)
