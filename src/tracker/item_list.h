/**
 * Item list
 *
 * @author Takuto Yanagida
 * @version 2025-11-10
 */

#pragma once

#include <vector>
#include <memory>

#include "item.h"
#include "comparator.h"

class ItemList {

	std::vector<std::shared_ptr<Item>> its_;
	size_t sel_size_{};

public:

	ItemList() noexcept {}
	ItemList(const ItemList&) = delete;
	ItemList& operator=(const ItemList&) = delete;
	ItemList(ItemList&&) = delete;
	ItemList& operator=(ItemList&&) = delete;
	~ItemList() = default;

	size_t size() const noexcept {
		return its_.size();
	}

	std::shared_ptr<Item> at(size_t idx) {
		return its_.at(idx);
	}

	const std::shared_ptr<Item> at(size_t idx) const {
		return its_.at(idx);
	}

	void add(std::shared_ptr<Item> it) {
		its_.push_back(it);
	}

	void insert(size_t index, std::shared_ptr<Item> it) {
		its_.insert(its_.begin() + index, it);
	}

	void clear() {
		for (auto& it : its_) {
			Item::destroy(it);
		}
		its_.clear();
	}

	void sort(const int by, const bool reverse) {
		auto proj = [](auto const& sp) noexcept { return sp.get(); };
		switch (by) {
		case 0: std::ranges::sort(its_, CompByName(reverse), proj); break;
		case 1: std::ranges::sort(its_, CompByType(reverse), proj); break;
		case 2: std::ranges::sort(its_, CompByDate(reverse), proj); break;
		case 3: std::ranges::sort(its_, CompBySize(reverse), proj); break;
		default: break;
		}
	}

	size_t select(size_t front, size_t back, bool all) noexcept {
		if (back < front) std::swap(front, back);
		if (all) {
			for (size_t i = front; i <= back; ++i) {
				auto &it = its_.at(i);
				if (it->data() != 0) continue;
				it->set_sel(true);
			}
			sel_size_ = back - front + 1;
		} else {
			for (size_t i = front; i <= back; ++i) {
				auto& it = its_.at(i);
				if (it->data() != 0) continue;
				it->set_sel(!it->is_sel());
				sel_size_ += (it->is_sel() ? 1 : -1);
			}
		}
		return sel_size_;
	}

	void unselect() noexcept {
		for (auto& it : its_) {
			it->set_sel(false);
		}
		sel_size_ = 0;
	}

	size_t selected_size() const noexcept {
		return sel_size_;
	}

};
