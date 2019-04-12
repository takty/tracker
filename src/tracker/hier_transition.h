//
// Transition of Folder Hierarchy
// 2019/04/12
//

#pragma once

#include <vector>

using namespace std;

class HierTransition {

	struct ViewData {
		wstring path_;
		unsigned int index_;
		ViewData() : index_(0U) {}
		void set(const wstring& path, unsigned int index) {path_.assign(path), index_ = index;}
	};

	std::vector<ViewData> views_;
	unsigned int currentViewIndex_;

public:

	HierTransition() {
		views_.resize(1);
		currentViewIndex_ = 0U;
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

	// Restore
	wstring& goBack() {
		--currentViewIndex_;
		return views_[currentViewIndex_].path_;
	}

	// Save
	void goForward(unsigned int index, const wstring& path) {
		if(canGoBack() && views_[currentViewIndex_ - 1].path_ == path) return;
		views_[currentViewIndex_].set(path, index);
		++currentViewIndex_;
		views_.resize(currentViewIndex_ + 1);
	}

	void clearIndexes() {
		for(auto i = 0U; i < views_.size(); ++i) {
			if(i != currentViewIndex_) views_[i].index_ = 0U;
		}
	}

};
