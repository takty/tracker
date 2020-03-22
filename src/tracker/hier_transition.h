/**
 *
 * Transition of Folder Hierarchy
 *
 * @author Takuto Yanagida
 * @version 2020-03-22
 *
 */


#pragma once

#include <vector>


class HierTransition {

	struct ViewData {
		std::wstring path_;
		unsigned int index_;
		ViewData() : index_(0U) {}
		void set(const std::wstring& path, unsigned int index) { path_.assign(path), index_ = index; }
	};

	std::vector<ViewData> views_;
	unsigned int currentViewIndex_ = 0U;

public:

	HierTransition() {
		views_.resize(1);
	}

	unsigned int index() const {
		return views_[currentViewIndex_].index_;
	}

	void setIndex(unsigned int index) {
		views_[currentViewIndex_].index_ = index;
	}

	bool canGoBack() const {
		return currentViewIndex_ > 0;
	}

	wstring& goBack() {
		--currentViewIndex_;
		return views_[currentViewIndex_].path_;
	}

	void goForward(unsigned int index, const wstring& path) {
		if (canGoBack() && views_[currentViewIndex_ - 1].path_ == path) return;
		views_[currentViewIndex_].set(path, index);
		++currentViewIndex_;
		views_.resize(currentViewIndex_ + 1);
	}

	void clearIndexes() {
		for (auto i = 0U; i < views_.size(); ++i) {
			if (i != currentViewIndex_) views_[i].index_ = 0U;
		}
	}

};
