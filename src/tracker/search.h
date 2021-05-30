/**
 *
 * Search Functions
 *
 * @author Takuto Yanagida
 * @version 2021-05-30
 *
 */


#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <regex>
#include <optional>
#include <windows.h>

#include "item_list.h"
#include "migemo_wrapper.h"
#include "pref.hpp"


class Search {

	Migemo       migemo_;
	ULONGLONG    last_time_ = 0;
	bool         use_migemo_ = false;
	bool         reserve_find_ = false;
	std::wstring str_;
	std::wstring query_;

public:

	Search() noexcept {}

	bool initialize(bool use_migemo) noexcept {
		use_migemo_ = (use_migemo && migemo_.load_library());
		return use_migemo_;
	}

	const std::wstring& add_key(wchar_t key) noexcept {
		last_time_ = ::GetTickCount64();
		str_.append(1, ::_totlower(key));
		reserve_find_ = true;  // Flag the call to findFirst using a timer
		return str_;
	}

	const std::wstring& remove_key() noexcept {
		if (str_.empty()) return str_;
		last_time_ = ::GetTickCount64();
		str_.resize(str_.size() - 1);
		reserve_find_ = !str_.empty();  // Flag the call to findFirst using a timer
		return str_;
	}

	void clear_key() noexcept {
		str_.clear();
		reserve_find_ = false;
	}

	bool is_reserved() noexcept {
		if (reserve_find_) {
			const auto time = GetTickCount64();
			if (time - last_time_ > 500) return true;
		}
		return false;
	}

	std::optional<size_t> find_first(const size_t from, const ItemList& items) noexcept {
		reserve_find_ = false;
		if (use_migemo_) {
			migemo_.query(str_, query_);
		}
		return find_next(from, items);
	}

	std::optional<size_t> find_next(size_t from, const ItemList& items) noexcept {
		bool restart = false;
		if (from == items.size()) from = 0U;

		if (use_migemo_) {
			std::wregex re(query_, std::regex_constants::icase);
			for (size_t i = from; ; ++i) {
				if (i >= items.size()) {
					i = 0U;
					restart = true;
				}
				if (restart && i == from) break;

				std::wsmatch m{};
				if (std::regex_search(items.at(i)->name(), m, re)) {
					return i;
				}
			}
		}
		else {
			for (size_t i = from; ; ++i) {
				if (i >= items.size()) {
					i = 0U;
					restart = true;
				}
				if (restart && i == from) break;

				std::wstring name{ items.at(i)->name() };
				std::transform(name.begin(), name.end(), name.begin(), ::_totlower);  // Lower case
				if (name.find(str_) != std::wstring::npos) {
					return i;
				}
			}
		}
		return std::nullopt;
	}

};
