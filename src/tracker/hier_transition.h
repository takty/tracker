/**
 *
 * Transition of Folder Hierarchy
 *
 * @author Takuto Yanagida
 * @version 2021-05-30
 *
 */


#pragma once

#include <vector>


class HierTransition {

	struct ViewData {
		std::wstring path_;
		size_t index_{};
		ViewData() noexcept {}
		void set(const std::wstring& path, const size_t index) {
			path_.assign(path);
			index_ = index;
		}
	};

	std::vector<ViewData> views_;
	size_t cur_view_idx_{};

public:

	HierTransition() noexcept {
		views_.resize(1);
	}

	unsigned int index() const noexcept(false) {
		return views_.at(cur_view_idx_).index_;
	}

	void set_index(const size_t index) noexcept(false) {
		views_.at(cur_view_idx_).index_ = index;
	}

	bool can_go_back() const noexcept {
		return cur_view_idx_ > 0;
	}

	std::wstring& go_back() noexcept(false) {
		--cur_view_idx_;
		return views_.at(cur_view_idx_).path_;
	}

	void go_forward(const size_t index, const std::wstring& path) {
		if (can_go_back() && views_.at(cur_view_idx_ - 1).path_ == path) return;
		views_.at(cur_view_idx_).set(path, index);
		++cur_view_idx_;
		views_.resize(cur_view_idx_ + 1);
	}

	void clear_indexes() noexcept(false) {
		for (auto i = 0U; i < views_.size(); ++i) {
			if (i != cur_view_idx_) views_.at(i).index_ = 0U;
		}
	}

};
