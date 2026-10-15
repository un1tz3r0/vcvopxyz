# To do

## Next

- [ ] **16 patterns per track and pattern chaining** in the Sequencer, read and written in project files: only
      the pattern playing is now. The Sequencer is the master, so these go in it rather than in a module of their
      own.

## Later

- [ ] **More FX,** if the OP-Z has any the six here miss: each is one `Unit` for `FxTrack`.
- [ ] **More OP-Z synth engines,** each a `Voice` and two dials on the shared synth track.
- [ ] **The OP-XY:** its sequencer and engines, once the OP-Z's are complete.
- [ ] **VCV Library:** releases go on GitHub for now, since VCV's guidelines bar copying a product's layout without
      its maker's permission. Next would be asking teenage engineering; a redesign under another name is the last
      resort.

## Loose ends

- [ ] Tune by ear: each engine's voicing, the built-in drum kits and the drums' level against the synths, the sweep
      component's depth (half a dial's travel now) and portamento component's glide times (up to a beat now).
- [ ] **Drum note styles** follow a reading of the guide (retrig rings two sounds or more, mono chokes, gate and
      loop follow the note). Check them against the hardware.
- [ ] **OP-1 kit files:** the slices' pitch and volume encodings are guesses. Check them with a kit whose sounds
      are retuned or turned down on an OP-1 or OP-Z.
- [ ] **Kit editing:** a kit's slice starts and ends, and each slice's tuning and play mode, can't be changed in
      Rack yet, and kits can't be saved back out as OP-1 kit files.
- [ ] Loading a kit from the context menu isn't undoable.
- [ ] A preset keeps a loaded kit's slices but not its sounds, which live in the patch's storage folder.
- [ ] Jack LEDs on unpatched outputs stay dark, since an unpatched output has no channels to meter.
- [ ] The RUN, RESET and CONT inputs aren't tested in a running Rack yet.
- [ ] **OP-Z project files** are untested on a real OP-Z. Check an export loads, and work out the track settings'
      bytes (step count, step length, note length, quantize) and swing, which are guesses.
- [ ] Write the mixer's mutes into exported project files.
- [ ] **Recording dial moves as parameter locks** while recording live, if the OP-Z does: the dials only lock a
      held step now.
- [ ] A Screen shows the dials' own values, not the locks of the step playing.
- [ ] Exported project files hold micro timing from −12 to +11 ticks, as the OP-Z's own do, so notes further off the
      step than that are pulled in.
- [ ] **CI:** `actions/checkout@v4` and `upload-artifact@v4` warn about Node 20, and the Linux build runs in an
      end-of-life `ubuntu:20.04` container, kept for its old glibc.
- [ ] **Digital's dials:** on the hardware the main wave is a sine that wraps around as it's driven, P1 setting how
      many times it wraps and P2 some kind of phase self-modulation (Vic, 2026-10-14). Digital's P1 is the
      modulator's octave and P2 the FM depth now; rework them to match, and tune by ear.
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
- [ ] The FX modules' plug ids aren't written to exported project files, so the OP-Z keeps whatever effects the
      imported file had, or its defaults.
- [ ] **Dead code:** `src/Screen.{hpp,cpp}` and the unused widgets in `src/ui.hpp`, with their SVGs.
