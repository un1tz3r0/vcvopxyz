#include "Kit.hpp"
#include <cstring>

namespace opz {

namespace {

uint64_t bigEndian(const uint8_t* p, int bytes) {
	uint64_t v = 0;
	for (int i = 0; i < bytes; i++)
		v = v << 8 | p[i];
	return v;
}

uint64_t littleEndian(const uint8_t* p, int bytes) {
	uint64_t v = 0;
	for (int i = bytes - 1; i >= 0; i--)
		v = v << 8 | p[i];
	return v;
}

/** A decoded file: its channels mixed to mono, and an OP-1 application chunk's text, if it has one. */
struct Audio {
	std::vector<float> samples;
	float rate = 44100.f;
	std::string op1;
};

/** Integer PCM of any width up to 32 bits, or IEEE floats, interleaved, to mono. */
struct Format {
	int channels = 0, bytes = 0;
	bool little = false, real = false, offsetBinary = false;

	bool valid() const {
		return channels > 0 && (real ? bytes == 4 || bytes == 8 : bytes >= 1 && bytes <= 4);
	}

	float sample(const uint8_t* p) const {
		uint64_t v = little ? littleEndian(p, bytes) : bigEndian(p, bytes);
		if (real) {
			if (bytes == 8) {
				double d;
				std::memcpy(&d, &v, 8);
				return (float) d;
			}
			uint32_t w = (uint32_t) v;
			float f;
			std::memcpy(&f, &w, 4);
			return f;
		}
		if (offsetBinary)
			v ^= 0x80;
		return float(int32_t(uint32_t(v) << (32 - 8 * bytes))) / 2147483648.f;
	}

	void decode(const uint8_t* data, size_t size, size_t frames, std::vector<float>& out) const {
		size_t frameBytes = size_t(bytes) * channels;
		frames = std::min(frames, size / frameBytes);
		out.resize(frames);
		for (size_t i = 0; i < frames; i++) {
			float sum = 0.f;
			for (int c = 0; c < channels; c++)
				sum += sample(data + i * frameBytes + c * bytes);
			out[i] = sum / channels;
		}
	}
};

/** An AIFF's sample rate, an 80-bit extended float. */
double extended(const uint8_t* p) {
	int exponent = int(bigEndian(p, 2) & 0x7fff);
	double v = std::ldexp((double) bigEndian(p + 2, 8), exponent - 16446);
	return p[0] & 0x80 ? -v : v;
}

bool decodeAiff(const std::vector<uint8_t>& f, Audio* a, std::string* error) {
	Format format;
	size_t frames = 0, dataSize = 0;
	const uint8_t* data = nullptr;
	for (size_t at = 12; at + 8 <= f.size();) {
		const uint8_t* c = &f[at];
		size_t size = std::min<size_t>(bigEndian(c + 4, 4), f.size() - at - 8);
		if (!std::memcmp(c, "COMM", 4) && size >= 18) {
			format.channels = (int) bigEndian(c + 8, 2);
			frames = bigEndian(c + 10, 4);
			format.bytes = int(bigEndian(c + 14, 2) + 7) / 8;
			a->rate = (float) extended(c + 16);
			if (size >= 22) {
				std::string type((const char*) c + 26, 4);
				format.little = type == "sowt";
				format.real = type == "fl32" || type == "FL32" || type == "fl64" || type == "FL64";
				if (type != "NONE" && type != "twos" && !format.little && !format.real) {
					*error = "compressed AIFF-C (" + type + ") isn't supported";
					return false;
				}
			}
		}
		else if (!std::memcmp(c, "SSND", 4) && size >= 8) {
			size_t offset = std::min<size_t>(bigEndian(c + 8, 4), size - 8);
			data = c + 16 + offset;
			dataSize = size - 8 - offset;
		}
		else if (!std::memcmp(c, "APPL", 4) && size >= 4 && !std::memcmp(c + 8, "op-1", 4))
			a->op1.assign((const char*) c + 12, size - 4);
		at += 8 + size + (size & 1);
	}
	if (!format.valid() || !data || !(a->rate > 0.f)) {
		*error = "no audio in it that can be read";
		return false;
	}
	format.decode(data, dataSize, frames, a->samples);
	return true;
}

bool decodeWav(const std::vector<uint8_t>& f, Audio* a, std::string* error) {
	Format format;
	format.little = true;
	const uint8_t* data = nullptr;
	size_t dataSize = 0;
	for (size_t at = 12; at + 8 <= f.size();) {
		const uint8_t* c = &f[at];
		size_t size = std::min<size_t>(littleEndian(c + 4, 4), f.size() - at - 8);
		if (!std::memcmp(c, "fmt ", 4) && size >= 16) {
			int tag = (int) littleEndian(c + 8, 2);
			if (tag == 0xfffe && size >= 26)
				tag = (int) littleEndian(c + 32, 2);
			format.channels = (int) littleEndian(c + 10, 2);
			a->rate = (float) littleEndian(c + 12, 4);
			format.bytes = int(littleEndian(c + 22, 2) + 7) / 8;
			format.real = tag == 3;
			format.offsetBinary = format.bytes == 1;
			if (tag != 1 && tag != 3) {
				*error = "compressed WAV isn't supported";
				return false;
			}
		}
		else if (!std::memcmp(c, "data", 4)) {
			data = c + 8;
			dataSize = size;
		}
		at += 8 + size + (size & 1);
	}
	if (!format.valid() || !data || !(a->rate > 0.f)) {
		*error = "no audio in it that can be read";
		return false;
	}
	format.decode(data, dataSize, SIZE_MAX, a->samples);
	return true;
}

bool decode(const std::string& path, Audio* a, std::string* error) {
	std::vector<uint8_t> f = system::readFile(path);
	bool aiff = f.size() >= 12 && !std::memcmp(&f[0], "FORM", 4) && (!std::memcmp(&f[8], "AIFF", 4) || !std::memcmp(&f[8], "AIFC", 4));
	bool wav = f.size() >= 12 && !std::memcmp(&f[0], "RIFF", 4) && !std::memcmp(&f[8], "WAVE", 4);
	if (!aiff && !wav) {
		*error = "not an AIFF or WAV file";
		return false;
	}
	return aiff ? decodeAiff(f, a, error) : decodeWav(f, a, error);
}

/** The slices of an OP-1 drum kit's JSON, or false if it isn't one. */
bool readMarkers(Kit& kit, const std::string& text) {
	size_t from = text.find('{'), to = text.rfind('}');
	if (from == std::string::npos || to == std::string::npos || to < from)
		return false;
	json_error_t e;
	json_t* root = json_loadb(text.data() + from, to - from + 1, 0, &e);
	const char* type = json_string_value(json_object_get(root, "type"));
	bool drum = type && std::string(type) == "drum" && json_is_array(json_object_get(root, "start")) && json_is_array(json_object_get(root, "end"));
	for (int i = 0; drum && i < Kit::SLICES; i++) {
		auto field = [&](const char* key, double otherwise) {
			json_t* v = json_array_get(json_object_get(root, key), i);
			return json_is_number(v) ? json_number_value(v) : otherwise;
		};
		int frames = (int) kit.samples.size();
		int a = clamp((int) std::lround(field("start", 0.0) / 4058.0), 0, frames);
		int b = clamp((int) std::lround(field("end", 0.0) / 4058.0), 0, frames);
		Kit::Slice& s = kit.slices[i];
		s.start = std::min(a, b);
		s.end = std::max(a, b);
		double mode = field("playmode", 8192.0);
		s.mode = mode < 8192.0 ? Kit::GATE : mode > 16384.0 ? Kit::LOOP : Kit::ONESHOT;
		s.reverse = field("reverse", 8192.0) > 12288.0;
		s.speed = (float) std::exp2(field("pitch", 0.0) / 512.0 / 12.0);
		s.gain = (float) std::clamp(field("volume", 8192.0) / 8192.0, 0.0, 4.0);
	}
	json_decref(root);
	return drum;
}

std::string lowercase(std::string s) {
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
	return s;
}

bool isAudio(const std::string& path) {
	std::string ext = lowercase(system::getExtension(path));
	return ext == ".wav" || ext == ".aif" || ext == ".aiff" || ext == ".aifc";
}

} // namespace


void Kit::append(const std::vector<float>& sound, float soundRate, const std::string& sliceName) {
	if (used >= SLICES || sound.empty())
		return;
	Slice& s = slices[used++];
	s = Slice();
	s.name = sliceName;
	s.start = (int) samples.size();
	double step = soundRate / rate;
	for (double t = 0.0; t <= sound.size() - 1; t += step) {
		size_t i = (size_t) t;
		float f = float(t - i), next = i + 1 < sound.size() ? sound[i + 1] : sound[i];
		samples.push_back(sound[i] + (next - sound[i]) * f);
	}
	s.end = (int) samples.size();
}

std::unique_ptr<Kit> Kit::load(const std::string& path, std::string* error) {
	Audio a;
	if (!decode(path, &a, error))
		return nullptr;
	auto kit = std::make_unique<Kit>();
	kit->name = system::getStem(path);
	kit->rate = a.rate;
	kit->samples = std::move(a.samples);
	kit->used = SLICES;
	if (!readMarkers(*kit, a.op1)) {
		int n = (int) kit->samples.size();
		for (int i = 0; i < SLICES; i++) {
			kit->slices[i].start = int(int64_t(n) * i / SLICES);
			kit->slices[i].end = int(int64_t(n) * (i + 1) / SLICES);
		}
	}
	return kit;
}

std::unique_ptr<Kit> Kit::loadFolder(const std::string& dir, std::string* error) {
	std::vector<std::string> files;
	for (const std::string& path : system::getEntries(dir))
		if (system::isFile(path) && isAudio(path))
			files.push_back(path);
	std::sort(files.begin(), files.end(), [](const std::string& a, const std::string& b) {
		return lowercase(system::getFilename(a)) < lowercase(system::getFilename(b));
	});
	auto kit = std::make_unique<Kit>();
	kit->name = system::getFilename(dir);
	for (const std::string& path : files) {
		Audio a;
		std::string skipped;
		if (kit->used < SLICES && decode(path, &a, &skipped))
			kit->append(a.samples, a.rate, system::getStem(path));
	}
	if (!kit->used) {
		*error = "no AIFF or WAV files in it that can be read";
		return nullptr;
	}
	return kit;
}

std::vector<uint8_t> Kit::toWav() const {
	std::vector<uint8_t> f(44 + 4 * samples.size());
	auto put = [&](size_t at, uint32_t v, int bytes) {
		for (int i = 0; i < bytes; i++, v >>= 8)
			f[at + i] = v & 0xff;
	};
	uint32_t size = 4 * (uint32_t) samples.size();
	std::memcpy(&f[0], "RIFF", 4);
	put(4, 36 + size, 4);
	std::memcpy(&f[8], "WAVEfmt ", 8);
	put(16, 16, 4);
	put(20, 3, 2);             // IEEE float
	put(22, 1, 2);
	put(24, (uint32_t) rate, 4);
	put(28, (uint32_t) rate * 4, 4);
	put(32, 4, 2);
	put(34, 32, 2);
	std::memcpy(&f[36], "data", 4);
	put(40, size, 4);
	std::memcpy(&f[44], samples.data(), size);
	return f;
}

json_t* Kit::toJson() const {
	json_t* root = json_object();
	json_object_set_new(root, "name", json_string(name.c_str()));
	json_t* list = json_array();
	for (const Slice& s : slices)
		json_array_append_new(list, json_pack("[iiffibs]", s.start, s.end, s.speed, s.gain, s.mode, s.reverse, s.name.c_str()));
	json_object_set_new(root, "slices", list);
	return root;
}

std::unique_ptr<Kit> Kit::fromSaved(const std::vector<uint8_t>& wav, json_t* root) {
	Audio a;
	std::string error;
	if (wav.size() < 12 || !decodeWav(wav, &a, &error) || !json_is_array(json_object_get(root, "slices")))
		return nullptr;
	auto kit = std::make_unique<Kit>();
	const char* name = json_string_value(json_object_get(root, "name"));
	kit->name = name ? name : "";
	kit->rate = a.rate;
	kit->samples = std::move(a.samples);
	kit->used = SLICES;
	int frames = (int) kit->samples.size();
	for (int i = 0; i < SLICES; i++) {
		Slice& s = kit->slices[i];
		double speed = 1.0, gain = 1.0;
		int reverse = 0;
		const char* sliceName = "";
		json_unpack(json_array_get(json_object_get(root, "slices"), i), "[iiFFibs]", &s.start, &s.end, &speed, &gain, &s.mode, &reverse, &sliceName);
		s.start = clamp(s.start, 0, frames);
		s.end = clamp(s.end, s.start, frames);
		s.speed = (float) speed;
		s.gain = (float) gain;
		s.mode = clamp(s.mode, 0, (int) LOOP);
		s.reverse = reverse;
		s.name = sliceName;
	}
	return kit;
}

} // namespace opz
