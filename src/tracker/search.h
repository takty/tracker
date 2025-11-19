/**
 * Search Functions
 *
 * @author Takuto Yanagida
 * @version 2025-11-19
 */

#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include <cwctype>
#include <algorithm>
#include <optional>
#include <regex>

#include "gsl/gsl"

#include "item_list.h"
#include "migemo_wrapper.h"
#include "pref.hpp"

class Search {

	Migemo       migemo_;
	ULONGLONG    last_key_search_time_ = 0;
	bool         use_migemo_           = false;
	bool         reserve_find_         = false;
	std::wstring search_word_;
	std::wstring migemo_pattern_;

public:

	Search() noexcept = default;

	bool initialize(bool useMigemo) {
		use_migemo_ = (useMigemo && migemo_.load_library());
		return use_migemo_;
	}

	// Key input search
	void key_search(WPARAM key) {
		const auto time = GetTickCount64();
		if (time - last_key_search_time_ > 1000) {
			search_word_.clear();
		}
		search_word_.append(1, std::towlower(gsl::narrow<wint_t>(key)));
		last_key_search_time_ = time;
		reserve_find_       = true;  // Flag the call to findFirst using a timer
	}

	bool is_reserved() const noexcept {
		if (reserve_find_) {
			const auto time = GetTickCount64();
			if (time - last_key_search_time_ > 500) return true;
		}
		return false;
	}

	std::optional<size_t> find_first(std::optional<size_t> cursor_idx, const ItemList& items) {
		reserve_find_ = false;
		if (use_migemo_) {
			migemo_.query(search_word_, migemo_pattern_);
		}
		return find_next(cursor_idx, items);
	}

	std::optional<size_t> find_next(std::optional<size_t> cursor_idx, const ItemList& items) const {
		std::optional<size_t> jump_to;
		bool restart = false;

		size_t start_idx = (!cursor_idx) ? 0 : cursor_idx.value() + 1;
		if (start_idx == items.size()) start_idx = 0;

		if (use_migemo_) {
			std::wregex pat(migemo_pattern_, std::regex_constants::ECMAScript | std::regex_constants::icase);

			for (size_t i = start_idx; ; ++i) {
				if (i >= items.size()) {
					i = 0;
					restart = true;
				}
				if (restart && i == start_idx) break;

				if (std::regex_search(items.at(i)->name(), pat)) {
					jump_to = i;
					break;
				}
			}
		} else {
			for (size_t i = start_idx; ; ++i) {
				if (i >= items.size()) {
					i = 0;
					restart = true;
				}
				if (restart && i == start_idx) break;

				std::wstring name(items.at(i)->name());
				transform(name.begin(), name.end(), name.begin(), std::towlower);  // Lower case
				if (name.find(search_word_) != std::wstring::npos) {
					jump_to = i;
					break;
				}
			}
		}
		return jump_to;
	}

};
