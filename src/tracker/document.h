/**
 *
 * Document
 *
 * @author Takuto Yanagida
 * @version 2021-05-30
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

	enum class ListType { FILE, NAVI = 64 };

private:

	Observer* view_;
	const TypeTable& extentions_;
	Pref & pref_;
	Pref & bookmark_data_;
	Pref & history_data_;

	Bookmark fav_;
	History  his_;
	Drives   dri_;

	std::wstring cur_path_, last_cur_path_;
	ItemList files_, navis_;
	Option opt_;

	int special_separator_option_data_;
	int hierarchy_separator_option_data_;

	// Make a file list of ordinary folders
	void set_normal_folder(const std::wstring& path) noexcept {
		// Create hierarchical list
		const auto last = navis_.size();
		std::wstring cur(path);

		while (!cur.empty()) {
			navis_.insert(last, navis_.create_item()->set_file_item(cur, extentions_));
			cur = Path::parent(cur);
		}
		auto* item = navis_.create_item();;
		if (!item) return;
		item->data() = hierarchy_separator_option_data_;
		navis_.add(item);

		// Add files
		FileSystem::find_first_file(path, [&](const std::wstring& parent, const WIN32_FIND_DATA& wfd) {
			const bool isHidden = (wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0;
			if (!isHidden || opt_.is_hidden_shown()) {
				files_.add(files_.create_item()->set_file_item(parent, wfd, extentions_));
			}
			return true;  // continue
		});
		opt_.sort_files(files_);
	}

	// Make a file list
	void make_file_list() noexcept {
		files_.clear();
		navis_.clear();

		const std::array<const std::wstring, 3> path = { fav_.PATH, his_.PATH, dri_.PATH };
		const std::array<const std::wstring, 3> name = { fav_.NAME, his_.NAME, dri_.NAME };

		for (int i = 0; i < 3; ++i) navis_.add(navis_.create_item()->set_special_folder_item(path.at(i), name.at(i)));
		auto* item = navis_.create_item();
		if (!item) return;
		item->data() = special_separator_option_data_;
		navis_.add(item);

		const ErrorMode em;
		if (cur_path_ == fav_.PATH) {
			for (auto i = 0U; i < fav_.size(); ++i) files_.add(files_.create_item()->set_file_item(fav_.at(i), extentions_, i));
		} else if (cur_path_ == his_.PATH) {
			his_.clean_up();
			for (auto i = 0U; i < his_.size(); ++i) files_.add(files_.create_item()->set_file_item(his_.at(i), extentions_, i));
			opt_.sort_history(files_);
		} else if (cur_path_ == dri_.PATH) {
			dri_.clean_up();
			for (auto i = 0U; i < dri_.size(); ++i) files_.add(files_.create_item()->set_file_item(dri_.at(i), extentions_));
		} else {
			while (!cur_path_.empty()) {  // Go back to the folder where the file exists
				if (FileSystem::exists(cur_path_)) break;
				cur_path_ = Path::parent(cur_path_);
			}
			if (!cur_path_.empty()) set_normal_folder(cur_path_);  // Folder found
			else {
				dri_.clean_up();
				for (auto i = 0U; i < dri_.size(); ++i) files_.add(files_.create_item()->set_file_item(dri_.at(i), extentions_));
			}
		}
		if (files_.size() == 0U) files_.add(files_.create_item()->set_empty_item());
	}

public:

	Document(const TypeTable& exts, Pref& pref, Pref& bookmark_data, Pref& history_data, int special_separator_option_data, int hierarchy_separator_option_data) noexcept
		: extentions_(exts), pref_(pref), bookmark_data_(bookmark_data), history_data_(history_data)
	{
		special_separator_option_data_ = special_separator_option_data;
		hierarchy_separator_option_data_ = hierarchy_separator_option_data;
		cur_path_ = dri_.PATH;
	}

	void set_view(Observer* view) noexcept {
		view_ = view;
	}

	void initialize(bool is_first = true) noexcept {
		his_.initialize(pref_);
		if (is_first) {
			opt_.restore(pref_);
			fav_.restore(bookmark_data_);
			his_.restore(history_data_);
		}
	}

	void finalize() noexcept {
		fav_.store(bookmark_data_);
		his_.store(history_data_);
		opt_.store(pref_);
	}

	void update() noexcept {
		navis_.clear_selection();
		files_.clear_selection();
		make_file_list();
		view_->updated();
	}


	// ------------------------------------------------------------------------


	void set_current_directory(const std::wstring& path) noexcept {
		if (path != cur_path_) {
			last_cur_path_.assign(cur_path_);
			cur_path_.assign(path);
		}
		update();
	}

	// Move to lower folder
	bool move_to_lower(ListType lt, size_t index) noexcept {
		const Item* it = ((lt == ListType::FILE) ? files_ : navis_).at(index);
		if (it->is_empty()) return false;

		if (it->is_dir()) {
			std::wstring path;
			if (it->path().at(0) == L':') {
				path.assign(it->path());
			} else if (it->is_link()) {  // Save current folder if folder shortcut
				path = Link::resolve(it->path());
			} else {
				path.assign(it->path());
			}
			set_current_directory(path);
			return true;
		}
		if (Link::is_link(it->path())) {  // If it is a shortcut
			auto path = Link::resolve(it->path());
			path = Path::parent(path);  // Get parent path
			set_current_directory(path);
			return true;
		}
		if (in_bookmark() || in_history()) {
			auto path = Path::parent(it->path());
			set_current_directory(path);
			return true;
		}
		return false;
	}

	// Check if it is possible to move to lower folder
	bool is_movable_to_lower(ListType lt, size_t index) noexcept {
		const Item* it = ((lt == ListType::FILE) ? files_ : navis_).at(index);

		if (it->is_empty())                return false;
		if (it->is_dir())                  return true;
		if (Link::is_link(it->path()))     return true;
		if (in_bookmark() || in_history()) return true;
		return false;
	}

	// Set operators for multiple selected files
	Selection& set_operator(const size_t index, ListType lt, Selection& ope) noexcept {
		auto &list = (lt == ListType::FILE) ? files_ : navis_;
		if (list.at(index)->is_empty()) return ope;

		// When index is not selected (including hierarchy) -> Single file is selected alone
		if (!list.at(index)->is_selected()) {
			const auto& it = list.at(index);
			if (!it->is_special()) {
				ope.add(it->path());
			}
			return ope;
		}
		// Copy selected file name
		ope.add(list.at(index)->path());  // Copy the file specified by index to the beginning
		for (auto i = 0u; i < list.size(); ++i) {
			if (list.at(i)->is_selected() && i != index) {
				ope.add(list.at(i)->path());
			}
		}
		return ope;
	}

	// Add to history
	void set_history(const std::wstring& path) noexcept {
		his_.add(path);
	}

	// Delete history
	void clear_history() noexcept {
		his_.clear();
		update();
	}

	// Sort bookmarks
	bool arrange_bookmarks(const size_t drag, const size_t drop) noexcept {
		if (drag == drop) return false;
		if (!in_bookmark()) return false;
		return fav_.arrange(files_.at(drag)->id(), files_.at(drop)->id());
	}

	// Add to / Remove from bookmarks
	void add_or_remove_bookmark(const std::wstring& obj, ListType lt, const size_t index) noexcept {
		auto &list = (lt == ListType::FILE) ? files_ : navis_;

		if (list.at(index)->is_empty()) return;
		if (in_bookmark()) {
			fav_.remove(files_.at(index)->id());
		} else {
			fav_.add(obj);
		}
		update();
	}

	// Select file by range specification
	void select_file(const size_t front, const size_t back, ListType lt, bool all) noexcept {
		auto &list = (lt == ListType::FILE) ? files_ : navis_;
		list.select(front, back, all);
	}

	Option& get_option() noexcept {
		return opt_;
	}

	const Option& get_option() const noexcept {
		return opt_;
	}

	// Return the number of selected files
	size_t selected_size() const noexcept {
		return files_.selected_size();
	}

	// The current directory is a bookmark
	bool in_bookmark() const noexcept {
		return cur_path_ == fav_.PATH;
	}

	// Current directory is history
	bool in_history() const noexcept {
		return cur_path_ == his_.PATH;
	}

	// Current directory is drive
	bool in_drives() const noexcept {
		return cur_path_ == dri_.PATH;
	}

	// Return the current path
	std::wstring& current_path() noexcept {
		return cur_path_;
	}

	// Return the previous current path
	std::wstring& last_current_path() noexcept {
		return last_cur_path_;
	}

	// Return file information
	Item* get_item(ListType lt, size_t index) noexcept {
		return (lt == ListType::FILE) ? files_.at(index) : navis_.at(index);
	}

	const ItemList& get_navi_list() const noexcept {
		return navis_;
	}

	const ItemList& get_file_list() const noexcept {
		return files_;
	}

};
