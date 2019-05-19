#pragma once

#include <vector>
#include <string>
#include <algorithm>

#include "file_utils.hpp"
#include "pref.hpp"


//
// History
// 2019-04-12
//

class History {

	std::vector<std::wstring> paths_;
	int max_size_ = 0;

public:

	const std::wstring PATH{ L":HISTORY" }, NAME{ L"History" };

	History() {}

	void initialize(Pref& pref) {
		max_size_ = pref.item_int(KEY_RECENT_NUM, VAL_RECENT_NUM);
	}

	void restore(Pref& pref) {
		paths_ = pref.items<std::vector<std::wstring>>(SECTION_HISTORY, KEY_FILE, MAX_HISTORY);
	}

	void store(Pref& pref) {
		pref.set_items(paths_, SECTION_HISTORY, KEY_FILE);
	}

	int size() {
		return paths_.size();
	}

	std::wstring& operator[](int index) {
		return paths_[index];
	}

	void add(const std::wstring& path) {
		auto root = path[0] + L":\\";
		if (FileSystem::is_removable(root)) return;  // Do not leave removable

		paths_.erase(remove(paths_.begin(), paths_.end(), path), paths_.end());  // Delete the same history
		paths_.insert(paths_.begin(), path);  // Add

		if ((int)paths_.size() > max_size_) {  // Maximum number of history limit
			paths_.resize(max_size_);
		}
	}

	void clean_up() {
		// Delete a nonexistent path
		paths_.erase(remove_if(paths_.begin(), paths_.end(), [](std::wstring& p) {return !FileSystem::is_existing(p); }), paths_.end());
	}

	void clear() {
		paths_.clear();
	}

};
