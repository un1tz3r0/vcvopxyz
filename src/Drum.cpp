/*
Drum: the drum tracks' plug, a kit player (KitCore.hpp), and the dialogs that load a slot's kit from a file and save one
as an OP-1 or OP-Z kit file, for the Sequencer's menu.
*/
#include "opz/KitCore.hpp"
#include <osdialog.h>

namespace opz {

static PlugRegistration drumPlug({"Drum", DRUM, 0, false, DrumCore::configSound, makeCore<KitCore>});

void loadKit(KitCore* m, int slot, bool folder) {
	osdialog_filters* filters = folder ? nullptr : osdialog_filters_parse("Kits and sounds:aif,aiff,aifc,wav");
	char* chosen = osdialog_file(folder ? OSDIALOG_OPEN_DIR : OSDIALOG_OPEN, nullptr, nullptr, filters);
	if (filters)
		osdialog_filters_free(filters);
	if (!chosen)
		return;
	std::string path = chosen, error;
	std::free(chosen);
	try {
		if (std::unique_ptr<Kit> kit = folder ? Kit::loadFolder(path, &error) : Kit::load(path, &error))
			return m->useKit(slot, std::move(kit));
	}
	catch (const Exception& e) {
		error = e.what();
	}
	osdialog_message(OSDIALOG_WARNING, OSDIALOG_OK, ("Couldn't load " + path + ": " + error).c_str());
}

void saveKit(KitCore* m, int slot) {
	std::vector<uint8_t> file;
	std::string name;
	bool cut = false;
	{
		std::lock_guard<std::mutex> lock(m->kitMutex);
		const Kit* k = m->kitIn(slot);
		if (!k)
			return;
		file = k->toOp1(&cut);
		name = k->name;
	}
	osdialog_filters* filters = osdialog_filters_parse("OP-1 and OP-Z kits:aif");
	char* chosen = osdialog_file(OSDIALOG_SAVE, nullptr, (name + ".aif").c_str(), filters);
	osdialog_filters_free(filters);
	if (!chosen)
		return;
	std::string path = chosen;
	std::free(chosen);
	if (system::getExtension(path).empty())
		path += ".aif";
	try {
		system::writeFile(path, file);
	}
	catch (const Exception& e) {
		return (void) osdialog_message(OSDIALOG_WARNING, OSDIALOG_OK, ("Couldn't write " + path + ": " + e.what()).c_str());
	}
	if (cut)
		osdialog_message(OSDIALOG_INFO, OSDIALOG_OK, "The kit is longer than the 12 seconds a kit file holds, so it was cut short there.");
}

} // namespace opz
