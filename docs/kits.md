# Kits

<picture><source media="(prefers-color-scheme: dark)" srcset="assets/guide/kit-screen-dark.png"><img alt="A Screen editing the kick track's kit" src="assets/guide/kit-screen-light.png"></picture>

A kit is the plug of a [drum track](drum-tracks.md). It has 24 sounds, one for each key from F3. Each sound is a
slice of one long sample, with its own tuning, level, direction and play mode.

Each drum track has ten plug slots. To fill one, right-click the Sequencer, open **Plug slots**, then the track and the
plug slot. Choose one of these:

| Kind | From | A patch saves it as |
| --- | --- | --- |
| [OP-Z kit](#op-z-kits) | Sampled from an OP-Z, and part of the plugin. | Its name. |
| [Built-in kit](#built-in-kits) | Made by the plugin. | Its name. |
| [A kit file](#kit-files) | An OP-1 or OP-Z kit, any AIFF or WAV, or a folder of sounds. | Its sounds, in the patch's storage folder. |

Each kit that you put in a plug slot is one step of Rack's undo.

## OP-Z kits

The OP-Z's own kits, sampled from the device. Each is one WAV with its slices, in the plugin's `res/kits` folder.

| Track | Kits |
| --- | --- |
| Kick | Kick 1, Kick 2, Kick 3, Kick 4 |
| Snare | Snare 1, Snare 2, Snare 3, Snare 4 |
| Perc | Hat 1, Hat 2, Hat 3, Hat 4, Hat 5 |

You can put any of them on any drum track. Some sounds in these first samples are missing or too short. They are pad
16 of the snare and hat kits, five pads of Hat 5, and a few sounds that ring into the next. These play nothing or end
early until they are sampled again.

## Built-in kits

Each drum track has its own built-in kit, in the plug slot after its OP-Z kits. The plugin makes them when you add
the first Sequencer.

| Track | Sounds |
| --- | --- |
| Kick | 808s from long and deep to tight, 909s, rumble, hardcore, distorted, gabber, acoustic, soft, thud, felt, click, blip, sub, zap |
| Snare | 808, 909, tight, fat, crack, lo-fi, deep and noise snares, rims and a sidestick, 808, 909, tight and big claps, snap, brushes, slap, piccolo, swell, electro, trash |
| Perc | Closed, pedal and open hats, crash, ride, china, four toms, congas, bongo, cowbell, clave, woodblock, shaker, tambourine, triangle |
| Sample | Zaps, laser, drop, blips on C, E, G and C, FM bell, glass, pluck and gong, 808 notes on C, D♯ and G, sub drop, noise burst, swoosh, static, thunder, glitch, robot, chirp, hit |

## Kit files

A plug slot's menu has these items:

| Item | Does |
| --- | --- |
| **Load a kit file…** | Loads an OP-1 or OP-Z kit, which keeps its slices, tuning, volume, direction and play modes. Any other AIFF or WAV becomes 24 equal slices. |
| **Load a folder of sounds…** | Loads up to 24 AIFF and WAV files, one for each key, in the order of their names. |
| **Save as a kit file…** | Writes the kit as an OP-1 and OP-Z kit: a 16-bit AIFF at 44.1 kHz, with its slices. |

- The Sequencer mixes stereo files to mono. Sounds from a folder are resampled to 44.1 kHz.
- A file's sample rate must be between 1 and 384 kHz. Samples that are not numbers play as silence.
- A kit file holds 12 seconds. A saved kit that is longer stops at 12 seconds.
- Saved kit files load back here unchanged. Nobody has tried them on an OP-Z yet.

A loaded kit saves with the patch, in the patch's storage folder, so the patch plays it on any computer. A Sequencer
that you paste with Ctrl+V has no storage folder yet, so its loaded kits are empty. Ctrl+D (duplicate) and a saved
patch keep them.

## Edit a kit

A [Screen](screen.md#kit-editor) edits the kit of the selected drum track: each slice's ends, tuning, level, play
mode and direction. An edit to an OP-Z kit or a built-in kit makes a copy of it in its plug slot. The copy saves with
the patch, as a loaded kit does.
