/**
 * Document Options
 *
 * @author Takuto Yanagida
 * @version 2025-10-21
 */

#pragma once

#include <vector>

#include "comparator.h"
#include "pref.hpp"
#include "item_list.h"

class Option {

	bool showHidden_, sortRev_;
	int sortBy_;

	bool sortHis_, sortHisRev_;
	int sortHisBy_;

public:

	enum {sbName, sbType, sbDate, sbSize};  // Sort type

	void Restore(Pref& pref) noexcept {
		pref.set_current_section(SECTION_WINDOW);
		showHidden_ = pref.item_int(KEY_SHOW_HIDDEN, VAL_SHOW_HIDDEN) != 0;
		sortRev_    = pref.item_int(KEY_SORT_REV,    VAL_SORT_REV)    != 0;
		sortBy_     = pref.item_int(KEY_SORT_BY,     VAL_SORT_BY);

		sortHis_    = pref.item_int(KEY_SORT_HISTORY,     VAL_SORT_HISTORY)     != 0;
		sortHisRev_ = pref.item_int(KEY_SORT_HISTORY_REV, VAL_SORT_HISTORY_REV) != 0;
		sortHisBy_  = pref.item_int(KEY_SORT_HISTORY_BY,  VAL_SORT_HISTORY_BY);
	}

	void Store(Pref& pref) const noexcept {
		pref.set_item_int(SECTION_WINDOW, KEY_SHOW_HIDDEN, showHidden_);
		pref.set_item_int(SECTION_WINDOW, KEY_SORT_REV,    sortRev_);
		pref.set_item_int(SECTION_WINDOW, KEY_SORT_BY,     sortBy_);

		pref.set_item_int(SECTION_WINDOW, KEY_SORT_HISTORY,     sortHis_);
		pref.set_item_int(SECTION_WINDOW, KEY_SORT_HISTORY_REV, sortHisRev_);
		pref.set_item_int(SECTION_WINDOW, KEY_SORT_HISTORY_BY,  sortHisBy_);
	}

	int GetSortType() const noexcept {
		return sortBy_;
	}

	TCHAR GetSortTypeChar() const noexcept {
		switch (sortBy_) {
		case sbName: return sortRev_ ? 'N' : 'n';
		case sbType: return sortRev_ ? 'T' : 't';
		case sbDate: return sortRev_ ? 'D' : 'd';
		case sbSize: return sortRev_ ? 'S' : 's';
		default:     return sortRev_ ? 'N' : 'n';
		}
	}

	bool GetSortOrder() const noexcept {
		return sortRev_;
	}

	bool IsShowHidden() const noexcept {
		return showHidden_;
	}

	int SetSortType(int t) noexcept {
		return sortBy_ = t;
	}

	bool SetSortOrder(bool f) noexcept {
		return sortRev_ = f;
	}

	bool SetShowHidden(bool f) noexcept {
		return showHidden_ = f;
	}

	void SortFiles(ItemList& files) const {
		switch (sortBy_) {
		case 0: files.Sort(CompByName(sortRev_)); break;
		case 1: files.Sort(CompByType(sortRev_)); break;
		case 2: files.Sort(CompByDate(sortRev_)); break;
		case 3: files.Sort(CompBySize(sortRev_)); break;
		default: break;
		}
	}

	void SortHistory(ItemList& files) const {
		if (!sortHis_) return;
		switch (sortHisBy_) {
		case 0: files.Sort(CompByName(sortHisRev_)); break;
		case 1: files.Sort(CompByType(sortHisRev_)); break;
		case 2: files.Sort(CompByDate(sortHisRev_)); break;
		case 3: files.Sort(CompBySize(sortHisRev_)); break;
		default: break;
		}
	}

};
