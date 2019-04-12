#pragma once

#include <vector>

#include "comparator.h"
#include "pref.hpp"
#include "item_list.h"


//
// Document Options
// 2019-04-12
//

class Opt {

	bool showHidden_, sortOrder_;
	int sortType_;

public:

	enum {sbName, sbType, sbDate, sbSize};  // Sort type

	void Restore(Pref& pref)
	{
		pref.set_current_section(SECTION_WINDOW);
		showHidden_ = (pref.item_int(KEY_SHOW_HIDDEN, VAL_SHOW_HIDDEN) != 0);
		sortOrder_  = (pref.item_int(KEY_SORT_REV, VAL_SORT_REV) != 0);
		sortType_   = pref.item_int(KEY_SORT_BY, VAL_SORT_BY);
	}

	void Store(Pref& pref)
	{
		pref.set_item_int(SECTION_WINDOW, KEY_SHOW_HIDDEN, showHidden_);
		pref.set_item_int(SECTION_WINDOW, KEY_SORT_REV, sortOrder_);
		pref.set_item_int(SECTION_WINDOW, KEY_SORT_BY, sortType_);
	}

	// Return sort type
	int GetSortType()
	{
		return sortType_;
	}

	// Return sort order
	bool GetSortOrder()
	{
		return sortOrder_;
	}

	// Return whether to handle hidden files
	bool IsShowHidden()
	{
		return showHidden_;
	}

	// Set sort type
	int SetSortType(int t)
	{
		return sortType_ = t;
	}

	// Specify the sort direction
	bool SetSortOrder(bool f)
	{
		return sortOrder_ = f;
	}

	// Set whether to handle hidden files
	bool SetShowHidden(bool f)
	{
		return showHidden_ = f;
	}

	void SortFiles(ItemList& files)
	{
		switch (sortType_) {
		case 0: files.Sort(CompByName(sortOrder_)); break;
		case 1: files.Sort(CompByType(sortOrder_)); break;
		case 2: files.Sort(CompByDate(sortOrder_)); break;
		case 3: files.Sort(CompBySize(sortOrder_)); break;
		}
	}

};
