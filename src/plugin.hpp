#pragma once
#include <rack.hpp>

using namespace rack;

// Set by init() in plugin.cpp; asset::plugin(pluginInstance, "res/...") resolves paths inside this plugin.
extern Plugin* pluginInstance;

// One Model per module. Each is defined at the bottom of its module's .cpp and registered in plugin.cpp.
extern Model* modelDrum;
extern Model* modelDigital;
extern Model* modelAnalog;
extern Model* modelCluster;
extern Model* modelOrgan;
extern Model* modelBreakout;
extern Model* modelDrumBreakout;
extern Model* modelDelay;
extern Model* modelReverb;
extern Model* modelRymd;
extern Model* modelChorus;
extern Model* modelCrush;
extern Model* modelDist;
extern Model* modelScreen;
extern Model* modelSequencer;
