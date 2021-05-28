/**
 *
 * Bookmarks
 *
 * @author Takuto Yanagida
 * @version 2021-05-16
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

	Bookmark() noexcept(false) {}

	void restore(const Pref& data) {
		paths_ = data.load_lines<std::vector<std::wstring>>();
	}

	void store(const Pref& data) {
		data.save_lines(paths_);
	}

	int size() noexcept {
		return paths_.size();
	}

	std::wstring& operator[](int index) noexcept(false) {
		return paths_.at(index);
	}

	bool arrange(int drag, int drop) {
		if (drag < 0 || drag >= std::ssize(paths_)) return false;
		if (drop < 0 || drop >= std::ssize(paths_)) return false;
		std::wstring path(paths_.at(drag));
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
