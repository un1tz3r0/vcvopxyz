#include "plugin.hpp"

Plugin* pluginInstance;

// Called once when Rack loads the plugin. Every Model registered here must also be listed in plugin.json,
// with a matching slug, or Rack refuses to load the plugin.
void init(Plugin* p) {
	pluginInstance = p;
	p->addModel(modelDrum);
	p->addModel(modelDigital);
	p->addModel(modelAnalog);
	p->addModel(modelCluster);
	p->addModel(modelOrgan);
	p->addModel(modelBreakout);
	p->addModel(modelDrumBreakout);
	p->addModel(modelDelay);
	p->addModel(modelReverb);
	p->addModel(modelRymd);
	p->addModel(modelChorus);
	p->addModel(modelCrush);
	p->addModel(modelDist);
	p->addModel(modelScreen);
	p->addModel(modelSequencer);
}
