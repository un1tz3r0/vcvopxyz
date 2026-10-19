#pragma once
#include "../plugin.hpp"
#include <memory>

/*
A drum kit, as the OP-1 and OP-Z keep one: a single buffer of mono samples cut into 24 slices, one per key. A kit
file is an AIFF whose "op-1" application chunk holds JSON with each slice's start and end and how it plays. Its
encodings come from reverse engineering, not from teenage engineering: markers count 4058 to a sample (2^31 over
12 seconds at 44.1 kHz), play modes are 4096 for gate, 8192 for one-shot and 20480 for loop, and reverse is 8192
for forwards. Pitch in 512ths of a semitone and volume with 8192 as unity are guesses.
*/
namespace opz {

struct Kit {
	/** How a slice plays: while its note is held, to its end, or round and round while held. */
	enum Mode { GATE, ONESHOT, LOOP };

	struct Slice {
		int start = 0, end = 0;     // in samples; the end is past the last
		float speed = 1.f;          // its own tuning, as a playback rate
		float gain = 1.f;
		int mode = ONESHOT;
		bool reverse = false;
		std::string name;

		bool empty() const {
			return end - start < 2;
		}
	};

	static constexpr int SLICES = 24;

	std::string name;
	float rate = 44100.f;
	std::vector<float> samples;
	Slice slices[SLICES];
	int used = 0;                   // slices filled by append()

	/** The sound at a fractional position between `start` and `end`, interpolated over four samples (Catmull-Rom)
	that stop at the ends. */
	float at(double position, int start, int end) const {
		int i = (int) std::floor(position);
		float f = float(position - i);
		auto x = [&](int j) { return samples[std::clamp(j, start, end - 1)]; };
		float y0 = x(i - 1), y1 = x(i), y2 = x(i + 1), y3 = x(i + 2);
		float c1 = 0.5f * (y2 - y0), c2 = y0 - 2.5f * y1 + 2.f * y2 - 0.5f * y3, c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
		return ((c3 * f + c2) * f + c1) * f + y1;
	}

	/** Adds a sound as the next slice, resampled to the kit's rate. */
	void append(const std::vector<float>& sound, float soundRate, const std::string& sliceName);

	/** A kit file, or any AIFF or WAV, which without the kit's markers is cut into 24 equal slices. */
	static std::unique_ptr<Kit> load(const std::string& path, std::string* error);
	/** Up to 24 sounds from a folder of AIFF and WAV files, in order of their names, one per key. */
	static std::unique_ptr<Kit> loadFolder(const std::string& dir, std::string* error);

	/** As an OP-1 or OP-Z kit file: a 16-bit AIFF at 44.1 kHz with the slices in its "op-1" chunk. The OP-1 counts
	markers up to 2^31, so only the first 12 seconds fit; `cut` says whether there was more. */
	std::vector<uint8_t> toOp1(bool* cut = nullptr) const;

	/** As a patch keeps it: the samples as a float WAV, and everything else as JSON. */
	std::vector<uint8_t> toWav() const;
	json_t* toJson() const;
	static std::unique_ptr<Kit> fromSaved(const std::vector<uint8_t>& wav, json_t* root);
};

/** The kit a drum track plays until another is loaded, made for its track: kicks on 1, snares and claps on 2,
hats, toms and percussion on 3, and effects and tones on 4. Synthesized the first time it's asked for. */
const Kit& builtinKit(int track);

} // namespace opz
