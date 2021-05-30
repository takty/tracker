/**
 *
 * Item list
 *
 * @author Takuto Yanagida
 * @version 2021-05-30
 *
 */


#pragma once

#include <vector>
#include <algorithm>

#include "item.h"


class ItemList {

	std::vector<Item*> buf_;
	std::vector<Item*> items_;
	size_t sel_num_{};

	void delete_item(Item *f) {
		if (buf_.size() >= 1024) {
			delete f;
		}
		else {
			buf_.push_back(f);
		}
	}

public:

	ItemList() noexcept {}

	~ItemList() {
		for (auto& it : items_) delete it;
		for (auto& it : buf_) delete it;
	}

	auto size() const noexcept {
		return items_.size();
	}

	auto at(size_t index) noexcept {
		return items_.at(index);
	}

	const auto at(size_t index) const noexcept {
		return items_.at(index);
	}

	auto begin() noexcept {
		return items_.begin();
	}

	auto end() noexcept {
		return items_.end();
	}

	// ---------------------------------------------------------------------------

	Item* create_item() {
		Item* f = nullptr;
		if (buf_.empty()) {
			f = new Item();
		} else {
			f = buf_.back();
			buf_.pop_back();
		}
		if (!f) return nullptr;
		f->clear();
		return f;
	}

	void add(Item* item) {
		items_.push_back(item);
	}

	void insert(size_t index, Item* item) {
		items_.insert(items_.begin() + index, item);
	}

	void clear() {
		for (auto& i : items_) delete_item(i);
		items_.clear();
	}

	template<class Pred> void sort(Pred p) {
		std::sort(items_.begin(), items_.end(), p);
	}

	size_t select(size_t front, size_t back, bool all) noexcept(false) {
		if (front == -1 || back == -1) return sel_num_;
		if (back < front) std::swap(front, back);
		if (all) {
			for (size_t i = front; i <= back; ++i) {
				if (items_.at(i)->data() != 0) continue;
				items_.at(i)->set_selected(true);
			}
			sel_num_ = back - front + 1;
		} else {
			for (size_t i = front; i <= back; ++i) {
				auto& fd = items_.at(i);
				if (fd->data() != 0) continue;
				fd->set_selected(!fd->is_selected());
				sel_num_ += (fd->is_selected() ? 1 : -1);
			}
		}
		return sel_num_;
	}

	void clear_selection() noexcept {
		sel_num_ = 0U;
	}

	size_t selected_size() noexcept {
		return sel_num_;
	}

};
