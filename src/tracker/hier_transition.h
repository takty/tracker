/**
 *
 * Transition of Folder Hierarchy
 *
 * @author Takuto Yanagida
 * @version 2021-05-08
 *
 */


#pragma once

#include <vector>


class HierTransition {

	struct ViewData {
		std::wstring path_;
		unsigned int index_;
		ViewData() noexcept : index_(0U) {}
		void set(const std::wstring& path, unsigned int index) { path_.assign(path), index_ = index; }
	};

	std::vector<ViewData> views_;
	unsigned int currentViewIndex_{};

public:

	HierTransition() noexcept(false) {
		views_.resize(1);
	}

	unsigned int index() const noexcept(false) {
		return views_.at(currentViewIndex_).index_;
	}

	void setIndex(unsigned int index) noexcept(false) {
		views_.at(currentViewIndex_).index_ = index;
	}

	bool canGoBack() const noexcept {
		return currentViewIndex_ > 0;
	}

	std::wstring& goBack() noexcept(false) {
		--currentViewIndex_;
		return views_.at(currentViewIndex_).path_;
	}

	void goForward(unsigned int index, const std::wstring& path) {
		if (canGoBack() && views_.at(currentViewIndex_ - 1).path_ == path) return;
		views_.at(currentViewIndex_).set(path, index);
		++currentViewIndex_;
		views_.resize(currentViewIndex_ + 1);
	}

	void clearIndexes() noexcept(false) {
		for (auto i = 0U; i < views_.size(); ++i) {
			if (i != currentViewIndex_) views_.at(i).index_ = 0U;
		}
	}

};
