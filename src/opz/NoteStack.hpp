#pragma once
#include <algorithm>

namespace opz {

/** The channels holding a note down, oldest first. In mono and legato the newest one plays, and releasing it falls
back to the one held before. */
struct NoteStack {
	int channels[16];
	int size = 0;

	bool empty() const {
		return size == 0;
	}

	int top() const {
		return channels[size - 1];
	}

	void push(int channel) {
		remove(channel);
		channels[size++] = channel;
	}

	void remove(int channel) {
		size = std::remove(channels, channels + size, channel) - channels;
	}

	void clear() {
		size = 0;
	}
};

} // namespace opz
