/**
 *
 * Transition of Folder Hierarchy
 *
 * @author Takuto Yanagida
 * @version 2025-10-24
 *
 */


#pragma once

#include <vector>


class HierTransition {

	struct ViewData {
		std::wstring path_;
		size_t index_;
		ViewData() noexcept : index_(0U) {}
		void set(const std::wstring& path, size_t index) { path_.assign(path), index_ = index; }
	};

	std::vector<ViewData> views_;
	size_t currentViewIndex_ = 0U;

public:

	HierTransition() noexcept {
		views_.resize(1);
	}

	size_t index() const noexcept {
		return views_.at(currentViewIndex_).index_;
	}

	void setIndex(size_t index) noexcept {
		views_.at(currentViewIndex_).index_ = index;
	}

	bool canGoBack() const noexcept {
		return currentViewIndex_ > 0;
	}

	std::wstring& goBack() noexcept {
		--currentViewIndex_;
		return views_.at(currentViewIndex_).path_;
	}

	void goForward(size_t index, const std::wstring& path) {
		if (canGoBack() && views_.at(currentViewIndex_ - 1).path_ == path) {
			return;
		}
		views_.at(currentViewIndex_).set(path, index);
		++currentViewIndex_;
		views_.resize(currentViewIndex_ + 1);
	}

	void clearIndexes() noexcept {
		for (auto i = 0U; i < views_.size(); ++i) {
			if (i != currentViewIndex_) {
				views_.at(i).index_ = 0U;
			}
		}
	}

};
