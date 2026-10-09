# Path to the Rack SDK (or a Rack source tree). Override on the command line or in the environment:
#   make RACK_DIR=~/Rack-SDK
RACK_DIR ?= ../..

# Passed to both the C and C++ compiler
FLAGS +=
CFLAGS +=
CXXFLAGS +=
# Appended after the SDK's -std=c++11, so it wins. macOS builds target 10.9, so avoid the library parts of C++17
# that need a newer libc++ (std::optional::value, std::get on a variant, std::any_cast).
EXTRA_CXXFLAGS += -std=c++17

# Link static libraries only: you can't assume anything about the user's shared library search path.
LDFLAGS +=

SOURCES += $(wildcard src/*.cpp)

# Files packaged by `make dist` alongside the plugin binary and plugin.json
DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)
DISTRIBUTABLES += $(wildcard presets)

# `make panels` needs only Python, not the SDK
ifneq ($(MAKECMDGOALS),panels)
include $(RACK_DIR)/plugin.mk
endif

# Regenerate the panels and component graphics in res/ (needs Python 3 and fontTools)
panels:
	python3 res-src/panel.py

.PHONY: panels
