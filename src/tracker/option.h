/**
 *
 * Document Options
 *
 * @author Takuto Yanagida
 * @version 2021-05-30
 *
 */


#pragma once

#include <vector>

#include "comparator.h"
#include "pref.hpp"
#include "item_list.h"


class Option {

	bool show_hidden_, sort_rev_;
	int sort_by_;

	bool sort_his_, sort_his_rev_;
	int sort_his_by_;

public:

	enum { ST_NAME, ST_TYPE, ST_DATE, ST_SIZE };  // Sort type

	void restore(const Pref& pref) {
		show_hidden_ = pref.get(SECTION_WINDOW, KEY_SHOW_HIDDEN, VAL_SHOW_HIDDEN) != 0;
		sort_rev_    = pref.get(SECTION_WINDOW, KEY_SORT_REV,    VAL_SORT_REV)    != 0;
		sort_by_     = pref.get(SECTION_WINDOW, KEY_SORT_BY,     VAL_SORT_BY);

		sort_his_     = pref.get(SECTION_WINDOW, KEY_SORT_HISTORY,     VAL_SORT_HISTORY)     != 0;
		sort_his_rev_ = pref.get(SECTION_WINDOW, KEY_SORT_HISTORY_REV, VAL_SORT_HISTORY_REV) != 0;
		sort_his_by_  = pref.get(SECTION_WINDOW, KEY_SORT_HISTORY_BY,  VAL_SORT_HISTORY_BY);
	}

	void store(Pref& pref) {
		pref.set(SECTION_WINDOW, KEY_SHOW_HIDDEN, show_hidden_);
		pref.set(SECTION_WINDOW, KEY_SORT_REV,    sort_rev_);
		pref.set(SECTION_WINDOW, KEY_SORT_BY,     sort_by_);

		pref.set(SECTION_WINDOW, KEY_SORT_HISTORY,     sort_his_);
		pref.set(SECTION_WINDOW, KEY_SORT_HISTORY_REV, sort_his_rev_);
		pref.set(SECTION_WINDOW, KEY_SORT_HISTORY_BY,  sort_his_by_);
	}

	int get_sort_type() const noexcept {
		return sort_by_;
	}

	bool get_sort_order() const noexcept {
		return sort_rev_;
	}

	bool is_hidden_shown() const noexcept {
		return show_hidden_;
	}

	int set_sort_type(int t) noexcept {
		return sort_by_ = t;
	}

	bool set_sort_order(bool f)noexcept {
		return sort_rev_ = f;
	}

	bool set_hidden_shown(bool f) noexcept {
		return show_hidden_ = f;
	}

	void sort_files(ItemList& files) const {
		switch (sort_by_) {
		case 0: files.sort(CompByName(sort_rev_)); break;
		case 1: files.sort(CompByType(sort_rev_)); break;
		case 2: files.sort(CompByDate(sort_rev_)); break;
		case 3: files.sort(CompBySize(sort_rev_)); break;
		default: files.sort(CompByName(sort_rev_)); break;
		}
	}

	void sort_history(ItemList& files) const {
		if (!sort_his_) return;
		switch (sort_his_by_) {
		case 0: files.sort(CompByName(sort_his_rev_)); break;
		case 1: files.sort(CompByType(sort_his_rev_)); break;
		case 2: files.sort(CompByDate(sort_his_rev_)); break;
		case 3: files.sort(CompBySize(sort_his_rev_)); break;
		default: files.sort(CompByName(sort_his_rev_)); break;
		}
	}

};
