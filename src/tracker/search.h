/**
 * Search Functions
 *
 * @author Takuto Yanagida
 * @version 2025-11-20
 */

#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <optional>
#include <regex>
#include <chrono>

#include "gsl/gsl"
#include "item_list.h"
#include "migemo_wrapper.h"
#include "pref.hpp"

class Search {

	static unsigned long long get_elapsed_time_ms() noexcept {
		const auto now = std::chrono::steady_clock::now();
		auto dur = now.time_since_epoch();
		auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
		return gsl::narrow<unsigned long long>(ms);
	}

	static std::wstring escape_regex_special_chars(const std::wstring& s) {
		std::wstring res;
		const std::wstring scs = L"\\^$.|?*+()[]{}/";

		for (const wchar_t c : s) {
			if (scs.find(c) != std::wstring::npos) {
				res += L'\\';
			}
			res += c;
		}
		return res;
	}

	Migemo             migemo_;
	unsigned long long last_key_search_time_ = 0;
	bool               use_migemo_           = false;
	bool               reserve_find_         = false;
	std::wstring       search_word_;
	std::wstring       migemo_pattern_;

public:

	Search() noexcept = default;

	bool initialize(bool use_migemo) {
		use_migemo_ = (use_migemo && migemo_.load_library());
		return use_migemo_;
	}

	// Key input search
	void key_search(wchar_t key) {
		const auto time = get_elapsed_time_ms();
		if (time - last_key_search_time_ > 1000) {
			search_word_.clear();
		}
		search_word_.append(1, key);
		last_key_search_time_ = time;
		reserve_find_         = true;  // Flag the call to findFirst using a timer
	}

	bool is_reserved() const noexcept {
		if (reserve_find_) {
			const auto time = get_elapsed_time_ms();
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

		std::wregex pat;
		if (use_migemo_) {
			pat.assign(migemo_pattern_, std::regex_constants::ECMAScript | std::regex_constants::icase);
		} else {
			pat.assign(escape_regex_special_chars(search_word_), std::regex_constants::ECMAScript | std::regex_constants::icase);
		}
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
		return jump_to;
	}

};
