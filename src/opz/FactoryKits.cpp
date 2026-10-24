#include "Kit.hpp"
#include <mutex>
#include <tuple>

namespace opz {

namespace {

/** The kits res/kits holds: listed at once, each read when it's first wanted. */
struct Shelf {
	std::string dir = asset::plugin(pluginInstance, "res/kits");
	std::vector<FactoryKit> list;
	std::vector<std::unique_ptr<Kit>> kits;
	std::vector<bool> tried;
	std::mutex mutex;

	Shelf() {
		for (const std::string& path : system::isDirectory(dir) ? system::getEntries(dir) : std::vector<std::string>()) {
			if (system::getExtension(path) != ".json")
				continue;
			json_error_t error;
			json_t* root = json_load_file(path.c_str(), 0, &error);
			const char* name = json_string_value(json_object_get(root, "name"));
			if (name)
				list.push_back({system::getStem(path), name, clamp((int) json_integer_value(json_object_get(root, "track")), 0, 3)});
			json_decref(root);
		}
		// "Kick 2" before "Kick 10".
		std::sort(list.begin(), list.end(), [](const FactoryKit& a, const FactoryKit& b) {
			return std::make_tuple(a.track, a.id.size(), a.id) < std::make_tuple(b.track, b.id.size(), b.id);
		});
		kits.resize(list.size());
		tried.resize(list.size());
	}
};

Shelf& shelf() {
	static Shelf s;
	return s;
}

} // namespace


const std::vector<FactoryKit>& factoryKits() {
	return shelf().list;
}

int factoryKitIndex(const std::string& id) {
	const std::vector<FactoryKit>& list = factoryKits();
	for (size_t i = 0; i < list.size(); i++)
		if (list[i].id == id)
			return (int) i;
	return -1;
}

const Kit* factoryKit(int i) {
	Shelf& s = shelf();
	if (i < 0 || i >= (int) s.list.size())
		return nullptr;
	std::lock_guard<std::mutex> lock(s.mutex);
	if (!s.tried[i]) {
		s.tried[i] = true;
		std::string base = system::join(s.dir, s.list[i].id);
		json_error_t error;
		json_t* root = json_load_file((base + ".json").c_str(), 0, &error);
		try {
			if (root)
				s.kits[i] = Kit::fromSaved(system::readFile(base + ".wav"), root);
		}
		catch (const Exception&) {}
		json_decref(root);
		if (!s.kits[i])
			WARN("Can't read the kit %s in %s", s.list[i].id.c_str(), s.dir.c_str());
	}
	return s.kits[i].get();
}

} // namespace opz
