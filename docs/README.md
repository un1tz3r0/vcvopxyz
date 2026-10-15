# vcvopxyz manual

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="assets/rack-dark.png">
  <img alt="A Sequencer playing four Drums, Digital, Analog, Cluster and Organ, with a Screen beside Digital" src="assets/rack-light.png">
</picture>

## Modules

| Page | What it covers |
| --- | --- |
| [Sequencer](sequencer.md) | The OP-Z's front panel and the master of the modules to its right: tempo, transport, eight tracks of 16 steps, recording, the mixer, 16 patterns with chains, and project slots. |
| [Step components](step-components.md) | The Sequencer's 14 per-step modifiers: pulses, ratchets, ramps, glides, sweeps, jumps and sparks. |
| [Synth tracks](synth-tracks.md) | What every track shares (status screen, pages, LFO, jacks), the synth note styles, then Digital, Analog, Cluster and Organ. |
| [Drum tracks](drum-tracks.md) | Drum, the kick, snare, perc and sample tracks: 24-sound kits, built in or loaded from files, and the drum note styles. |
| [FX tracks](fx-tracks.md) | Delay, Reverb, Rymd, Chorus, Crush and Dist, the send effects on FX tracks 9 and 10. |
| [Breakout and Drum Breakout](breakout.md) | A synth track and a drum track whose sounds are made of other modules. |
| [Screen](screen.md) | The OP-Z's phone app, as an expander. |

## Getting it running

- Installing: [Linux](../INSTALL.linux.md), [macOS](../INSTALL.macos.md) (Intel and Apple silicon),
  [Windows](../INSTALL.windows.md).
- [Development](development.md): building, releasing, the source tree, panels, and adding a module.

## Behind it

- [DESIGN.md](../DESIGN.md): how the modules fit together, and the choices made where the OP-Z guide is silent.
- [TODO.md](../TODO.md): what's next, and the loose ends.

## A first patch

1. Add a **Sequencer**, then a **Digital** directly to its right. No cables: Digital plays track 5 (bass).
2. Patch the Sequencer's **L** and **R** to an audio interface or mixer.
3. Click the bass track's key beside the dials, play the round keys to hear it, and click steps to paste the last
   notes played into them.
4. Click **PLAY**. Turn the Sequencer's dials to shape the sound, and click **SHIFT** for the next page.
5. Add Analog, Cluster or Organ to the right of Digital for tracks 6, 7 and 8, and a Screen beside any engine to see
   its pages.
6. Add up to four **Drum**s anywhere in the row for tracks 1 to 4. Select the kick track and click steps 1, 5, 9
   and 13 for four on the floor.
7. Add a **Delay** anywhere in the row, select the bass track and turn its **FX 1** send up on the red page for
   echoes. Click **TRACK** and step 9 to reach the delay's own dials, and play a key to set its time.
8. Switch **PROJECT** on and long-press a black key to save the project to that slot.
