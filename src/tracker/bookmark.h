/**
 * Bookmarks
 *
 * @author Takuto Yanagida
 * @version 2025-11-10
 */

#pragma once

#include <vector>
#include <string>

#include "pref.hpp"
#include "text_reader_writer.hpp"

class Bookmark {

	inline static const std::wstring FILE_NAME{ L"bookmark.txt" };

	const std::wstring path_;
	std::vector<std::wstring> paths_;

public:

	inline static const std::wstring PATH{ L":BOOKMARK" }, NAME{ L"Bookmark" };

	Bookmark(const std::wstring& iniPath) noexcept : path_(path::parent(iniPath).append(L"\\").append(FILE_NAME)) {}

	void restore(Pref& pref) {
		paths_ = text_reader_writer::read(path_);
		if (paths_.empty()) {
			paths_ = pref.items<std::vector<std::wstring>>(SECTION_BOOKMARK, KEY_FILE, MAX_BOOKMARK);
		}
	}

	void store() const {
		text_reader_writer::write(path_, paths_);
	}

	size_t size() noexcept {
		return paths_.size();
	}

	std::wstring& operator[](size_t index) {
		return paths_.at(index);
	}

	bool arrange(size_t drag, size_t drop) {
		if (paths_.size() <= drag) return false;
		if (paths_.size() <= drop) return false;
		std::wstring path(paths_.at(drag));
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
