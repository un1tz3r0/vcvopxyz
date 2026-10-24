#pragma once
#include "Plugs.hpp"
#include <atomic>

/*
Presets: 14 for every plug, shared by every Sequencer, loaded and stored with TRACK and a white key. A preset holds a
track's four pages of dials, its note style and its portamento, each as a share of its travel; the arp's dials belong
to the sequence and stay out. Factory presets come from res/presets/<slug>/NN.json in the plugin, the user's from
vcvopxyz/presets/<slug>/NN.json in Rack's user folder, the user's in place of the factory one with the same number.
A plug's first preset, while no file holds it, is its dials as they start.

All are read at once, on the UI thread, before a Sequencer runs. Storing one only changes memory, on the audio
thread, and the UI thread writes the files after (writePresets). A preset loaded on one Sequencer while another
stores it may mix the two, which only costs a load again.
*/
namespace opz {

constexpr int PRESETS = 14;
/** The track params a preset holds: P1_PARAM to LEVEL_PARAM, STYLE_PARAM and GLIDE_PARAM, which run in order. */
constexpr int PRESET_PARAMS = 18;

struct Preset {
	std::atomic<bool> used{false};
	std::atomic<float> shares[PRESET_PARAMS]; // each param's place in its travel, or below 0 where a file left it out
	std::atomic<int> stored{0};               // the stores so far, against the ones written
	int written = 0;

	/** For the audio thread. */
	void store(const float* values);
	/** Puts it back as it was before a store, for undo, on the UI thread: `values`, or unused, which removes its
	file. */
	void restore(const float* values, bool used);
};

/** Reads every preset, if nothing has yet; for the UI thread. */
void readPresets();
/** A plug's preset `i`. */
Preset& preset(const Plug* plug, int i);
/** Writes the user's files of the presets stored since it last did; for the UI thread. */
void writePresets();

} // namespace opz
