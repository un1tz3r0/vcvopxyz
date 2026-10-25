# Rack in two minutes

This page is for readers who have never used VCV Rack. It covers only what the [guide](guide.md) needs. VCV's own
[manual](https://vcvrack.com/manual/) has the rest.

VCV Rack is a modular synthesizer on your computer. A patch is a set of modules, joined by cables, in rows on a rack.
Rack saves a patch as a `.vcv` file.

## Modules

- To add a module, right-click an empty part of the rack. The module browser opens. Type a name, such as
  "Sequencer", and click the module. You can also press **Enter** to open the browser.
- To move a module, drag its panel. Modules in a row push each other along, and they snap together.
- To remove a module, click its panel and press **Delete** or **Backspace**.
- Right-click a module's panel for its **context menu**. The menu has the module's settings.

The modules of this plugin are in the browser under the brand "un1tz3r0". The [install pages](../INSTALL.linux.md)
show how to add the plugin.

## Cables

- An output jack sends a signal, and an input jack receives one. Most panels show outputs on a dark background.
- To patch a cable, drag from an output jack and release on an input jack.
- To remove a cable, drag its plug away from the jack and release it on an empty part of the rack.
- One output can feed many inputs. Drag from the same output again for each cable.

Audio is a voltage of about ±5 V. A pitch CV uses 1 V for each octave (V/OCT). A gate is 10 V while a note plays.

## Sound

Rack makes no sound until a patch has an audio module. The **Audio-2** module, in the browser under "VCV", sends its
two inputs to your sound card.

1. Add an **Audio-2** module.
2. Click its display, and choose a driver and then a device.
3. Patch your sound to its inputs: **L/MON** for the left speaker and **RIGHT** for the right.

## Dials and keys

- Drag a dial up or down to turn it. Hold **Ctrl** (**⌘** on a Mac) while you drag, for fine steps.
- Hover the mouse over a dial, key or jack to see its tooltip. The tooltip gives its name and value.
- Right-click a dial to type a value.

## Undo and save

- **Ctrl+Z** undoes a change, and **Ctrl+Shift+Z** redoes it. On a Mac, use **⌘** for **Ctrl**.
- **Ctrl+S** saves the patch. **File > Save as** saves it under a new name.
- **Ctrl** and the scroll wheel zoom in and out. The scroll wheel alone moves the view.

Rack keeps some files outside your patches, such as this plugin's [presets](how-it-works.md#what-saves-where).
**Help > Open user folder** shows where they are.

Next: [How it works](how-it-works.md), then [Your first song](guide.md).
