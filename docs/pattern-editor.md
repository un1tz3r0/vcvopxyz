# Pattern Editor

The Pattern Editor shows a [pattern](project.md#patterns) of the [Sequencer](sequencer.md) on a large screen, and
edits it with the mouse and the computer keyboard. Put it to the Sequencer's left. It can also be to the left of a
[Remotes](remotes.md) or a [Song Editor](song-editor.md) that touches the Sequencer. It makes no sound.

To change its width, drag its left edge. Its right edge stays against the Sequencer. It is 24 to 64 HP wide, and
32 HP when you add it.

## The two views

Click **VIEW** to change between the views. **VIEW** lights in the overview.

- **Track view** shows the notes of one track on a roll. The steps go from left to right, and the pitches go from
  low to high. A drum track's rows are the 24 sounds of its kit. Below the roll are the lane and the components.
- **The overview** shows the 16 steps of each of the 11 tracks. Each dot is a note. A brighter dot is a note with a
  higher velocity. A white corner shows step components. A blue dot shows parameter locks.

In both views, steps past the track's step count are grey and have lines across them. You cannot edit them. The
step that plays has a green background.

The boxes at the top show the pattern, the track, its plug, and the track's step count, step length and note length.
**CURSOR** shows the step and the note at the cursor.

## Edit with the mouse

In track view:

| Action | Result |
| --- | --- |
| Click an empty cell | Adds a note there. Drag before you release to move it. |
| Click a note | Removes it. |
| Drag a note | Moves it one step or one semitone at a time. Hold Ctrl to move it one tick at a time. |
| Drag the end of a note | Changes its length, one step at a time. Hold Ctrl for one tick at a time. Drag past the last step for a drone. |
| Drag across the lane | Sets the velocity, or the lock, of each step that you cross. |
| Ctrl+click a lock in the lane | Removes the lock. |

A step has at most four notes. A step that is full flashes red when you add a note to it. A note can start up to 23
ticks before its step and 24 ticks after it. A tick is 1/24 of a step.

In the overview, click a step to go to that track and step in track view. Click a track's name to choose the track.

Each drag or click is one step of the undo history. Use Rack's **Undo** (Ctrl+Z) to undo it.

## Edit with the keyboard

Click the screen to send the computer keyboard to it. A green line around the screen shows that it has the keyboard.
Press Escape, or click outside the screen, to return the keyboard to Rack. Rack's Ctrl+Z, Ctrl+Shift+Z and Ctrl+S
work while the screen has the keyboard.

| Key | Result |
| --- | --- |
| Arrow keys | Move the cursor one step or one row. In the overview, up and down choose the track. |
| Shift+Left, Shift+Right | Move the note at the cursor one step. With Ctrl, one tick. |
| Shift+Up, Shift+Down | Move the note at the cursor one semitone. With Ctrl, one octave. |
| Enter or Space | Adds or removes the note at the cursor. In the overview, Enter opens track view. |
| Alt+Left, Alt+Right | Make the note at the cursor one step shorter or longer. With Ctrl, one tick. |
| Alt+D | Makes the note at the cursor a drone, or makes a drone a usual note again. |
| Two digits, 00 to 99 | Set the lane's value at the cursor's step. |
| Tab, Shift+Tab | Choose the next or the previous track. |
| V | Changes the view. |
| L, Shift+L | Show the next or the previous lane. |
| Page Up, Page Down | Move the roll one octave. |
| Delete or Backspace | Clears the step at the cursor: its notes, components and locks. |
| Ctrl+C, Ctrl+V | Copy the step at the cursor, and paste it at the cursor. |

When the screen has the keyboard, the mouse wheel moves the roll one semitone. Hold Shift to move it one octave.

## The lane

The lane below the roll shows the velocity of each note. Click **LANE**, or press L, to show the
[parameter locks](steps.md#parameter-locks) of one dial in its place. Each click shows the next dial of the 16 on the
four pages. The name of the dial shows at the left of the lane. **LANE** lights in the color of that dial.
A step with no lock on that dial shows a short line.

## Controls

| Control | What it does |
| --- | --- |
| **VIEW** | Changes between track view and the overview. |
| **FOLLOW** | When it is lit, the screen shows the track that the Sequencer selects. A track that you choose on the screen is then also selected on the Sequencer. |
| **LANE** | Shows velocity, or the locks of the next dial. |
| **OCTAVE -** and **+** | Move the roll one octave down or up. A drum track's roll does not move. |
| **PATTERN** wheel | Shows a different pattern of the 16. Drag it up or down, or scroll on it. |

The screen shows the pattern that plays, and follows a change of pattern. After you turn **PATTERN** to a different
pattern, the screen stays on it. **PAT** at the top is green when the screen shows the pattern that plays. Turn
**PATTERN** back to that pattern to follow it again.

You can edit a pattern that does not play. It plays your changes when the Sequencer starts it.

## Not yet

The component row below the lane shows each step's components, but you cannot edit them here yet. Use the
Sequencer's [step components](step-components.md) for that.
