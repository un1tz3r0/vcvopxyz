#pragma once
#include "SynthTrack.hpp"
#include "TrackWidget.hpp"

namespace opz {

template <class Engine>
struct SynthTrackWidget : TrackWidget {
	SynthTrackWidget(SynthTrack<Engine>* module) : TrackWidget(module, Engine::SLUG) {}
};

} // namespace opz
