#pragma once
#include <algorithm>

namespace opz {

/** The notes held down, oldest first: gate channels, or keys of the panel's keyboard. In mono and legato the newest
one plays, and releasing it falls back to the one held before. */
struct NoteStack {
	static constexpr int CAPACITY = 24;
	int notes[CAPACITY];
	int size = 0;

	bool empty() const {
		return size == 0;
	}

	int top() const {
		return notes[size - 1];
	}

	void push(int note) {
		remove(note);
		notes[size++] = note;
	}

	void remove(int note) {
		size = std::remove(notes, notes + size, note) - notes;
	}

	void clear() {
		size = 0;
	}
};

} // namespace opz
