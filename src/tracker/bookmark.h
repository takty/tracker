/**
 *
 * Bookmarks
 *
 * @author Takuto Yanagida
 * @version 2021-05-30
 *
 */


#pragma once

#include <vector>
#include <string>
#include <iterator>

#include "pref.hpp"


class Bookmark {

	std::vector<std::wstring> paths_;

public:

	const std::wstring PATH{ L":BOOKMARK" }, NAME{ L"Bookmark" };

	Bookmark() noexcept {}

	void restore(const Pref& data) noexcept(false) {
		paths_ = data.load_lines<std::vector<std::wstring>>();
	}

	void store(const Pref& data) noexcept(false) {
		data.save_lines(paths_);
	}

	// ---------------------------------------------------------------------------

	auto size() const noexcept {
		return paths_.size();
	}

	auto at(const size_t index) noexcept {
		return paths_.at(index);
	}

	auto begin() noexcept {
		return paths_.begin();
	}

	auto end() noexcept {
		return paths_.end();
	}

	// ---------------------------------------------------------------------------

	void add(const std::wstring& path) noexcept {
		paths_.push_back(path);
	}

	void remove(const size_t index) noexcept {
		paths_.erase(paths_.begin() + index);
	}

	bool arrange(const size_t drag, const size_t drop) noexcept {
		if (drag >= paths_.size()) return false;
		if (drop >= paths_.size()) return false;
		std::wstring path{ paths_.at(drag) };
		paths_.erase(paths_.begin() + drag);
		paths_.insert(paths_.begin() + drop, path);
		return true;
	}

};
