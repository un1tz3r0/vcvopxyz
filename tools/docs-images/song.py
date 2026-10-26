"""The song that docs/guide.md writes, from the patterns of patches/opztest.vcv: render_guide.py's scenes play it,
and it writes it as patches/first-song.vcv."""
import io, json, tarfile
from pathlib import Path

STEPS, TEMPO, FX1_SEND = 16, 105, 0.5    # the arp's send to the delay
# Each track's notes, as {step: (pitch, length in ticks of a sixteenth's 24)}, steps from 1; length 0 is the track's.
NOTES = {
    'kick': {1: (66, 0), 9: (66, 0), 11: (66, 0), 14: (66, 0)},
    'snare': {5: (57, 0), 13: (57, 0)},
    'perc': {2: (72, 0), 3: (59, 0), 5: (73, 0), 7: (59, 0), 8: (73, 0), 9: (60, 0), 11: (59, 0), 12: (73, 0),
             14: (72, 0), 15: (59, 0), 16: (61, 0)},
    'bass': {1: (45, 0), 3: (54, 0), 5: (45, 0), 7: (57, 0), 9: (49, 0), 11: (61, 0), 13: (42, 0), 15: (57, 0)},
    'lead': {2: (49, 0), 3: (51, 0), 5: (52, 0), 7: (59, 0), 8: (61, 0), 10: (54, 0), 11: (56, 0), 13: (57, 0),
             15: (63, 0), 16: (64, 0)},
    'arp': {1: (57, 72), 3: (59, 72), 5: (63, 72), 7: (64, 0), 9: (66, 72), 11: (69, 72), 13: (63, 72), 15: (61, 48)},
}
TRACKS = ['kick', 'snare', 'perc', 'sample', 'bass', 'lead', 'arp', 'chord']
CHANNELS = [2, 2, 2, 2, 1, 3, 1, 4]


def pattern(name, notes=None):
    notes = NOTES.get(name, {}) if notes is None else notes
    return [[[notes[s][0], 100, 0, notes[s][1]]] if s in notes else [] for s in range(1, STEPS + 1)]


def tracks(*names):
    """The Sequencer's tracks with the named ones' notes, the rest empty."""
    return [{'pattern': pattern(n) if n in names else pattern(None), 'channels': c} for n, c in zip(TRACKS, CHANNELS)]


def write(path, modules, cables):
    """A Rack 2 patch: patch.json in a zstd-compressed tar."""
    import zstandard
    patch = json.dumps({'version': '2.6.6', 'modules': modules, 'cables': cables}, indent=1).encode()
    tar = io.BytesIO()
    with tarfile.open(fileobj=tar, mode='w', format=tarfile.USTAR_FORMAT) as t:
        info = tarfile.TarInfo('./patch.json')
        info.size, info.mode = len(patch), 0o644
        t.addfile(info, io.BytesIO(patch))
    Path(path).write_bytes(zstandard.ZstdCompressor(level=19).compress(tar.getvalue()))
