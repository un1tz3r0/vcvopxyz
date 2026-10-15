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
TRACKS = ['kick', 'snare', 'perc', 'sample', 'bass', 'lead', 'arp', 'chord',
          'fx1', 'fx2', 'tape', 'master', 'perform', 'module', 'lights', 'motion']
COMPONENTS = ['pulse', 'pulse_hold', 'multiply', 'velocity', 'ramp_up', 'ramp_down', 'random',
              'portamento', 'sweep', 'tonality', 'jump', 'parameter_spark', 'component_spark', 'trigger_spark']


def icon(name, x, y, size, color, background='{panel}'):
    """ICONS[name], `size` mm across, centred on (x, y)."""
    scale = size / 2
    body = ''.join(ICONS[name]).replace('@', color).replace('%', background)
    return (f'<g transform="translate({num(x)} {num(y)}) scale({num(scale)})" fill="none" stroke="{color}" '
            f'stroke-width="{num(STROKE / scale)}" stroke-linecap="round" stroke-linejoin="round">{body}</g>')


SCALE = 2.16    # how much bigger than the OP-Z's own face the Sequencer draws it, keys and lit symbols included
KEY = 4.2       # a key's diameter (mm), before scaling
RING = 0.3      # its ring's line weight (mm)
NUMERAL = 2.7   # the black keys' numbers (mm)
# The symbols that are also LEDs, and how big each is drawn (mm).
SYMBOLS = dict(dot=2.0, rec=2.4, track=2.2, minus=2.2, plus=2.2, play=2.0, stop=1.9, shift=2.2, metronome=2.3,
               project=2.2, mixer=2.2, screen=2.3,
               kick=2.3, snare=2.3, perc=2.3, sample=2.3, bass=2.3, lead=2.3, arp=2.3, chord=2.3, master=2.3, fx1=2.3,
               fx2=2.3)


def symbol(name, x, y, color):
    """A key's symbol: a number for a black key, else one of SYMBOLS."""
    if name.isdigit():
        return f'<path d="{NUNITO.path(name, x, y, NUMERAL)}" fill="{color}"/>'
    return icon(name, x, y, SYMBOLS[name], color)


def symbol_svg(name):
    """The lit symbol, white on a key-sized canvas at the Sequencer's scale, for the module to tint with its LED's
    colour. A lit dot glows bigger than the printed one."""
    body = icon('dot', KEY / 2, KEY / 2, 3.6, '#FFFFFF') if name == 'dot' else symbol(name, KEY / 2, KEY / 2, '#FFFFFF')
    return document(KEY * SCALE, KEY * SCALE, f'  <g transform="scale({num(SCALE)})">{body}</g>')


def round_key(p, name, x, y, sym='dot'):
    """A key printed as a ring around its symbol, and the box its widget takes."""
    p.art.append(f'<circle cx="{num(x)}" cy="{num(y)}" r="{num((KEY - RING) / 2)}" fill="none" stroke="{{ink}}" '
                 f'stroke-width="{num(RING)}"/>')
    p.art.append(symbol(sym, x, y, '{glyph}'))
    p.place(name, x - KEY / 2, y - KEY / 2, KEY, KEY)


def number_key(p, name, x, y, digit):
    """A black key: a solid disc with its number knocked out of it."""
    p.art.append(f'<circle cx="{num(x)}" cy="{num(y)}" r="{num(KEY / 2)}" fill="{{ink}}"/>')
    p.art.append(symbol(digit, x, y, '{panel}'))
    p.place(name, x - KEY / 2, y - KEY / 2, KEY, KEY)
