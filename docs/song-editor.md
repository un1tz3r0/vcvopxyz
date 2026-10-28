# Song Editor

The Song Editor shows the [Sequencer](sequencer.md)'s songs on a large screen, and edits them. Put it to the
Sequencer's left. It can also be to the left of a [Remotes](remotes.md) that touches the Sequencer. It makes no sound,
and it is 24 HP wide.

## Songs

A **song** is a [saved chain](project.md#saved-chains) with more data. Each of the 14 white keys of project mode keeps
one song. A song has up to 99 rows. Each row has three columns:

| Column | Values | What it does |
| --- | --- | --- |
| **PAT** | P01 to P16 | The [pattern](project.md#patterns) that the row plays. |
| **LEN** | -, x2 to x99, 1/16 to 999/16 | **-** plays the pattern one time, to the end of its longest track. **x2** plays it two times. **5/16** plays five sixteenths, then the next row starts from step 1. |
| **MUTE** | -, G1 to G10 | **-** uses the pattern's own [mute group](mixer.md). **G3** uses mute group 3 while the row plays. |

The song also has three settings:

| Setting | Values | What it does |
| --- | --- | --- |
| **IN** | 0 to 64 bars | The fade in at the start of the song. |
| **OUT** | 0 to 64 bars | The fade out at the end of the song. |
| **Tempo** | PAT, 40 to 200 BPM | **PAT** plays each pattern at its own tempo. A number plays every row at that tempo. |

A cable in the Sequencer's **CLOCK** sets the tempo over the two. The screen then shows **EXT**.

## Song mode and pattern mode

Click **MODE** to change between the two modes. **MODE** lights in song mode.

- **Song mode:** the Sequencer plays the song from its first row. At the end of the last row, the song stops. To play
  it again and again, set **Loop songs** in the Sequencer's context menu. Fades do not apply when the song loops.
- **Pattern mode:** the Sequencer works as without the Song Editor. A chain plays its patterns in turn. The
  **LEN** and **MUTE** columns do not apply.

In song mode, a click on a white key of project mode plays that song. A click on a step returns to pattern mode.

## The screen

From the top, the screen shows:

- the mode, and **PLAYING** or **STOPPED**,
- the length of the song, in bars:beats:sixteenths and in hours:minutes:seconds,
- the song's number, and the tempo with its source (**PAT**, **SONG** or **EXT**),
- the time of the playhead, in the same two formats,
- the song's settings,
- the rows, with the length of each row in bars.

The row that plays has a green background and a bar that shows its progress. A frame shows the row that you edit.

In pattern mode, the song's number shows **-P**, and the song's data shows **-**.

## Controls

The lines on the panel connect each control to the item that it changes.

| Control | What it does |
| --- | --- |
| **MODE** | Changes between song mode and pattern mode. |
| **SONG** wheel | Chooses the song, in song mode. The new song starts from its first row. |
| **ROW** wheel | Chooses the row to edit. Above the first row is the settings row. |
| **PAT** wheel | Changes the row's pattern, or on the settings row, the fade in. |
| **LEN** wheel | Changes the row's length, or on the settings row, the fade out. |
| **MUTE** wheel | Changes the row's mute group, or on the settings row, the tempo. |
| **INSERT** | Adds a copy of the row below it. |
| **DELETE** | Removes the row. A song keeps at least one row. |

To turn a wheel, drag it up or down, or scroll on it. Hold Ctrl to move four values for each step.

On the screen, click a row to edit it. Double-click a row to play it. In pattern mode, only the **PAT** column changes.
A change to the row that plays changes the pattern at once.

The Sequencer saves each change to the song's white key at once.

## Each pattern's tempo

Each pattern keeps its own tempo. When a pattern starts, the Sequencer sets the tempo to the pattern's tempo. To set
the tempo of a pattern, change the tempo while it plays. Patterns from before version 2.1.8 have no tempo of their
own, and use the tempo that plays.

## With Remotes

With a Song Editor, a [Remotes](remotes.md) also controls the songs:

- **MODE** high is song mode, low is pattern mode.
- In song mode, **SELECT** channel 1 to 16 plays row 1 to 16, and **NEXT** and **PREV** move one row.
- In song mode, **SONG** and **LOAD** choose the song to play.
