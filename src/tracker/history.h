/**
 *
 * History
 *
 * @author Takuto Yanagida
 * @version 2021-05-30
 *
 */


#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <iterator>

#include "file_utils.hpp"
#include "pref.hpp"


class History {

	std::vector<std::wstring> paths_;
	size_t max_size_{};

public:

	const std::wstring PATH{ L":HISTORY" }, NAME{ L"History" };

	History() noexcept {}

	void initialize(const Pref& pref) noexcept {
		max_size_ = gsl::narrow_cast<size_t>(pref.get(SECTION_HISTORY, KEY_MAX_HISTORY, VAL_MAX_HISTORY));
	}

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

	auto at(size_t index) noexcept {
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
		auto root = path.substr(0, 1) + L":\\";
		if (FileSystem::is_removable(root)) return;  // Do not leave removable

		paths_.erase(remove(paths_.begin(), paths_.end(), path), paths_.end());  // Delete the same history
		paths_.insert(paths_.begin(), path);  // Add

		if (paths_.size() > max_size_) {  // Maximum number of history limit
			paths_.resize(max_size_);
		}
	}

	void clean_up() noexcept {
		// Delete a nonexistent path
		paths_.erase(
			remove_if(
				paths_.begin(),
				paths_.end(),
				[](std::wstring& p) { return !FileSystem::exists(p); }
			),
			paths_.end()
		);
	}

	void clear() noexcept {
		paths_.clear();
	}

};
