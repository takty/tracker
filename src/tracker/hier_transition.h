/**
 * Transition of Folder Hierarchy
 *
 * @author Takuto Yanagida
 * @version 2025-11-19
 */

#pragma once

#include <vector>

class HierTransition {

	struct ViewData {
		std::wstring path_;
		size_t index_;
		ViewData() noexcept : index_(0U) {}
		void set(const std::wstring& path, size_t index) { 
			path_.assign(path);
			index_ = index;
		}
	};

	std::vector<ViewData> views_;
	size_t cur_view_idx_ = 0U;

public:

	HierTransition() noexcept {
		views_.resize(1);
	}

	size_t index() const noexcept {
		return views_.at(cur_view_idx_).index_;
	}

	void set_index(size_t idx) noexcept {
		views_.at(cur_view_idx_).index_ = idx;
	}

	bool can_go_back() const noexcept {
		return cur_view_idx_ > 0;
	}

	std::wstring& go_back() noexcept {
		--cur_view_idx_;
		return views_.at(cur_view_idx_).path_;
	}

	void go_forward(size_t idx, const std::wstring& path) {
		if (can_go_back() && views_.at(cur_view_idx_ - 1).path_ == path) {
			return;
		}
		views_.at(cur_view_idx_).set(path, idx);
		++cur_view_idx_;
		views_.resize(cur_view_idx_ + 1);
	}

	void clear_indexes() noexcept {
		for (auto i = 0U; i < views_.size(); ++i) {
			if (i != cur_view_idx_) {
				views_.at(i).index_ = 0U;
			}
		}
	}

};
