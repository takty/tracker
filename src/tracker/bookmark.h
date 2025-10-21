/**
 *
 * Bookmarks
 *
 * @author Takuto Yanagida
 * @version 2025-10-22
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

	bool arrange(size_t drag, size_t drop) {
		if (paths_.size() <= drag) return false;
		if (paths_.size() <= drop) return false;
		std::wstring path(paths_[drag]);
		paths_.erase(paths_.begin() + drag);
		paths_.insert(paths_.begin() + drop, path);
		return true;
	}

	void add(const std::wstring& path) {
		paths_.push_back(path);
	}

	void remove(size_t index) noexcept {
		paths_.erase(paths_.begin() + index);
	}

};
