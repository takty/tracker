/**
 * Search Functions
 *
 * @author Takuto Yanagida
 * @version 2025-11-09
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
	ULONGLONG    lastKeySearchTime_ = 0;
	bool         useMigemo_         = false;
	bool         reserveFind_       = false;
	std::wstring searchWord_;
	std::wstring migemoPattern_;

public:

	Search() noexcept = default;

	bool Initialize(bool useMigemo) {
		useMigemo_ = (useMigemo && migemo_.loadLibrary());
		return useMigemo_;
	}

	// Key input search
	void KeySearch(WPARAM key) {
		const auto time = GetTickCount64();
		if (time - lastKeySearchTime_ > 1000) {
			searchWord_.clear();
		}
		searchWord_.append(1, std::towlower(gsl::narrow<wint_t>(key)));
		lastKeySearchTime_ = time;
		reserveFind_       = true;  // Flag the call to findFirst using a timer
	}

	bool IsReserved() const noexcept {
		if (reserveFind_) {
			const auto time = GetTickCount64();
			if (time - lastKeySearchTime_ > 500) return true;
		}
		return false;
	}

	std::optional<size_t> FindFirst(std::optional<size_t> cursorIndex, const ItemList& items) {
		reserveFind_ = false;
		if (useMigemo_) {
			migemo_.query(searchWord_, migemoPattern_);
		}
		return FindNext(cursorIndex, items);
	}

	std::optional<size_t> FindNext(std::optional<size_t> cursorIndex, const ItemList& items) const {
		std::optional<size_t> jumpTo;
		bool restart = false;

		size_t startIndex = (!cursorIndex) ? 0 : cursorIndex.value() + 1;
		if (startIndex == items.size()) startIndex = 0;

		if (useMigemo_) {
			std::wregex pat(migemoPattern_, std::regex_constants::ECMAScript | std::regex_constants::icase);

			for (size_t i = startIndex; ; ++i) {
				if (i >= items.size()) {
					i = 0;
					restart = true;
				}
				if (restart && i == startIndex) break;

				if (std::regex_search(items.at(i)->name(), pat)) {
					jumpTo = i;
					break;
				}
			}
		} else {
			for (size_t i = startIndex; ; ++i) {
				if (i >= items.size()) {
					i = 0;
					restart = true;
				}
				if (restart && i == startIndex) break;

				std::wstring name(items.at(i)->name());
				transform(name.begin(), name.end(), name.begin(), std::towlower);  // Lower case
				if (name.find(searchWord_) != std::wstring::npos) {
					jumpTo = i;
					break;
				}
			}
		}
		return jumpTo;
	}

};
