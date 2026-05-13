/**
 * Document Options
 *
 * @author Takuto Yanagida
 * @version 2026-05-13
 */

#pragma once

#include <vector>

#include "pref.hpp"
#include "item_list.h"

class Option {

	bool show_hidden_;
	bool sort_rev_;
	int sort_by_;

	bool dot_file_as_hidden_;
	bool sort_his_;
	bool sort_his_rev_;
	int sort_his_by_;

public:

	enum { sbName, sbType, sbDate, sbSize };  // Sort type

	void restore(Pref& pref) noexcept {
		pref.set_current_section(SECTION_WINDOW);
		show_hidden_ = pref.item_int(KEY_SHOW_HIDDEN, VAL_SHOW_HIDDEN) != 0;
		sort_rev_    = pref.item_int(KEY_SORT_REV,    VAL_SORT_REV)    != 0;
		sort_by_     = pref.item_int(KEY_SORT_BY,     VAL_SORT_BY);

		dot_file_as_hidden_ = pref.item_int(KEY_DOT_FILE_AS_HIDDEN, VAL_DOT_FILE_AS_HIDDEN) != 0;
		sort_his_           = pref.item_int(KEY_SORT_HISTORY,       VAL_SORT_HISTORY)       != 0;
		sort_his_rev_       = pref.item_int(KEY_SORT_HISTORY_REV,   VAL_SORT_HISTORY_REV)   != 0;
		sort_his_by_        = pref.item_int(KEY_SORT_HISTORY_BY,    VAL_SORT_HISTORY_BY);
	}

	void store(Pref& pref) const noexcept {
		pref.set_item_int(SECTION_WINDOW, KEY_SHOW_HIDDEN, show_hidden_);
		pref.set_item_int(SECTION_WINDOW, KEY_SORT_REV,    sort_rev_);
		pref.set_item_int(SECTION_WINDOW, KEY_SORT_BY,     sort_by_);
	}

	int get_sort_type() const noexcept {
		return sort_by_;
	}

	TCHAR get_sort_type_char() const noexcept {
		switch (sort_by_) {
		case sbName: return sort_rev_ ? 'N' : 'n';
		case sbType: return sort_rev_ ? 'T' : 't';
		case sbDate: return sort_rev_ ? 'D' : 'd';
		case sbSize: return sort_rev_ ? 'S' : 's';
		default:     return sort_rev_ ? 'N' : 'n';
		}
	}

	bool get_sort_order() const noexcept {
		return sort_rev_;
	}

	bool is_show_hidden() const noexcept {
		return show_hidden_;
	}

	bool is_dot_file_as_hidden() const noexcept {
		return dot_file_as_hidden_;
	}

	int set_sort_type(int t) noexcept {
		return sort_by_ = t;
	}

	bool set_sort_order(bool f) noexcept {
		return sort_rev_ = f;
	}

	bool set_show_hidden(bool f) noexcept {
		return show_hidden_ = f;
	}

	void sort_files(ItemList& il) const {
		il.sort(sort_by_, sort_rev_);
	}

	void sort_history(ItemList& il) const {
		if (!sort_his_) return;
		il.sort(sort_his_by_, sort_his_rev_);
	}

};
