# Project

<picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/face-project-dark.png"><img alt="The face in project mode: the pattern playing white, the project slot lit on the black keys" src="assets/keys/face-project-light.png"></picture>

A **project** is one song. It has 16 patterns, its chains, its tempo and its mixer. The Sequencer keeps ten projects
in its **project slots**. This page tells how to play patterns, chain them, and save and load projects.

Click **PROJECT** to latch project mode. Click it again to release it. In project mode:

- The steps are the project's 16 patterns.
- The black keys are the ten project slots.
- The white keys are 14 saved chains.
- The display shows the project's number.

## Patterns

A **pattern** holds every track. For each track, it holds:

- the steps, with their step components and parameter locks,
- the step count, step length, note length, quantize and polyphony,
- the plug slot that the track plays, and its dials.

A pattern also holds the mixer's mute group and its [tempo](song-editor.md#each-patterns-tempo).

In project mode, the steps show the patterns. The pattern that plays lights white, and patterns with notes light
dimly.

- **Play a pattern:** click its step. While the Sequencer plays, the new pattern starts at once, and each track
  continues from the step that it is on.
- **A new pattern** keeps the plugs and dials as they are, so it starts from the sound of the last pattern.

Without a chain, each track loops at its own length. Tracks of different lengths drift against each other.

## Chains

A **chain** plays patterns in turn. At the end of each pattern, every track starts the next pattern from step 1
together.

1. Latch **PROJECT**, then click **PLAY**. PLAY blinks.
2. Click the steps of the patterns, in the order to play them. The first pattern starts at once. A chain holds up to
   32 patterns, and a pattern can be in it more than once. The display counts them.
3. Click **PLAY** again to finish the chain.

The rest of the chain lights green on the steps. A pattern's end is the end of its longest track: its step count
times its step length. Tracks that wait for **ADV** triggers do not count. Outside project mode, **PLAY** starts the
chain from its first pattern. To return to one pattern, click its step.

To play patterns and chains from other modules, use [Remotes](remotes.md). To give each row of a saved chain a
length, a mute group, and the song fades and tempo, use the [Song Editor](song-editor.md).

### Saved chains

The 14 white keys keep a chain each. A white key lights dim when it holds a chain, and bright for the chain that
plays.

- To save the chain that plays, long-press a white key, or Ctrl+click it.
- To play a saved chain, click its white key.

With a [Song Editor](song-editor.md), each saved chain is a song, with up to 99 rows.

## Copy a pattern

1. Latch **PROJECT**, then click **SHIFT**. The Sequencer copies the pattern that plays. SHIFT and the steps blink.
2. Click each step that gets a copy.
3. Click **SHIFT** again to stop.

A paste over the pattern that plays is one step of Rack's undo for each track that it changes.

## Project slots

A project slot holds everything that makes a song:

- the 16 patterns, the chain that plays and the saved chains,
- the tempo, the swing and the metronome,
- the mixer's levels and its ten mute groups,
- the master track and the tracks that it moves.

The black keys light dim for the project slots in use, and bright for the one that plays.

- **Load:** click a black key. The Sequencer first saves the current project to its own slot, as the OP-Z does.
  Then it loads the project, with each track's plug and dials. An empty slot loads as a new, blank project.
- **Save:** long-press a black key, or Ctrl+click it. The Sequencer saves the current project to that slot.

To stop the save before a load, right-click the Sequencer, open **Projects**, and clear **Save the project on
switching**.

The project slots save with the patch. A switch of project is one step of Rack's undo. A switch of pattern is not,
but each edit in a pattern is. If you undo an edit later, it changes the pattern that you edited, whichever plays.

## Project files

The Sequencer's context menu, **Projects**, moves the project slots in and out of files.

| Menu item | Does |
| --- | --- |
| **Save the slots to a file…** | Writes all ten project slots to one JSON file. |
| **Load the slots from a file…** | Reads them back, for example into another patch. |
| **Export the slots to a projects folder…** | Writes each project slot in use as `project01.opz` to `project10.opz`. These are the files in the OP-Z's disk mode `projects` folder. |
| **Import a projects folder…** | Reads those files into the project slots of the same numbers. |
| **Import a project into slot N…** | Reads one file into the current project slot. |

The export writes all 16 patterns, the chain that plays and the 14 saved chains. The master track goes to the file's
track 12, and the FX tracks to tracks 9 and 10. Micro timing goes from −12 to +11 ticks, as far as the OP-Z's files
go.

An OP-Z project file holds more than a project slot, for example the drum samples and the module tracks. The patch
keeps each imported file. An export writes the project slot over that file, so the parts that the Sequencer does not
use return unchanged.

> **Caution:** the file format comes from the OP-Z community, not from teenage engineering. Nobody has tried the
> exported files on an OP-Z yet. Keep a copy of your OP-Z's projects before you put exported files on it.
