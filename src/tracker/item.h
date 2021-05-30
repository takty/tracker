/**
 *
 * File Item
 *
 * @author Takuto Yanagida
 * @version 2021-05-30
 *
 */


#pragma once

#include <string>
#include <windows.h>

#include "file_utils.hpp"
#include "document.h"
#include "type_table.h"


class Item {

	static std::wstring& EMPTY_STR() {
		static std::wstring EMPTY_STR(L"empty");
		return EMPTY_STR;
	}

	enum { DIR = 2, HIDE = 4, LINK = 8, HIER = 16, SEL = 32, EMPTY = 64 };

	std::wstring path_, name_;
	FILETIME date_;
	unsigned long long size_ = 0;
	int color_ = 0;
	int style_ = 0;
	int data_  = 0;

	// parentPath must include \ at the end
	void assign(const std::wstring& parent_path, const WIN32_FIND_DATA& wfd, const TypeTable& exts) {
		name_ = &wfd.cFileName[0];
		path_ = parent_path + name_;
		date_ = wfd.ftLastWriteTime;
		size_ = (static_cast<unsigned long long>(wfd.nFileSizeHigh) << 32) | wfd.nFileSizeLow;

		const auto is_hidden = (wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)    != 0;
		const auto is_dir    = (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		check_file(is_dir, is_hidden, exts);
	}

	void assign(const std::wstring& path, const TypeTable& exts) {
		path_ = path;
		name_ = Path::name(path_);

		auto attr      = ::GetFileAttributes(Path::ensure_unc_prefix(path_).c_str());
		auto is_dir    = (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
		auto is_hidden = (attr & FILE_ATTRIBUTE_HIDDEN) != 0;

		// When there is no file
		if (attr == INVALID_FILE_ATTRIBUTES) {
			is_dir    = false;
			is_hidden = true;
		}
		// Measures to prevent drive from appearing as hidden file
		if (Path::is_root(path_)) is_hidden = false;

		check_file(is_dir, is_hidden, exts);  // File item check
	}

	// Examine file items
	void check_file(bool is_dir, bool is_hidden, const TypeTable& exts) {
		auto ext = Path::ext(name_);  // Get extension
		style_ = 0;
		if (!is_dir && ext == L"lnk") {  // When it is a link
			name_.resize(name_.size() - 4);  // remove .lnk
			auto link_path = Link::resolve(path_);
			auto attr = ::GetFileAttributes(Path::ensure_unc_prefix(link_path).c_str());
			if (attr == INVALID_FILE_ATTRIBUTES) {  // When the link is broken
				style_ = LINK | HIDE;
				color_ = -1;
				return;
			}
			is_dir = (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
			if (!is_dir) ext = Path::ext(link_path);  // Acquisition of extension of link destination
			style_ = LINK;
		}
		style_ |= (is_dir ? DIR : 0) | (is_hidden ? HIDE : 0);
		color_ = exts.get_color(is_dir ? EXT_FOLDER : ext);
	}

public:

	Item() noexcept : date_({ 0 }) {}

	Item(const Item& inst)            = delete;
	Item(Item&& inst)                 = delete;
	Item& operator=(const Item& inst) = delete;
	Item& operator=(Item&& inst)      = delete;

	Item* set_file_item(const std::wstring& parent_path, const WIN32_FIND_DATA& wfd, const TypeTable& exts) {
		assign(parent_path, wfd, exts);
		data_ = 0;
		return this;
	}

	Item* set_file_item(const std::wstring& path, const TypeTable& exts, size_t id = 0) {
		assign(path, exts);
		style_ |= MAKELONG(0, id);
		data_ = 0;
		return this;
	}

	Item* set_empty_item() {
		name_.assign(EMPTY_STR());
		style_ = Item::EMPTY;
		data_ = 0;
		return this;
	}

	Item* set_special_folder_item(const std::wstring& path, const std::wstring& name) {
		path_.assign(path);
		name_.assign(name);
		style_ = Item::DIR;
		color_ = ::GetSysColor(COLOR_GRAYTEXT);
		data_ = 0;
		return this;
	}

	int& data() noexcept {
		return data_;
	}

	const int& data() const noexcept {
		return data_;
	}

	void set_selected(bool f) noexcept {
		f ? (style_ |= SEL) : (style_ &= ~SEL);
	}

	void clear() noexcept {
		path_.clear();
		name_.clear();
		color_ = style_ = 0;
		size_ = 0UL;
		date_.dwLowDateTime = date_.dwHighDateTime = 0;
	}

	const std::wstring& path() const noexcept {
		return path_;
	}

	const std::wstring& name() const noexcept {
		return name_;
	}

	unsigned long long size() const noexcept {
		return size_;
	}

	const FILETIME& date() const noexcept {
		return date_;
	}

	int color() const noexcept {
		return color_;
	}

	size_t id() const noexcept {
		return HIWORD(style_);
	}

	bool is_empty() const noexcept {
		return (style_ & EMPTY) != 0;
	}

	bool is_selected() const noexcept {
		return (style_ & SEL) != 0;
	}

	bool is_link() const noexcept {
		return (style_ & LINK) != 0;
	}

	bool is_dir() const noexcept {
		return (style_ & DIR) != 0;
	}

	bool is_hidden() const noexcept {
		return (style_ & HIDE) != 0;
	}

	bool is_special() const noexcept {
		return path_.starts_with(L":");
	}

};
