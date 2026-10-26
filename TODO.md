# To do

## Next

The engine move, planned in Vic's "Plan: engines into the Sequencer" doc (2026-10-24), is done: OP-Z kits (2.0.25),
the engine core split (2.0.26), the row record's dials (2.0.27), engines in the Sequencer (2.1.0), presets (2.1.1),
the display (2.1.2) and the manual rewritten for it (2.1.3). Next:

- [ ] **Re-sample the flagged pads** with track 4's kits: pad 16 of every snare and hat kit (the bounces stop 8 ms
      into it), Hat 5's pads 4, 6, 8, 10 and 12 (nothing played), and the sounds still ringing when the next one
      started (Hat 2 pads 10 to 13, Hat 4 pads 9 and 10, Hat 5 pads 1, 2, 13 and 15, Kick 1 pad 14, Kick 2 pad 4,
      Kick 4 pads 3 and 13). `split.py` prints the list.
- [ ] **Voice presets 2 to 4** of each synth engine by ear (Vic), store them, and copy the files from Rack's user
      folder (`vcvopxyz/presets/<slug>/`) into `res/presets/` as the factory ones.
- [ ] **Sample,** the OP-Z's last synth engine: a PCM sample player with a crush dial, which could load its sound
      through `Kit`.

## Later

- [ ] **More FX,** if the OP-Z has any the six here miss: each is one `Unit` for `FxTrack`.
- [ ] **The OP-XY:** its sequencer and engines, in a fresh start once the OP-Z's are complete.
- [ ] **VCV Library:** releases go on GitHub for now, since VCV's guidelines bar copying a product's layout without
      its maker's permission. Next would be asking teenage engineering; a redesign under another name is the last
      resort.

## Found during the engine move

Whatever turns up off the plan's path, each for a PR of its own afterwards.

- [ ] Every track now runs in the Sequencer's process, on one engine thread, where the engine modules could spread
      across Rack's threads.

## Loose ends

- [ ] Tune by ear: each engine's voicing (Digital's wrap range and feedback ceiling too), the built-in drum kits and the drums' level against the synths, the sweep
      component's depth (half a dial's travel now) and portamento component's glide times (up to a beat now).
- [ ] **Drum note styles** follow a reading of the guide (retrig rings two sounds or more, mono chokes, gate and
      loop follow the note). Check them against the hardware.
- [ ] **OP-1 kit files:** the slices' pitch and volume encodings are guesses. Check them with a kit whose sounds
      are retuned or turned down on an OP-1 or OP-Z.
- [ ] A preset keeps a loaded kit's slices but not its sounds, which live in the patch's storage folder.
- [ ] Jack LEDs on unpatched outputs stay dark, since an unpatched output has no channels to meter.
- [ ] On the light panel, a white light on a black key doesn't show: track mode's plug slots and tempo mode's keys
  light only on the dark panel (seen in the guide's `slots` renders).
- [ ] The RUN, RESET and CONT inputs aren't tested in a running Rack yet.
- [ ] **OP-Z project files** are untested on a real OP-Z. Check an export loads, and work out the track settings'
      bytes (step count, step length, note length, quantize) and swing, which are guesses.
- [ ] Write the mixer's mutes into exported project files.
- [ ] A Screen shows the dials' own values, not the locks of the step playing.
- [ ] Exported project files hold micro timing from −12 to +11 ticks, as the OP-Z's own do, so notes further off the
      step than that are pulled in.
- [ ] **CI:** `actions/checkout@v4` and `upload-artifact@v4` warn about Node 20, and the Linux build runs in an
      end-of-life `ubuntu:20.04` container, kept for its old glibc.
- [ ] **Key detection and the master track** follow a reading of the guide. Check against the hardware: which keys
      it finds for ambiguous patterns, where home is, whether latch lets go at the pattern's start or the bar's, and
      what the tonality component's quantize values snap to.
- [ ] The master track's own sound dials (the OP-Z's master filter and the like) aren't there: outside track mode its
      dials do nothing.
- [ ] **Arp:** check against the hardware its eight speeds, the styles' accents (and style 3, which the only source
      lists the same as style 2), what range does between whole octaves, and how long its notes are.
- [ ] A held step plays its arp locks only while playing: stopped, the arp plays with the dials as they are.
- [ ] **FX:** check the six effects against the hardware: Delay's key times and feedback ceiling, the reverbs'
      sizes and predelay, Chorus-80's depth and rate range, Crush's rate and bit range, Dist's drive and tone. Which
      LFO targets the FX tracks really have is a guess (time and send stand in for attack and pitch).
- [ ] Analog, Organ, EP and Rymd have no OP-Z plug id, so an exported project file keeps whatever the imported
      one had on their tracks, or the OP-Z's defaults.
- [ ] **Patterns and chains:** check against the hardware where a chain's pattern ends (the longest track here),
      whether a pattern never played keeps the sound before it, and what PROJECT with the white keys does beyond
      the saved chains.
- [ ] A project switch's undo step holds the project either side, about 800 KB now that a slot has 16 patterns;
      a long session of switches adds up.
- [ ] In the light theme a white LED barely shows against the light keys, so TRACK, SHIFT, a selected track and
      the playhead are hard to see lit.
- [ ] The manual's pictures come from `tools/docs-images` in a headless Rack; rerun them when a panel changes.
- [ ] **Dead code:** `src/Screen.{hpp,cpp}` and the unused widgets in `src/ui.hpp`, with their SVGs.
