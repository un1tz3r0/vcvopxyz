"""Remotes: jacks that play the Sequencer's patterns, on its left. LINK lights once it finds one to its right; the
inputs pick patterns and saved chains, BAR holds them for the next bar, MODE switches to songs, and the outputs pulse
as patterns start and end."""
from panelkit import HP, Panel
from synth_track import box, jacks

WIDTH, HEIGHT = 8 * HP, 128.5
LABELS = dict(SONG_END='SONG END', PATTERN='PATTERN')


def panel():
    p = Panel(WIDTH, HEIGHT)
    p.rect(0, 0, WIDTH, HEIGHT, 0, 'panel')
    p.title('Remotes')
    p.place('LINK_LIGHT', WIDTH / 2 - 4.0, 16.0)
    p.text('LINK', WIDTH / 2 + 1.6, 16.0 + 0.8, size=2.2, anchor='middle')
    columns = [WIDTH / 2 - 6.6, WIDTH / 2 + 6.6]
    rows = [28.0, 42.0, 56.0, 70.0, 94.0, 108.5]
    box(p, columns, 0, 1, rows[4], rows[5])
    ins = lambda *names: [(n, 'INPUT') for n in names]
    outs = lambda *names: [(n, 'OUTPUT') for n in names]
    jacks(p, [(rows[0], ins('SELECT', 'BAR')), (rows[1], ins('PREV', 'NEXT')), (rows[2], ins('SONG', 'LOAD')),
              (rows[3], ins('MODE') + [None]), (rows[4], outs('START', 'END')), (rows[5], outs('SONG_END', 'PATTERN'))], columns, labels=LABELS)
    return p
