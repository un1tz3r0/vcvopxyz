# Troubleshooting

Find the problem in the headings. Each one lists the causes, the most frequent first, and what to do. If the problem
is not here, [Controls](controls.md) tells what each key does now.

## No sound at all

- **No audio module.** Rack makes no sound until a patch has one. Add an **Audio-2**, choose your sound card on its
  display, and patch the Sequencer's **L** and **R** to it. See [Rack in two minutes](rack-basics.md).
- **The Sequencer does not play.** Click **PLAY**. It lights green. If it blinks, see
  [PLAY blinks, and nothing plays](#play-blinks-and-nothing-plays).
- **The mixer levels are low.** Latch **MIXER**, and raise the dials: drum level, synth level and master level. See
  [Mixer](mixer.md#levels).

## One track makes no sound

- **The track is muted.** Latch **MIXER**. A muted track's step is dark. Click the step. A sound mute's step is
  red. Latch **SHIFT**, and click the step. See [Mutes](mixer.md#mutes).
- **The track's level is low.** Select the track, click its mix page, and raise its level.
- **A breakout has the track.** The track's key on the display shows a small jack. The breakout's patched jacks decide
  where the sound goes:
  - **L** or **R** patched: the track plays at the breakout's jacks, not in the mix.
  - **RET L** or **RET R** patched, but your modules make no sound: the return replaces the track's plug, so the
    track is silent. Check the modules, or remove the return's cables.
  See [Two ways out](breakout.md#two-ways-out).
- **The drum track's kit is gone.** See [A copied Sequencer has a silent drum track](#a-copied-sequencer-has-a-silent-drum-track).
- **The track has no notes.** Steps with notes light orange. A dim orange step is past the step count, so the track
  does not play it. See [Step count and step length](track.md#step-count-and-step-length).

## PLAY blinks, and nothing plays

A cable is in the **CLOCK** input. The Sequencer waits for the next pulse of that clock before it plays.

- Start the clock, for example the DAW that sends it.
- To use the Sequencer's own tempo, remove the cable from **CLOCK**.

## The tempo readout shows dashes

**EXT** is lit, so a cable is in the **CLOCK** input. The readout shows dashes until it has timed two pulses. It shows
them again after a long pause in the pulses. Start the clock, or remove the cable.

## The tempo is wrong with an outside clock

**Clock pulses per beat** does not match the clock. Right-click the Sequencer, and set it to the clock's rate. A MIDI
clock from **MIDI to CV** gives 24 pulses for each beat. See [Follow another clock](recipes.md#follow-another-clock).

## A breakout shows a dash

The breakout has no track.

- **It is not in the row.** It must touch the Sequencer, or touch a module of this plugin that is in the row. A module
  from another plugin between them breaks the row. See [The row](how-it-works.md#the-row).
- **Its track is taken.** A breakout to its left has the track that it chose. Right-click it, and choose another
  track in **Track**, or **Auto**.
- **Every track of its kind is taken.** For example, a fifth Synth Breakout has no free synth track.

## A breakout's ENV, LFO or CV outputs stay at 0 V

A breakout gives these outputs only while it has the track's notes. A breakout's note outputs are:

- **V/OCT** and **GATE** on a Synth Breakout.
- **TRIG** and **GATE** on a Drum Breakout.
- **SEND L** and **SEND R** on a Send FX Breakout.

To get the outputs:

1. Patch a note output to an input.
2. Remove the cables from **L** and **R**. While they are patched, the track's plug plays there.

See [Move other modules from a track](recipes.md#move-other-modules-from-a-track).

## A long press or a Ctrl+click does not latch

- **It is the wrong key.** Only the steps and **SHIFT** latch on a long press or a Ctrl+click. **TRACK**, **REC**,
  **PROJECT**, **MIXER** and **TEMPO** latch on a plain click.
- **The press is too short.** Hold the mouse button for 0.4 s.
- **The gesture is not active.** Right-click the Sequencer, and make sure that **Long-press toggles hold** and
  **Ctrl-click toggles hold** have a check mark.
- **On a Mac,** Ctrl+click is a right-click. Use ⌘+click.

See [Clicks, latches and holds](controls.md#clicks-latches-and-holds).

## The keys do something unexpected

A key is latched, and it changes what the other keys do. Look for:

- **TRACK**, **PROJECT**, **MIXER** or **TEMPO** lit. Click it to release the mode.
- **SHIFT** lit. Click it to release it.
- A blinking step. Click it to release it.

See [Modes](controls.md#modes).

## A MIDI keyboard plays the wrong track

The Sequencer's **V/OCT**, **GATE** and **VEL** inputs play the selected track. Click a track key to select another
track. To play one track always, patch your keyboard to the **V/OCT** and **GATE** inputs of a breakout that has that
track. See [Breakouts](breakout.md#inputs).

## A note does not stop

Click **STOP** while the Sequencer is stopped. This releases every note, also the notes from the inputs.

## Rack says that modules are missing

- **A patch from before version 2.1.0.** It had a module for each track's engine, and those modules are gone. The
  Sequencer gives each track its plug, dials and kit file from them. Save the patch again, and the message stops.
  See [Plugs](track.md#plugs).
- **A module from another plugin.** Install that plugin from the VCV Library.

## A copied Sequencer has a silent drum track

A kit that you loaded from a file keeps its sounds in the patch's own folder. A copied or duplicated Sequencer, or a
Sequencer preset, does not get them. The drum track's plug slot is then empty, and the track is silent. The OP-Z kits
and the built-in kits stay.

1. Right-click the Sequencer, and open **Plug slots**.
2. Open the drum track, then the empty plug slot.
3. Click **Load a kit file…**, and choose the file again.

To keep a kit for later, click **Save as a kit file…** in the same menu. See [Kits](kits.md).

## A lit black key is hard to see on the light panel

On the light panel, a white light on a black key does not show well. This is a known problem. Track mode's plug slots
and tempo mode's keys use these lights. In Rack's **View** menu, click **Use dark panels if available**.

## Next

- [Controls](controls.md): every key, mode and colour.
- [Patch recipes](recipes.md): short ways to reach a goal.
