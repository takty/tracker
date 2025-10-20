/**
 *
 * Search Functions
 *
 * @author Takuto Yanagida
 * @version 2025-10-21
 *
 */


#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include <algorithm>

#include "item_list.h"
#include "migemo_wrapper.h"
#include "regex.h"
#include "pref.hpp"


class Search {

	Migemo       migemo_;
	Regex        regex_;
	ULONGLONG    lastKeySearchTime_ = 0;
	bool         useMigemo_         = false;
	bool         reserveFind_       = false;
	std::wstring searchWord_;
	std::string  mkey_;  // Always ANSI

public:

	Search() {
	}

	bool Initialize(bool useMigemo) {
		useMigemo_ = (useMigemo && regex_.loadLibrary() && migemo_.loadLibrary());
		return useMigemo_;
	}

	// Key input search
	void KeySearch(int key) {
		auto time = GetTickCount64();
		if (time - lastKeySearchTime_ > 1000) searchWord_.clear();
		lastKeySearchTime_ = time;
		searchWord_.append(1, ::_totlower((wint_t)key));
		reserveFind_ = true;  // Flag the call to findFirst using a timer
	}

	bool IsReserved() {
		if (reserveFind_) {
			auto time = GetTickCount64();
			if (time - lastKeySearchTime_ > 500) return true;
		}
		return false;
	}

	int FindFirst(int cursorIndex, const ItemList& items) {
		reserveFind_ = false;
		if (useMigemo_) migemo_.query(searchWord_, mkey_);
		return FindNext(cursorIndex, items);
	}

	int FindNext(int cursorIndex, const ItemList& items) {
		int jumpTo = -1;
		bool restart = false;

		size_t startIndex = (cursorIndex == -1) ? 0 : cursorIndex + 1;
		if (startIndex == items.Count()) startIndex = 0;

		if (useMigemo_) {
			Pattern pat(regex_, mkey_);
			for (size_t i = startIndex; ; ++i) {
				if (i >= items.Count()) {
					i = 0;
					restart = true;
				}
				if (restart && i == startIndex) break;

				if (pat.match(items[i]->Name())) {
					jumpTo = i;
					break;
				}
			}
		} else {
			for (size_t i = startIndex; ; ++i) {
				if (i >= items.Count()) {
					i = 0;
					restart = true;
				}
				if (restart && i == startIndex) break;

				std::wstring name(items[i]->Name());
				transform(name.begin(), name.end(), name.begin(), ::_totlower);  // Lower case
				if (name.find(searchWord_) != std::wstring::npos) {
					jumpTo = i;
					break;
				}
			}
		}
		return jumpTo;
	}

};
