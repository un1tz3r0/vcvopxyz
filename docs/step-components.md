# Step components

<picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/face-components-dark.png"><img alt="Editing step components: steps 1, 5 and 9 chosen, with multiply and sweep on them" src="assets/keys/face-components-light.png"></picture>

A step component changes how a step plays. It does not change the step's notes. A step can have many components,
each with its own value.

## Add a component

1. Latch **SHIFT**: long-press it, or Ctrl+click it (⌘+click on a Mac).
2. Click the steps that get the component. They blink green. Steps that have components already glow green.
3. Click a white key. Each chosen step gets the component above that key, with a value that does something. Click
   the white key again to remove the component.
4. Click a black key to set the component's value. The black key lights green.
5. Click **SHIFT** to release it.

To clear every component on the track, click **STOP** while **SHIFT** is latched. To clear one step's components,
right-click the step and click **Clear step components**. A step's tooltip lists its components, and each key's
tooltip names its component or value. Each change is one step of Rack's undo.

## The components

White key 1 is the first component, and white key 14 is the last.

| White key | | Component | It makes the step |
| --- | --- | --- | --- |
| 1 | <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/key0-off-dark.png"><img alt="Pulse" src="assets/keys/key0-off-light.png" height="32"></picture> | **Pulse** | Play again for more than one slot before the playhead moves on. |
| 2 | <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/key2-off-dark.png"><img alt="Pulse hold" src="assets/keys/key2-off-light.png" height="32"></picture> | **Pulse hold** | Hold its note for more than one slot before the playhead moves on. |
| 3 | <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/key4-off-dark.png"><img alt="Multiply" src="assets/keys/key4-off-light.png" height="32"></picture> | **Multiply** | Play its notes several times inside the step, as a ratchet. |
| 4 | <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/key6-off-dark.png"><img alt="Velocity" src="assets/keys/key6-off-light.png" height="32"></picture> | **Velocity** | Play louder or softer. |
| 5, 6, 7 | <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/key7-off-dark.png"><img alt="Ramp up" src="assets/keys/key7-off-light.png" height="32"></picture> <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/key9-off-dark.png"><img alt="Ramp down" src="assets/keys/key9-off-light.png" height="32"></picture> <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/key11-off-dark.png"><img alt="Random" src="assets/keys/key11-off-light.png" height="32"></picture> | **Ramp up**, **ramp down**, **random** | Walk up, down or at random through the song's key from its note, one note for each trigger. |
| 8 | <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/key12-off-dark.png"><img alt="Portamento" src="assets/keys/key12-off-light.png" height="32"></picture> | **Portamento** | Glide into its note. |
| 9 | <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/key14-off-dark.png"><img alt="Sweep" src="assets/keys/key14-off-light.png" height="32"></picture> | **Sweep** | Move the filter, the sound or the pan, out and back over a bar. |
| 10 | <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/key16-off-dark.png"><img alt="Tonality" src="assets/keys/key16-off-light.png" height="32"></picture> | **Tonality** | Follow the [master track](master.md) in a different way. |
| 11 | <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/key18-off-dark.png"><img alt="Jump" src="assets/keys/key18-off-light.png" height="32"></picture> | **Jump** | Send the playhead to another step. |
| 12 | <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/key19-off-dark.png"><img alt="Parameter spark" src="assets/keys/key19-off-light.png" height="32"></picture> | **Parameter spark** | Play its [parameter locks](steps.md#parameter-locks) only on some passes. |
| 13 | <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/key21-off-dark.png"><img alt="Component spark" src="assets/keys/key21-off-light.png" height="32"></picture> | **Component spark** | Play its other components only on some passes. |
| 14 | <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/key23-off-dark.png"><img alt="Trigger spark" src="assets/keys/key23-off-light.png" height="32"></picture> | **Trigger spark** | Play its notes only on some passes. |

## The values

The black keys set the value. Black key 1 is the first value.

| Component | Black keys |
| --- | --- |
| Pulse, pulse hold | 1 to 9: that many slots. 0: a random number. |
| Multiply | 1 to 8: ×1 to ×8. 9: a broken chord. 0: quantize, which removes the micro timing. |
| Velocity | 1 to 4: softer. 5: no change. 6 to 8: louder. 9: mute. 0: random. |
| Ramp up, ramp down, random | 1 to 5: 2 to 6 notes over one octave. 6 to 0: 2 to 6 notes over three octaves. |
| Portamento | 1 to 8: glide 1 (short) to 8 (up to a beat). 9: direct. 0: random. |
| Sweep | 1, 2: filter up, down. 3, 4: sound up, down. 5: pan. 6 to 9: the first four, over two bars. 0: random. |
| Tonality | 1: ignore the master track. 2: follow the root by semitones only. 3, 4, 5: follow it, and add an octave, a fifth or a third. 6, 7: a semitone up, down. 8, 9, 0: snap to the moved scale, its triad, or its root and fifth. |
| Jump | 1: to step 1. 2, 3, 4: to 2/4, 3/4, 4/4 of the pattern. 5, 6: a step forward, back. 7: random. 8: stay. 9: align with the other tracks. 0: gate step. |
| Sparks | 1 to 8: on pass N of every N. 9: random. 0: reset the count. |

For a spark, click the same black key again to change how it counts. The four ways are: pass N of N, pass 1 of N,
all but pass N of N, and all but pass 1 of N.

## Notes

- A **slot** is one turn of a step under the playhead. Pulse and pulse hold give a step more slots, so the pattern
  gets longer. A jump moves the playhead, but does not change the step count.
- The ramps and random walk the key that the Sequencer finds in the synth tracks. Then they follow the master track.
- The tonality component acts on each note as the note starts. Thus it follows a master note that starts at the same
  time.
- A sweep moves the sound that the track plays, not the dials. The dials stay where they are. The master track has
  no sound to sweep.
- The **gate step** jump moves every track whose step length is **Per ADV trigger**, as a trigger at **ADV** does.
  Thus one track can clock other tracks.
