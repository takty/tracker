/**
 *
 * Document
 *
 * @author Takuto Yanagida
 * @version 2025-10-21
 *
 */


#pragma once

#include <string>

#include <windows.h>

#include "file_utils.hpp"
#include "pref.hpp"
#include "selection.h"
#include "item_list.h"
#include "item.h"
#include "type_table.h"
#include "observer.h"

#include "bookmark.h"
#include "history.h"
#include "drives.h"
#include "option.h"


class Item;


class Document {

public:

	enum class ListType { FILE, HIER = 64 };

private:

	Observer* view_;
	const TypeTable& extentions_;
	Pref& pref_;

	Bookmark fav_;
	History  his_;
	Drives   dri_;

	std::wstring currentPath_, lastCurrentPath_;
	ItemList files_, navis_;
	Option opt_;

	int special_separator_option_data_;
	int hierarchy_separator_option_data_;

	// Make a file list of ordinary folders
	void SetNormalFolder(const std::wstring& path) {
		// Create hierarchical list
		auto last = navis_.Count();
		std::wstring cur(path);

		while (!cur.empty()) {
			navis_.Insert(last, navis_.CreateItem()->SetFileItem(cur, extentions_));
			cur = Path::parent(cur);
		}
		auto* item = navis_.CreateItem();// ->SetSeparatorItem(true);
		item->data() = hierarchy_separator_option_data_;
		navis_.Add(item);

		// Add files
		FileSystem::find_first_file(path, [&](const std::wstring& parent, const WIN32_FIND_DATA& wfd) {
			bool isHidden = (wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0;
			if (!isHidden || opt_.IsShowHidden()) {
				files_.Add(files_.CreateItem()->SetFileItem(parent, wfd, extentions_));
			}
			return true;  // continue
		});
		opt_.SortFiles(files_);
	}

	// Make a file list
	void MakeFileList() {
		files_.Clear();
		navis_.Clear();

		const std::wstring path[3] = { fav_.PATH, his_.PATH, dri_.PATH };
		const std::wstring name[3] = { fav_.NAME, his_.NAME, dri_.NAME };

		for (int i = 0; i < 3; ++i) navis_.Add(navis_.CreateItem()->SetSpecialFolderItem(path[i], name[i]));
		auto* item = navis_.CreateItem();// ->SetSeparatorItem(false);
		item->data() = special_separator_option_data_;
		navis_.Add(item);

		ErrorMode em;
		if (currentPath_ == fav_.PATH) {
			for (size_t i = 0; i < fav_.size(); ++i) files_.Add(files_.CreateItem()->SetFileItem(fav_[i], extentions_, i));
		} else if (currentPath_ == his_.PATH) {
			his_.clean_up();
			for (size_t i = 0; i < his_.size(); ++i) files_.Add(files_.CreateItem()->SetFileItem(his_[i], extentions_, i));
			opt_.SortHistory(files_);
		} else if (currentPath_ == dri_.PATH) {
			dri_.clean_up();
			for (size_t i = 0; i < dri_.size(); ++i) files_.Add(files_.CreateItem()->SetFileItem(dri_[i], extentions_));
		} else {
			while (!currentPath_.empty()) {  // Go back to the folder where the file exists
				if (FileSystem::is_existing(currentPath_)) break;
				currentPath_ = Path::parent(currentPath_);
			}
			if (!currentPath_.empty()) SetNormalFolder(currentPath_);  // Folder found
			else {
				dri_.clean_up();
				for (size_t i = 0; i < dri_.size(); ++i) files_.Add(files_.CreateItem()->SetFileItem(dri_[i], extentions_));
			}
		}
		if (files_.Count() == 0) files_.Add(files_.CreateItem()->SetEmptyItem());
	}

public:

	Document(const TypeTable& exts, Pref& pref, int special_separator_option_data, int hierarchy_separator_option_data) : extentions_(exts), pref_(pref) {
		special_separator_option_data_ = special_separator_option_data;
		hierarchy_separator_option_data_ = hierarchy_separator_option_data;
		currentPath_ = dri_.PATH;
	}

	void SetView(Observer* view) {
		view_ = view;
	}

	void Initialize(bool firstTime = true) {
		his_.initialize(pref_);
		if (firstTime) {
			opt_.Restore(pref_);
			fav_.restore(pref_);
			his_.restore(pref_);
		}
	}

	void Finalize() {
		fav_.store(pref_);
		his_.store(pref_);
		opt_.Store(pref_);
	}

	void Update() {
		navis_.ClearSelection();
		files_.ClearSelection();
		MakeFileList();
		view_->Updated();
	}

	void SetCurrentDirectory(const std::wstring& path) {
		if (path != currentPath_) {
			lastCurrentPath_.assign(currentPath_);
			currentPath_.assign(path);
		}
		Update();
	}

	// Move to lower folder
	bool MoveToLower(ListType w, int index) {
		Item *f = ((w == ListType::FILE) ? files_ : navis_)[index];
		if (f->IsEmpty()) return false;

		if (f->IsDir()) {
			std::wstring path;
			if (f->Path()[0] == L':') {
				path.assign(f->Path());
			} else if (f->IsLink()) {  // Save current folder if folder shortcut
				path = Link::resolve(f->Path());
			} else {
				path.assign(f->Path());
			}
			SetCurrentDirectory(path);
			return true;
		}
		if (Link::is_link(f->Path())) {  // If it is a shortcut
			auto path = Link::resolve(f->Path());
			path = Path::parent(path);  // Get parent path
			SetCurrentDirectory(path);
			return true;
		}
		if (InBookmark() || InHistory()) {
			auto path = Path::parent(f->Path());
			SetCurrentDirectory(path);
			return true;
		}
		return false;
	}

	// Check if it is possible to move to lower folder
	bool MovableToLower(ListType w, int index) {
		Item *f = ((w == ListType::FILE) ? files_ : navis_)[index];

		if (f->IsEmpty()) return false;
		if (f->IsDir()) return true;
		if (Link::is_link(f->Path())) return true;
		if (InBookmark() || InHistory()) return true;
		return false;
	}

	// Set operators for multiple selected files
	Selection& SetOperator(int index, ListType type, Selection& ope) {
		ope.Clear();
		if (index == -1) return ope;

		auto &vec = (type == ListType::FILE) ? files_ : navis_;
		if (vec[index]->IsEmpty()) return ope;

		// When index is not selected (including hierarchy) -> Single file is selected alone
		if (!vec[index]->IsSelected()) {
			ope.Add(vec[index]->Path());
			return ope;
		}
		// Copy selected file name
		ope.Add(vec[index]->Path());  // Copy the file specified by index to the beginning
		for (size_t i = 0; i < vec.Count(); i++) {
			if (vec[i]->IsSelected() && i != index) {
				ope.Add(vec[i]->Path());
			}
		}
		return ope;
	}

	// Add to history
	void SetHistory(const std::wstring& path) {
		his_.add(path);
	}

	// Delete history
	void ClearHistory() {
		his_.clear();
		Update();
	}

	// Sort favorite
	bool ArrangeFavorites(int drag, int drop) {
		if (drag == -1 || drop == -1 || drag == drop) return false;
		if (!InBookmark()) return false;
		return fav_.arrange(files_[drag]->Id(), files_[drop]->Id());
	}

	// Add to / Remove from Favorites
	void AddOrRemoveFavorite(const std::wstring& obj, ListType w, int index) {
		auto &vec = (w == ListType::FILE) ? files_ : navis_;

		if (vec[index]->IsEmpty()) return;
		if (InBookmark()) {
			fav_.remove(files_[index]->Id());
		} else {
			fav_.add(obj);
		}
		Update();
	}

	// Select file by range specification
	void SelectFile(int front, int back, ListType type, bool all) {
		auto &vec = (type == ListType::FILE) ? files_ : navis_;
		vec.Select(front, back, all);
	}

	Option& GetOpt() {
		return opt_;
	}

	// Return the number of selected files
	size_t SelectedCount() {
		return files_.SelectionCount();
	}

	// The current directory is a bookmark
	bool InBookmark() {
		return currentPath_ == fav_.PATH;
	}

	// Current directory is history
	bool InHistory() {
		return currentPath_ == his_.PATH;
	}

	// Current directory is drive
	bool InDrives() {
		return currentPath_ == dri_.PATH;
	}

	// Return the current path
	std::wstring& CurrentPath() {
		return currentPath_;
	}

	// Return the previous current path
	std::wstring& LastCurrentPath() {
		return lastCurrentPath_;
	}

	// Return file information
	Item* GetItem(ListType w, int index) {
		return (w == ListType::FILE) ? files_[index] : navis_[index];
	}

	const ItemList& GetNavis() const {
		return navis_;
	}

	const ItemList& GetFiles() const {
		return files_;
	}

};
