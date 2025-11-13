/**
 * Document
 *
 * @author Takuto Yanagida
 * @version 2025-11-13
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

	Observer* observer_;
	const TypeTable& exts_;
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
		const auto last = navis_.size();
		std::wstring cur(path);

		while (!cur.empty()) {
			const auto it = Item::create();
			it->set_file(cur, exts_);
			navis_.insert(last, it);
			cur = path::parent(cur);
		}
		const auto it = Item::create();
		it->data() = hierarchy_separator_option_data_;
		navis_.add(it);

		// Add files
		file_system::find_first_file(path, [&](const std::wstring& parent, const WIN32_FIND_DATA& wfd) {
			const bool isHidden = (wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0;
			if (!isHidden || opt_.is_show_hidden()) {
				const auto it = Item::create();
				it->set_file(parent, wfd, exts_);
				files_.add(it);
			}
			return true;  // continue
		});
		opt_.sort_files(files_);
	}

	// Make a file list
	void MakeFileList() {
		files_.clear();
		navis_.clear();

		const auto itFav = Item::create();
		const auto itHis = Item::create();
		const auto itDri = Item::create();
		itFav->set_special(fav_.PATH, fav_.NAME);
		itHis->set_special(his_.PATH, his_.NAME);
		itDri->set_special(dri_.PATH, dri_.NAME);
		navis_.add(itFav);
		navis_.add(itHis);
		navis_.add(itDri);

		const auto itSp = Item::create();
		itSp->data() = special_separator_option_data_;
		navis_.add(itSp);

		const ErrorMode em;
		if (currentPath_ == fav_.PATH) {
			for (size_t i = 0; i < fav_.size(); ++i) {
				const auto it = Item::create();
				it->set_file(fav_[i], exts_, i);
				files_.add(it);
			}
		} else if (currentPath_ == his_.PATH) {
			his_.clean_up();
			for (size_t i = 0; i < his_.size(); ++i) {
				const auto it = Item::create();
				it->set_file(his_[i], exts_, i);
				files_.add(it);
			}
			opt_.sort_history(files_);
		} else if (currentPath_ == dri_.PATH) {
			dri_.clean_up();
			for (size_t i = 0; i < dri_.size(); ++i) {
				const auto it = Item::create();
				it->set_file(dri_[i], exts_);
				files_.add(it);
			}
		} else {
			while (!currentPath_.empty()) {  // Go back to the folder where the file exists
				if (file_system::is_existing(currentPath_)) break;
				currentPath_ = path::parent(currentPath_);
			}
			if (!currentPath_.empty()) SetNormalFolder(currentPath_);  // Folder found
			else {
				dri_.clean_up();
				for (size_t i = 0; i < dri_.size(); ++i) {
					const auto it = Item::create();
					it->set_file(dri_[i], exts_);
					files_.add(it);
				}
			}
		}
		if (files_.size() == 0) {
			const auto it = Item::create();
			it->set_empty();
			files_.add(it);
		}
	}

public:

	Document(const TypeTable& exts, Pref& pref, int special_separator_option_data, int hierarchy_separator_option_data) : exts_(exts), pref_(pref), opt_(), observer_(), fav_(pref.path()), his_(pref.path()) {
		special_separator_option_data_   = special_separator_option_data;
		hierarchy_separator_option_data_ = hierarchy_separator_option_data;
		currentPath_                     = dri_.PATH;
	}

	void SetObserver(Observer* o) noexcept {
		observer_ = o;
	}

	void Initialize(bool firstTime = true) {
		his_.initialize(pref_);
		if (firstTime) {
			opt_.restore(pref_);
			fav_.restore(pref_);
			his_.restore(pref_);
		}
	}

	void Finalize() {
		fav_.store();
		his_.store();
		opt_.store(pref_);
	}

	void Update() {
		navis_.unselect();
		files_.unselect();
		MakeFileList();
		observer_->updated();
	}

	void SetCurrentDirectory(const std::wstring& path) {
		if (path != currentPath_) {
			lastCurrentPath_.assign(currentPath_);
			currentPath_.assign(path);
		}
		Update();
	}

	// Move to lower folder
	bool MoveToLower(ListType w, size_t index) {
		const std::shared_ptr<Item> it = GetItem(w, index);
		if (it == nullptr) return false;
		if (it->is_empty()) return false;

		if (it->is_dir()) {
			std::wstring path;
			if (it->path().front() == L':') {
				path.assign(it->path());
			} else if (it->is_link()) {  // Save current folder if folder shortcut
				path = link::resolve(it->path());
			} else {
				path.assign(it->path());
			}
			SetCurrentDirectory(path);
			return true;
		}
		if (link::is_link(it->path())) {  // If it is a shortcut
			auto path = link::resolve(it->path());
			path = path::parent(path);  // Get parent path
			SetCurrentDirectory(path);
			return true;
		}
		if (InBookmark() || InHistory()) {
			auto path = path::parent(it->path());
			SetCurrentDirectory(path);
			return true;
		}
		return false;
	}

	// Check if it is possible to move to lower folder
	bool MovableToLower(ListType w, size_t index) {
		const std::shared_ptr<Item> it = GetItem(w, index);

		if (it == nullptr) return false;
		if (it->is_empty()) return false;

		if (it->is_dir()) return true;
		if (link::is_link(it->path())) return true;
		if (InBookmark() || InHistory()) return true;

		return false;
	}

	// Set operators for multiple selected files
	Selection& SetOperator(std::optional<size_t> index, ListType type, Selection& ope) {
		ope.clear();
		if (!index) return ope;

		const auto& vec = (type == ListType::FILE) ? files_ : navis_;
		const auto& it  = vec.at(index.value());
		if (!it) return ope;

		if (it->is_empty()) return ope;

		// When index is not selected (including hierarchy) -> Single file is selected alone
		if (!it->is_sel()) {
			ope.add(it->path());
			return ope;
		}
		// Copy selected file name
		ope.add(it->path());  // Copy the file specified by index to the beginning
		for (size_t i = 0; i < vec.size(); i++) {
			if (vec.at(i)->is_sel() && i != index.value()) {
				ope.add(vec.at(i)->path());
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
	bool ArrangeFavorites(std::optional<size_t> drag, std::optional<size_t> drop) {
		if (!drag || !drop || drag == drop) return false;
		if (!InBookmark()) return false;
		return fav_.arrange(files_.at(drag.value())->id(), files_.at(drop.value())->id());
	}

	// Add to / Remove from Favorites
	void AddOrRemoveFavorite(const std::wstring& obj, ListType w, size_t index) {
		auto &vec = (w == ListType::FILE) ? files_ : navis_;

		if (vec.at(index)->is_empty()) return;
		if (InBookmark() && w == ListType::FILE) {
			fav_.remove(files_.at(index)->id());
		} else {
			fav_.add(obj);
		}
		Update();
	}

	// Select file by range specification
	void SelectFile(size_t front, size_t back, ListType type, bool all) noexcept {
		auto &vec = (type == ListType::FILE) ? files_ : navis_;
		vec.select(front, back, all);
	}

	Option& GetOpt() noexcept {
		return opt_;
	}

	// Return the number of selected files
	size_t SelectedCount() noexcept {
		return files_.selected_size();
	}

	// The current directory is a bookmark
	bool InBookmark() noexcept {
		return currentPath_ == fav_.PATH;
	}

	// Current directory is history
	bool InHistory() noexcept {
		return currentPath_ == his_.PATH;
	}

	// Current directory is drive
	bool InDrives() noexcept {
		return currentPath_ == dri_.PATH;
	}

	// Return the current path
	std::wstring& CurrentPath() noexcept {
		return currentPath_;
	}

	// Return the previous current path
	std::wstring& LastCurrentPath() noexcept {
		return lastCurrentPath_;
	}

	// Return file information
	std::shared_ptr<Item> GetItem(ListType w, size_t index) {
		return (w == ListType::FILE) ? files_.at(index) : navis_.at(index);
	}

	const ItemList& GetNavis() const noexcept {
		return navis_;
	}

	const ItemList& GetFiles() const noexcept {
		return files_;
	}

	const size_t GetNaviCount() const noexcept {
		return navis_.size();
	}

	const size_t GetFileCount() const noexcept {
		return files_.size();
	}

	const bool IsFileEmpty() const {
		return files_.size() && files_.at(0)->is_empty();
	}

};
