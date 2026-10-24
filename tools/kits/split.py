#!/usr/bin/env python3
"""Cuts bounces of the OP-Z's drum kits into the plugin's kits.

Each folder in samples/ is one kit, named <kind>-kit<n> (kick, snare, hat or perc, sample or fx), holding its 24
sounds bounced at their own pitch with the envelope wide open, one after another on a steady grid: hits-01-16.wav and
hits-17-24.wav. A kit.json there can name other files and the pads each holds, in order, as
{"files": {"pads-16.wav": [16]}}; a later file's pad replaces an earlier one's.

Between the hits the OP-Z puts out digital silence, so each hit runs from the first sound after a silence to its last
sample above TAIL. The grid finds the hits that start before the last one has gone quiet, and the pads that are
missing. Each kit becomes res/kits/<kind>-<n>.wav, its sounds back to back in mono, and res/kits/<kind>-<n>.json, their
slices, as Kit::fromSaved reads a patch's kit. The report flags every pad that was missing, or still sounding when
the next one started or the file ended.

    python3 tools/kits/split.py [samples] [res/kits] [--previews DIR]
"""
import argparse
import json
import re
import wave
from pathlib import Path

import numpy as np

TRACKS = {'kick': 0, 'snare': 1, 'hat': 2, 'perc': 2, 'sample': 3, 'fx': 3}
SILENCE = 10 ** (-80 / 20)  # the OP-Z's digital silence, give or take dither
TAIL = 10 ** (-72 / 20)     # a sound ends after its last sample above this
RINGING = 10 ** (-60 / 20)  # still sounding, when the next hit or the file's end comes
GAP = 0.020                 # seconds of silence before a hit
FADE = 0.005                # seconds of fade at each sound's end
FADE_IN = 0.001             # and at its start, where it doesn't start from silence
SHORTEST = 0.050            # a sound cut off sooner than this is left out
SLICE = [1.0, 1.0, 1, False]  # speed, gain, one-shot, forwards


def read(path):
    with wave.open(str(path)) as w:
        if w.getsampwidth() != 2:
            raise SystemExit(f'{path}: only 16-bit WAV is read')
        a = np.frombuffer(w.readframes(w.getnframes()), '<i2').reshape(-1, w.getnchannels())
        return a.astype(np.float64).mean(1) / 32768, w.getframerate()


def onsets(x, rate):
    """Every sample that sounds after at least GAP of silence, or at the very start."""
    loud = np.flatnonzero(np.abs(x) > SILENCE)
    if not len(loud):
        return loud
    gaps = np.diff(loud) > GAP * rate
    return np.concatenate(([loud[0]], loud[1:][gaps]))


def grid(found, count, length, rate):
    """The first beat and the spacing that puts the most onsets on beats, preferring one whose `count` beats all fit
    in the file, then the widest: half of it puts them all on beats too, and so may twice it, past silent pads."""
    tolerance = 0.003 * rate
    best = (False, 0, 0.0)
    for j in range(1, min(len(found), 6)):
        for m in range(1, 9):
            step = (found[j] - found[0]) / m
            if step < 0.05 * rate:
                continue
            beats = (found - found[0]) / step
            score = np.sum(np.abs(beats - np.round(beats)) * step < tolerance)
            best = max(best, (found[0] + (count - 1) * (step - tolerance) < length, score, step))
    step = best[2]
    if not step:
        return found[0], length / count
    # Fitted to every onset on a beat, so the last beats land as well as the first.
    beats = np.round((found - found[0]) / step)
    on = (np.abs((found - found[0]) - beats * step) < tolerance) & (beats > 0)
    return found[0], np.sum((found[on] - found[0]) * beats[on]) / np.sum(beats[on] ** 2) if on.any() else step


def zero_before(x, i, rate):
    """The last zero or zero crossing at or before i, at most 2 ms back."""
    for j in range(i, max(i - int(0.002 * rate), 1), -1):
        if x[j - 1] == 0 or (x[j - 1] < 0) != (x[j] < 0):
            return j - 1 if abs(x[j - 1]) <= abs(x[j]) else j
    return i


def hits(x, rate, count):
    """Each pad's (start, end, flags), or None for a pad with no sound, `count` of them on the file's grid."""
    found = onsets(x, rate)
    if not len(found):
        return [None] * count
    first, step = grid(found, count, len(x), rate)
    starts = []
    for k in range(count):
        beat = first + k * step
        near = found[np.abs(found - beat) < 0.010 * rate]
        if len(near):
            starts.append(int(near[0]))
            continue
        window = np.abs(x[int(max(beat - 0.010 * rate, 0)):int(min(beat + step / 2, len(x)))])
        # A hit with no silence before it starts on its beat, if anything sounds there at all.
        starts.append(int(round(beat)) if len(window) and window.max() > SILENCE * 4 and beat < len(x) else None)
    out = []
    for k, s in enumerate(starts):
        if s is None:
            out.append(None)
            continue
        # The last hit ends where the next sound starts, if the bounce looped round to another.
        later = found[found > first + (k + 0.5) * step]
        limit = next((t for t in starts[k + 1:] if t is not None), int(later[0]) if len(later) else len(x))
        sound = np.flatnonzero(np.abs(x[s:limit]) > TAIL)
        end = s + (int(sound[-1]) + 1 if len(sound) else 1)
        flags = []
        if np.abs(x[max(limit - int(GAP * rate), s):limit]).max(initial=0) > RINGING:
            flags.append('ends the file still sounding' if limit == len(x) else 'still sounding at the next hit')
        out.append((zero_before(x, s, rate), end, flags))
    return out


def pads(folder):
    """Which pads each file holds, in the order they're cut."""
    manifest = folder / 'kit.json'
    if manifest.exists():
        return [(folder / f, p) for f, p in json.loads(manifest.read_text())['files'].items()]
    return [(folder / 'hits-01-16.wav', list(range(1, 17))), (folder / 'hits-17-24.wav', list(range(17, 25)))]


def cut(folder):
    """A kit's 24 sounds (None for a missing one), its rate, and a report line for each pad worth flagging."""
    sounds, sources, report, rate = [None] * 24, [None] * 24, {}, None
    for path, held in pads(folder):
        x, r = read(path)
        if rate not in (None, r):
            raise SystemExit(f'{path}: {r} Hz, but the kit is {rate} Hz')
        rate = r
        for pad, hit in zip(held, hits(x, r, len(held))):
            if hit is None:
                report[pad] = 'missing: nothing sounds on its beat'
                continue
            start, end, flags = hit
            if end - start < SHORTEST * r and flags:
                report[pad] = f'cut off after {1000 * (end - start) / r:.0f} ms, so left out'
                continue
            y = x[start:end].copy()
            fade = min(int(FADE * r), len(y))
            if fade:
                y[-fade:] *= np.linspace(1, 0, fade, endpoint=False)
            # A sound that starts under another's tail, or at the file's first sample, fades in too, quickly.
            if abs(y[0]) > TAIL:
                rise = min(int(FADE_IN * r), len(y))
                y[:rise] *= np.linspace(0, 1, rise, endpoint=False)
            sounds[pad - 1], sources[pad - 1] = y, {'file': path.name, 'from': start, 'to': end}
            if flags:
                report[pad] = ', '.join(flags)
            else:
                report.pop(pad, None)
    return sounds, sources, rate or 44100, report


def write(kit, sounds, sources, rate, out):
    pcm, slices, at = [], [], 0
    for y in sounds:
        n = 0 if y is None else len(y)
        if n:
            pcm.append(np.round(np.clip(y, -1, 32767 / 32768) * 32768).astype('<i2'))
        slices.append([at, at + n, *SLICE, ''])
        at += n
    with wave.open(str(out / f'{kit["id"]}.wav'), 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(rate)
        w.writeframes(np.concatenate(pcm).tobytes() if pcm else b'')
    meta = {'name': kit['name'], 'track': kit['track'], 'slices': slices, 'sources': sources}
    (out / f'{kit["id"]}.json').write_text(json.dumps(meta, indent='\t') + '\n')
    return at / rate


def preview(sounds, rate, path):
    """Every sound in turn, a quarter second apart, to listen through."""
    gap = np.zeros(rate // 4)
    y = np.concatenate([np.concatenate((s, gap)) for s in sounds if s is not None] or [gap])
    with wave.open(str(path), 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(rate)
        w.writeframes(np.round(np.clip(y, -1, 32767 / 32768) * 32768).astype('<i2').tobytes())


def main():
    p = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    p.add_argument('samples', nargs='?', default='samples', type=Path)
    p.add_argument('out', nargs='?', default='res/kits', type=Path)
    p.add_argument('--previews', type=Path, help='also write each kit as one WAV to listen through')
    args = p.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    if args.previews:
        args.previews.mkdir(parents=True, exist_ok=True)
    for folder in sorted(f for f in args.samples.iterdir() if f.is_dir()):
        m = re.fullmatch(r'([a-z]+)-kit(\d+)', folder.name)
        if not m or m[1] not in TRACKS:
            print(f'{folder.name}: skipped, not <kind>-kit<n>')
            continue
        kit = {'id': f'{m[1]}-{m[2]}', 'name': f'{m[1].capitalize()} {m[2]}', 'track': TRACKS[m[1]]}
        sounds, sources, rate, report = cut(folder)
        seconds = write(kit, sounds, sources, rate, args.out)
        if args.previews:
            preview(sounds, rate, args.previews / f'{kit["id"]}.wav')
        print(f'{kit["name"]}: {sum(s is not None for s in sounds)} sounds, {seconds:.2f} s')
        for pad, line in sorted(report.items()):
            print(f'  pad {pad}: {line}')


if __name__ == '__main__':
    main()
