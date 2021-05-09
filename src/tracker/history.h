/**
 *
 * History
 *
 * @author Takuto Yanagida
 * @version 2021-05-09
 *
 */


#pragma once

#include <vector>
#include <string>
#include <algorithm>

#include "file_utils.hpp"
#include "pref.hpp"


class History {

	std::vector<std::wstring> paths_;
	int max_size_{};

public:

	const std::wstring PATH{ L":HISTORY" }, NAME{ L"History" };

	History() noexcept(false) {}

	void initialize(Pref& pref) noexcept {
		max_size_ = pref.item_int(KEY_MAX_HISTORY, VAL_MAX_HISTORY);
	}

	void restore(Pref& pref) {
		paths_ = pref.items<std::vector<std::wstring>>(SECTION_HISTORY, KEY_FILE, MAX_HISTORY);
	}

	void store(Pref& pref) {
		pref.set_items(paths_, SECTION_HISTORY, KEY_FILE);
	}

	int size() noexcept {
		return paths_.size();
	}

	std::wstring& operator[](int index) noexcept(false) {
		return paths_.at(index);
	}

	void add(const std::wstring& path) {
		auto root = path.substr(0, 1) + L":\\";
		if (FileSystem::is_removable(root)) return;  // Do not leave removable

		paths_.erase(remove(paths_.begin(), paths_.end(), path), paths_.end());  // Delete the same history
		paths_.insert(paths_.begin(), path);  // Add

		if (static_cast<int>(paths_.size()) > max_size_) {  // Maximum number of history limit
			paths_.resize(max_size_);
		}
	}

	void clean_up() {
		// Delete a nonexistent path
		paths_.erase(remove_if(paths_.begin(), paths_.end(), [](std::wstring& p) {return !FileSystem::is_existing(p); }), paths_.end());
	}

	void clear() noexcept {
		paths_.clear();
	}

};
