/**
 *
 * Search Functions
 *
 * @author Takuto Yanagida
 * @version 2021-05-09
 *
 */


#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <regex>
#include <windows.h>

#include "item_list.h"
#include "migemo_wrapper.h"
#include "pref.hpp"


class Search {

	Migemo       migemo_;
	ULONGLONG    lastTime_ = 0;
	bool         useMigemo_ = false;
	bool         reserveFind_ = false;
	std::wstring str_;
	std::wstring query_;

public:

	Search() noexcept {}

	bool Initialize(bool useMigemo) {
		useMigemo_ = (useMigemo && migemo_.loadLibrary());
		return useMigemo_;
	}

	const std::wstring& AddKey(wchar_t key) {
		lastTime_ = GetTickCount64();
		str_.append(1, ::_totlower(key));
		reserveFind_ = true;  // Flag the call to findFirst using a timer
		return str_;
	}

	const std::wstring& RemoveKey() {
		if (str_.empty()) return str_;
		lastTime_ = GetTickCount64();
		str_.resize(str_.size() - 1);
		reserveFind_ = !str_.empty();  // Flag the call to findFirst using a timer
		return str_;
	}

	void ClearKey() {
		str_.clear();
		reserveFind_ = false;
	}

	bool IsReserved() noexcept {
		if (reserveFind_) {
			const auto time = GetTickCount64();
			if (time - lastTime_ > 500) return true;
		}
		return false;
	}

	int FindFirst(int cursorIndex, const ItemList& items) {
		reserveFind_ = false;
		if (useMigemo_) {
			migemo_.query(str_, query_);
		}
		return FindNext(cursorIndex, items);
	}

	int FindNext(int cursorIndex, const ItemList& items) {
		bool restart = false;

		int startIndex = cursorIndex + 1;
		if (startIndex == items.Count()) startIndex = 0;

		if (useMigemo_) {
			std::wregex re(query_, std::regex_constants::icase);
			for (int i = startIndex; ; ++i) {
				if (i >= items.Count()) {
					i = 0;
					restart = true;
				}
				if (restart && i == startIndex) break;

				std::wsmatch m{};
				if (std::regex_search(items[i]->Name(), m, re)) {
					return i;
				}
			}
		}
		else {
			for (int i = startIndex; ; ++i) {
				if (i >= items.Count()) {
					i = 0;
					restart = true;
				}
				if (restart && i == startIndex) break;

				std::wstring name(items[i]->Name());
				std::transform(name.begin(), name.end(), name.begin(), ::_totlower);  // Lower case
				if (name.find(str_) != std::wstring::npos) {
					return i;
				}
			}
		}
		return -1;
	}

};
