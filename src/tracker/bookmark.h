/**
 *
 * Bookmarks
 *
 * @author Takuto Yanagida
 * @version 2025-10-21
 *
 */


#pragma once

#include <vector>
#include <string>

#include "Pref.hpp"


class Bookmark {

	std::vector<std::wstring> paths_;

public:

	const std::wstring PATH{ L":BOOKMARK" }, NAME{ L"Bookmark" };

	Bookmark() {}

	void restore(Pref& pref) {
		paths_ = pref.items<std::vector<std::wstring>>(SECTION_BOOKMARK, KEY_FILE, MAX_BOOKMARK);
	}

	void store(Pref& pref) {
		pref.set_items(paths_, SECTION_BOOKMARK, KEY_FILE);
	}

	size_t size() {
		return paths_.size();
	}

	std::wstring& operator[](size_t index) {
		return paths_[index];
	}

	bool arrange(int drag, int drop) {
		if (drag < 0 || drag >= (int)paths_.size()) return false;
		if (drop < 0 || drop >= (int)paths_.size()) return false;
		std::wstring path(paths_[drag]);
		paths_.erase(paths_.begin() + drag);
		paths_.insert(paths_.begin() + drop, path);
		return true;
	}

	void add(const std::wstring& path) {
		paths_.push_back(path);
	}

	void remove(int index) {
		paths_.erase(paths_.begin() + index);
	}

};
