/**
 *
 * History
 *
 * @author Takuto Yanagida
 * @version 2025-10-26
 *
 */


#pragma once

#include <vector>
#include <string>
#include <algorithm>

#include "file_utils.hpp"
#include "pref.hpp"
#include "text_reader_writer.hpp"


class History {

	const std::wstring FILE_NAME{ L"history.txt" };

	const std::wstring path_;
	std::vector<std::wstring> paths_;
	size_t max_size_ = 0;

public:

	const std::wstring PATH{ L":HISTORY" }, NAME{ L"History" };

	History(const std::wstring& iniPath) noexcept : path_(Path::parent(iniPath).append(L"\\").append(FILE_NAME)) {}

	void initialize(Pref& pref) noexcept {
		max_size_ = pref.item_int(KEY_MAX_HISTORY, VAL_MAX_HISTORY);
	}

	void restore(Pref& pref) {
		paths_ = TextReaderWriter::read(path_);
		if (paths_.empty()) {
			paths_ = pref.items<std::vector<std::wstring>>(SECTION_HISTORY, KEY_FILE, MAX_HISTORY);
		}
	}

	void store() const {
		TextReaderWriter::write(path_, paths_);
	}

	size_t size() noexcept {
		return paths_.size();
	}

	std::wstring& operator[](size_t index) {
		return paths_.at(index);
	}

	void add(const std::wstring& path) {
		auto root = std::wstring(1, path.front()) + L":\\";
		if (FileSystem::is_removable(root)) return;  // Do not leave removable

		paths_.erase(remove(paths_.begin(), paths_.end(), path), paths_.end());  // Delete the same history
		paths_.insert(paths_.begin(), path);  // Add

		if (paths_.size() > max_size_) {  // Maximum number of history limit
			paths_.resize(max_size_);
		}
	}

	void clean_up() {
		// Delete a nonexistent path
		paths_.erase(
			remove_if(
				paths_.begin(),
				paths_.end(),
				[](std::wstring& p) noexcept {return !FileSystem::is_existing(p); }
			),
			paths_.end()
		);
	}

	void clear() noexcept {
		paths_.clear();
	}

};
