#include "plugin.hpp"
#include "ui.hpp"

Plugin* pluginInstance;

// Called once when Rack loads the plugin. Every Model registered here must also be listed in plugin.json,
// with a matching slug, or Rack refuses to load the plugin.
void init(Plugin* p) {
	pluginInstance = p;
	p->addModel(modelBreakout);
	p->addModel(modelDrumBreakout);
	p->addModel(modelFxBreakout);
	p->addModel(modelScreen);
	p->addModel(modelSequencer);
}


namespace keys {

Style style;

static const char* const GLOW_IDS[GLOWS] = {"halo", "reflection", "outline", "rim", "none", "fill"};
static const char* const DOT_IDS[DOTS] = {"1.7", "2.2", "2.6", "2.1.4"};
static const char* const LED_IDS[LEDS] = {"tight", "grey", "lens", "window"};

/** The index of `j`'s string among `ids`, or `fallback`. */
static int find(json_t* j, const char* const* ids, int n, int fallback) {
	const char* s = json_string_value(j);
	for (int i = 0; s && i < n; i++)
		if (!std::strcmp(s, ids[i]))
			return i;
	return fallback;
}

} // namespace keys

// The key style, in Rack's settings.json under this plugin's slug.
json_t* settingsToJson() {
	using namespace keys;
	json_t* root = json_object();
	json_object_set_new(root, "keyGlow", json_string(GLOW_IDS[style.glow]));
	json_object_set_new(root, "keyPressedIn", json_boolean(style.pressedIn));
	json_object_set_new(root, "keyDot", json_string(DOT_IDS[style.dot]));
	json_object_set_new(root, "keyLed", json_string(LED_IDS[style.led]));
	return root;
}

void settingsFromJson(json_t* root) {
	using namespace keys;
	Style defaults;
	style.glow = find(json_object_get(root, "keyGlow"), GLOW_IDS, GLOWS, defaults.glow);
	json_t* pressedIn = json_object_get(root, "keyPressedIn");
	style.pressedIn = pressedIn ? json_boolean_value(pressedIn) : defaults.pressedIn;
	style.dot = find(json_object_get(root, "keyDot"), DOT_IDS, DOTS, defaults.dot);
	style.led = find(json_object_get(root, "keyLed"), LED_IDS, LEDS, defaults.led);
}
