# Screen

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="assets/digital-screen-dark.png">
  <img alt="A Screen beside Digital" src="assets/digital-screen-light.png" width="270">
</picture>

The OP-Z's phone app, as an expander: the OP-Z famously has no screen, since you already carry one. Place it
directly to the right of an OP-Z engine or [FX module](fx-tracks.md). Its top line names the engine, the page the dials are on and the tempo, and
under it is the dial last turned, in its colour, with its value.

The middle shows a picture of the page:

| Page | Picture |
| --- | --- |
| Sound | A scope of the output, with the filter pair's response over it, and an FX module's time |
| Envelope | The envelope, a stage per dial colour, with a dot riding it for each sounding voice |
| LFO | One cycle of the shape, as deep as the amount, with the speed, the target and a dot at the current phase |
| Arp (the arp track's LFO page) | Sixteen beats of the arp over a triad: each note as high as the pattern takes it, as big as its accent |
| Mix | The two sends, the pan, the output meters and the level |
| Track | The note style, and the glide as a slide between two notes |

Below that, every dial of every page is an arc in its colour, with the page the dials are on lit up. Click one to
select it and jump to its page (the track row switches track mode on), then drag up or down anywhere on the screen to
turn it, finely with Ctrl, like the app's touch pad. Each drag is one undo step.

## Kit editor

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="assets/kit-screen-dark.png">
  <img alt="A Screen editing a Drum's kit" src="assets/kit-screen-light.png" width="270">
</picture>

Beside a [Drum](drum-tracks.md), a **KIT** button sits by the dial's value. Click it to swap the page's picture for
the kit, as teenage engineering's drum utility shows one, and again to go back.

- The top strip is the whole kit, with a tick where each slice starts and the slice shown shaded. Click a slice there
  to show it. Playing a key shows its slice too.
- Below it is the slice up close. Drag near its green start or blue end to move it, finely with Ctrl.
- **TUNE** and **LEVEL** drag up and down (four octaves of tuning, up to 400% level). Click **PLAY** to step through
  one shot, gate and loop, and **REVERSE** to flip it.

The slice's name and length replace the dial's value above. Each drag or click is one undo step, and the dials below
work as ever.

## The bus

A Screen passes the bus on, so engines to its right still get their tracks from the Sequencer. The engines have no
dials of their own, so a Screen is the way to edit one that isn't on a Sequencer's bus.
