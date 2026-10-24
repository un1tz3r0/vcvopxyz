# Screen

<picture><source media="(prefers-color-scheme: dark)" srcset="assets/guide/screen-dark.png"><img alt="A Screen showing the bass track's sound page" src="assets/guide/screen-light.png"></picture>

The OP-Z's phone app, as a module: the OP-Z famously has no screen, since you already carry one. Place it anywhere to
the right of a [Sequencer](sequencer.md), on its row, and it shows the track selected there. Its top line names the
track's plug, the page the dials are on and the tempo, and under it is the dial last turned, in its colour, with its
value. Off a Sequencer's row, or with the master track selected, it says where it goes.

The middle shows a picture of the page:

| Page | Picture |
| --- | --- |
| Sound | A scope of the track's sound, with the filter's response over it, and an FX track's time |
| Envelope | The envelope, a stage per dial colour, with a dot riding it for each sounding voice |
| LFO | One cycle of the shape, as deep as the amount, with the speed, the target and a dot at the current phase |
| Arp (the arp track's LFO page) | Sixteen beats of the arp over a triad: each note as high as the pattern takes it, as big as its accent |
| Mix | The two sends, the pan, the output meters and the level |
| Track | The note style, and the glide as a slide between two notes |

Below that, every dial of every page is an arc in its colour, with the page the dials are on lit up. Click one to
select it and jump to its page (the track row switches the Sequencer's track mode on), then drag up or down anywhere
on the screen to turn it, finely with Ctrl, like the app's touch pad. Each drag is one undo step.

<picture><source media="(prefers-color-scheme: dark)" srcset="assets/guide/arp-screen-dark.png"><img alt="A Screen showing the arp track's arp page" src="assets/guide/arp-screen-light.png"></picture>

## Kit editor

<picture><source media="(prefers-color-scheme: dark)" srcset="assets/guide/kit-screen-dark.png"><img alt="A Screen editing the kick track's kit" src="assets/guide/kit-screen-light.png"></picture>

With a drum track selected, a **KIT** button sits by the dial's value. Click it to swap the page's picture for the
kit playing, as teenage engineering's drum utility shows one, and again to go back.

- The top strip is the whole kit, with a tick where each slice starts and the slice shown shaded. Click a slice there
  to show it. Playing a key shows its slice too.
- Below it is the slice up close. Drag near its green start or blue end to move it, finely with Ctrl.
- **TUNE** and **LEVEL** drag up and down (four octaves of tuning, up to 400% level). Click **PLAY** to step through
  one shot, gate and loop, and **REVERSE** to flip it.

The slice's name and length replace the dial's value above. Each drag or click is one undo step, and the dials below
work as ever. Editing an OP-Z or built-in kit makes a copy of it in its slot that saves with the patch, as a kit
loaded from a file does; see [kits](kits.md).

## The bus

A Screen passes the bus on, so breakouts to its right still reach their tracks. Two Screens show the same track.
