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

	Bookmark() noexcept = default;

	void restore(Pref& pref) {
		paths_ = pref.items<std::vector<std::wstring>>(SECTION_BOOKMARK, KEY_FILE, MAX_BOOKMARK);
	}

	void store(Pref& pref) const {
		pref.set_items(paths_, SECTION_BOOKMARK, KEY_FILE);
	}

	size_t size() noexcept {
		return paths_.size();
	}

	std::wstring& operator[](size_t index) noexcept {
		return paths_[index];
	}

	bool arrange(int drag, int drop) {
		if (drag < 0 || static_cast<size_t>(drag) >= paths_.size()) return false;
		if (drop < 0 || static_cast<size_t>(drop) >= paths_.size()) return false;
		std::wstring path(paths_[drag]);
		paths_.erase(paths_.begin() + drag);
		paths_.insert(paths_.begin() + drop, path);
		return true;
	}

	void add(const std::wstring& path) {
		paths_.push_back(path);
	}

	void remove(int index) noexcept {
		paths_.erase(paths_.begin() + index);
	}

};
