# Drum tracks

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="assets/drums-dark.png">
  <img alt="Four Drums, playing tracks 1 to 4, with a Screen beside the kick" src="assets/drums-light.png" width="540">
</picture>

**Drum** is the OP-Z's drum track: a kit of 24 sounds, one per key. The first Drum to the right of a
[Sequencer](sequencer.md) plays track 1 (kick), the next track 2 (snare), then 3 (perc) and 4 (sample), whatever
synth engines sit between them. It is the same track as a [synth track](synth-tracks.md), with the same status screen,
pages, LFO and jacks; this page covers only where it differs.

## Kits

- **Keys:** F3 to E5, the Sequencer's keys at their home octave, play the 24 sounds in turn, and the octaves either
  side play them again. A key's tooltip on the Sequencer names its sound.
- **Built in:** each track starts on a kit of its own, synthesized when the plugin loads, so nothing ships as
  samples. The context menu's **Built-in kit** plays another track's instead.

| Track | Sounds |
| --- | --- |
| Kick | 808s from long and deep to tight, 909s, rumble, hardcore, distorted, gabber, acoustic, soft, thud, felt, click, blip, sub, zap |
| Snare | 808, 909, tight, fat, crack, lo-fi, deep and noise snares, rims and a sidestick, 808, 909, tight and big claps, snap, brushes, slap, piccolo, swell, electro, trash |
| Perc | Closed, pedal and open hats, crash, ride, china, four toms, congas, bongo, cowbell, clave, woodblock, shaker, tambourine, triangle |
| Sample | Zaps, laser, drop, blips on C, E, G and C, FM bell, glass, pluck and gong, 808 notes on C, D# and G, sub drop, noise burst, swoosh, static, thunder, glitch, robot, chirp, hit |

- **Loading:** the context menu's **Load a kit file…** takes an OP-1 or OP-Z kit (an AIFF with the kit's slices in
  it), or any other AIFF or WAV, cut into 24 equal slices. **Load a folder of sounds…** takes up to 24 AIFF and WAV
  files, one per key in order of their names. Stereo files are mixed to mono, and everything is resampled to 44.1 kHz.
- **Saving:** a loaded kit saves with the patch, in its storage folder, so the patch plays it on any computer.
  Built-in kits are made again on loading.
- **Kit settings:** a kit file's own slices keep their tuning, volume, direction and play mode: a slice set to gate
  or loop plays that way whatever the note style.

## Pages

| Page | Dials |
| --- | --- |
| Sound (green) | **Pitch** (±12 semitones, every sound), **Reverse** (off to the left, on to the right), **Filter**, **Reso** |
| Envelope (blue) | **Attack**, **Decay**, **Sustain**, **Release**, starting at full sustain and a 40 ms release |
| LFO (yellow) | As a synth track's, with these targets: pitch, reverse, filter, resonance, attack, pitch (±2 octaves), pan, volume |
| Mix (red) | As a synth track's |
| Track (white) | **Style** (retrig, mono, gate, loop) on the second dial; drums have no portamento |

## Note styles

- **Retrig** plays each sound to its end, and its tail rings on under the next. As many sounds ring at once as the
  track has channels, and at least two, as on the OP-Z; past that, the oldest is cut off.
- **Mono** plays one sound at a time, cutting the last one off with a 3 ms fade.
- **Gate** stops each sound with its note: the envelope releases when the note ends, so the step's note length sets
  how much plays.
- **Loop** repeats each sound while its note is held, then plays it out to its end as the envelope releases.

In retrig and mono the envelope's release never starts, since a sound plays to its end: attack, decay and sustain
shape it. The Sequencer gives each drum track two channels to begin with, so two sounds on one step play together.
