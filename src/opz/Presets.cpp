#include "Presets.hpp"
#include <memory>

namespace opz {

namespace {

const char* const NAMES[PRESET_PARAMS] = {"p1", "p2", "filter", "resonance", "attack", "decay", "sustain", "release",
	"lfoAmount", "lfoSpeed", "lfoTarget", "lfoShape", "fx1", "fx2", "pan", "level", "style", "portamento"};

std::atomic<bool> unwritten{false};

std::string fileName(int i) {
	return string::f("%02d.json", i + 1);
}

std::string userDirectory(const Plug* p) {
	return asset::user(system::join("vcvopxyz", "presets", p->slug));
}

/** A preset's file into it, if there is one and it reads. */
bool readFile(Preset& preset, const std::string& path) {
	if (!system::isFile(path))
		return false;
	FILE* f = std::fopen(path.c_str(), "r");
	if (!f)
		return false;
	json_error_t error;
	json_t* root = json_loadf(f, 0, &error);
	std::fclose(f);
	json_t* shares = json_object_get(root, "shares");
	if (!json_is_object(shares)) {
		WARN("Preset %s doesn't read: %s", path.c_str(), root ? "no shares" : error.text);
		json_decref(root);
		return false;
	}
	for (int k = 0; k < PRESET_PARAMS; k++) {
		json_t* v = json_object_get(shares, NAMES[k]);
		float s = json_is_number(v) ? (float) json_number_value(v) : -1.f;
		preset.shares[k] = std::isfinite(s) && s >= 0.f ? std::min(s, 1.f) : -1.f;
	}
	preset.used = true;
	json_decref(root);
	return true;
}

struct Table {
	std::unique_ptr<Preset[]> presets;

	Table() : presets(new Preset[plugCount() * PRESETS]) {
		for (int n = 0; n < plugCount(); n++) {
			const Plug* p = numberedPlug(n);
			for (int i = 0; i < PRESETS; i++) {
				Preset& preset = presets[n * PRESETS + i];
				if (!readFile(preset, system::join(userDirectory(p), fileName(i))))
					readFile(preset, asset::plugin(pluginInstance, system::join("res", "presets", p->slug, fileName(i))));
			}
		}
	}
};

Table& table() {
	static Table t;
	return t;
}

} // namespace


void Preset::store(const float* values) {
	for (int k = 0; k < PRESET_PARAMS; k++)
		shares[k] = values[k];
	used = true;
	stored++;
	unwritten = true;
}

void Preset::restore(const float* values, bool used) {
	store(values);
	this->used = used;
}

void readPresets() {
	table();
}

Preset& preset(const Plug* plug, int i) {
	return table().presets[plugNumber(plug) * PRESETS + clamp(i, 0, PRESETS - 1)];
}

void writePresets() {
	if (!unwritten.exchange(false))
		return;
	Table& t = table();
	for (int n = 0; n < plugCount(); n++) {
		const Plug* p = numberedPlug(n);
		for (int i = 0; i < PRESETS; i++) {
			Preset& preset = t.presets[n * PRESETS + i];
			int stored = preset.stored;
			if (stored == preset.written)
				continue;
			preset.written = stored;
			std::string dir = userDirectory(p), path = system::join(dir, fileName(i));
			if (!preset.used) {
				if (system::isFile(path))
					system::remove(path);
				continue;
			}
			json_t* root = json_object();
			json_object_set_new(root, "plug", json_string(p->slug.c_str()));
			json_t* shares = json_object();
			for (int k = 0; k < PRESET_PARAMS; k++)
				json_object_set_new(shares, NAMES[k], json_real(preset.shares[k]));
			json_object_set_new(root, "shares", shares);
			system::createDirectories(dir);
			if (json_dump_file(root, path.c_str(), JSON_INDENT(2) | JSON_REAL_PRECISION(9)) != 0)
				WARN("Couldn't write the preset %s", path.c_str());
			json_decref(root);
		}
	}
}

} // namespace opz
