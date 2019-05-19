#pragma once

#include <vector>

#include "Item.h"


//
// Item list
// 2016/02/22
//

class ItemList {

	std::vector<Item*> buf_;
	std::vector<Item*> items_;
	int selNum_;

	void DeleteItem(Item *f) {
		if (buf_.size() >= 1024) delete f;
		else buf_.push_back(f);
	}

public:

	ItemList() : selNum_(0) {}

	~ItemList() {
		for (auto& i : items_) delete i;
		for (auto& i : buf_) delete i;
	}

	int Count() const {
		return items_.size();
	}

	Item* operator[](int index) {
		return items_[index];
	}

	const Item* operator[](int index) const {
		return items_[index];
	}

	Item* CreateItem() {
		Item* f;
		if (buf_.empty()) {
			f = new Item();
		} else {
			f = buf_.back();
			buf_.pop_back();
		}
		f->Clear();
		return f;
	}

	void Add(Item* item) {
		items_.push_back(item);
	}

	void Insert(int index, Item* item) {
		items_.insert(items_.begin() + index, item);
	}

	void Clear() {
		for (auto& i : items_) DeleteItem(i);
		items_.clear();
	}

	template<class Pred> void Sort(Pred p) {
		sort(items_.begin(), items_.end(), p);
	}

	int Select(int front, int back, bool all) {
		if (front == -1 || back == -1) return selNum_;
		if (back < front) std::swap(front, back);
		if (all) {
			for (int i = front; i <= back; ++i) {
				if (items_[i]->data() != 0) continue;
				items_[i]->SetSelected(true);
			}
			selNum_ = back - front + 1;
		} else {
			for (int i = front; i <= back; ++i) {
				auto& fd = items_[i];
				if (fd->data() != 0) continue;
				fd->SetSelected(!fd->IsSelected());
				selNum_ += (fd->IsSelected() ? 1 : -1);
			}
		}
		return selNum_;
	}

	void ClearSelection() {
		selNum_ = 0;
	}

	int SelectionCount() {
		return selNum_;
	}

};
