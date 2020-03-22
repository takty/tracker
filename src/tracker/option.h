/**
 *
 * Document Options
 *
 * @author Takuto Yanagida
 * @version 2020-03-22
 *
 */


#pragma once

#include <vector>

#include "comparator.h"
#include "pref.hpp"
#include "item_list.h"


class Option {

	bool showHidden_, sortOrder_;
	int sortType_;

public:

	enum {sbName, sbType, sbDate, sbSize};  // Sort type

	void Restore(Pref& pref) {
		pref.set_current_section(SECTION_WINDOW);
		showHidden_ = (pref.item_int(KEY_SHOW_HIDDEN, VAL_SHOW_HIDDEN) != 0);
		sortOrder_  = (pref.item_int(KEY_SORT_REV, VAL_SORT_REV) != 0);
		sortType_   = pref.item_int(KEY_SORT_BY, VAL_SORT_BY);
	}

	void Store(Pref& pref) {
		pref.set_item_int(SECTION_WINDOW, KEY_SHOW_HIDDEN, showHidden_);
		pref.set_item_int(SECTION_WINDOW, KEY_SORT_REV, sortOrder_);
		pref.set_item_int(SECTION_WINDOW, KEY_SORT_BY, sortType_);
	}

	int GetSortType() {
		return sortType_;
	}

	bool GetSortOrder() {
		return sortOrder_;
	}

	bool IsShowHidden() {
		return showHidden_;
	}

	int SetSortType(int t) {
		return sortType_ = t;
	}

	bool SetSortOrder(bool f) {
		return sortOrder_ = f;
	}

	bool SetShowHidden(bool f) {
		return showHidden_ = f;
	}

	void SortFiles(ItemList& files) {
		switch (sortType_) {
		case 0: files.Sort(CompByName(sortOrder_)); break;
		case 1: files.Sort(CompByType(sortOrder_)); break;
		case 2: files.Sort(CompByDate(sortOrder_)); break;
		case 3: files.Sort(CompBySize(sortOrder_)); break;
		}
	}

};
