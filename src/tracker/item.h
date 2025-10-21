/**
 *
 * File Item
 *
 * @author Takuto Yanagida
 * @version 2025-10-20
 *
 */


#pragma once

#include <windows.h>
#include <string>

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
	size_t id_ = 0;

	// parentPath must include \ at the end
	void Assign(const std::wstring& parentPath, const WIN32_FIND_DATA& wfd, const TypeTable& exts) {
		path_ = parentPath + wfd.cFileName;
		name_ = wfd.cFileName;
		date_ = wfd.ftLastWriteTime;
		size_ = (((unsigned long long) wfd.nFileSizeHigh) << 32) | wfd.nFileSizeLow;

		auto isHidden = (wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0;
		auto isDir = (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		CheckFile(isDir, isHidden, exts);
	}

	void Assign(const std::wstring& filePath, const TypeTable& exts) {
		path_ = filePath;
		name_ = Path::name(path_);

		const auto attr     = ::GetFileAttributes(path_.c_str());
		auto isDir    = (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
		auto isHidden = (attr & FILE_ATTRIBUTE_HIDDEN) != 0;

		// When there is no file
		if (attr == INVALID_FILE_ATTRIBUTES) {
			isDir = false;
			isHidden = true;
		}
		// Measures to prevent drive from appearing as hidden file
		if (Path::is_root(path_)) isHidden = false;

		CheckFile(isDir, isHidden, exts);  // File item check
	}

	// Examine file items
	void CheckFile(bool isDir, bool isHidden, const TypeTable& exts) {
		auto ext = Path::ext(name_);  // Get extension
		style_ = 0;
		if (!isDir && ext == L"lnk") {  // When it is a link
			name_.resize(name_.size() - 4);  // remove .lnk
			auto linkPath = Link::resolve(path_);
			const auto attr     = ::GetFileAttributes(linkPath.c_str());
			if (attr == INVALID_FILE_ATTRIBUTES) {  // When the link is broken
				style_ = LINK | HIDE;
				color_ = -1;
				return;
			}
			isDir = (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
			if (!isDir) ext = Path::ext(linkPath);  // Acquisition of extension of link destination
			style_ = LINK;
		}
		style_ |= (isDir ? DIR : 0) | (isHidden ? HIDE : 0);
		color_ = exts.get_color(isDir ? EXT_FOLDER : ext);
	}

public:

	Item() noexcept : date_({ 0 }) {}

	Item* SetFileItem(const std::wstring& parentPath, const WIN32_FIND_DATA& wfd, const TypeTable& exts) {
		Assign(parentPath, wfd, exts);
		data_ = 0;
		id_   = 0;
		return this;
	}

	Item* SetFileItem(const std::wstring& path, const TypeTable& exts, size_t id = 0) {
		Assign(path, exts);
		data_ = 0;
		id_   = id;
		return this;
	}

	Item* SetEmptyItem() {
		name_.assign(EMPTY_STR());
		style_ = Item::EMPTY;
		color_ = 0;
		data_  = 0;
		id_    = 0;
		return this;
	}

	Item* SetSpecialFolderItem(const std::wstring& path, const std::wstring& name) {
		path_.assign(path);
		name_.assign(name);
		style_ = Item::DIR;
		color_ = ::GetSysColor(COLOR_GRAYTEXT);
		data_  = 0;
		id_    = 0;
		return this;
	}

	//Item* SetSeparatorItem(bool isHier) {
	//	style_ = (Item::SEPA | (isHier ? Item::HIER : 0));
	//	return this;
	//}

	int& data() noexcept {
		return data_;
	}

	const int& data() const noexcept {
		return data_;
	}


	//void SetId(int i) noexcept {
	//	style_ |= MAKELONG(0, i);
	//}

	void SetSelected(bool f) noexcept {
		f ? (style_ |= SEL) : (style_ &= ~SEL);
	}

	void Clear() noexcept {
		path_.clear();
		name_.clear();
		color_ = style_ = 0;
		size_ = 0UL;
		date_.dwLowDateTime = date_.dwHighDateTime = 0;
		id_ = 0;
	}

	const std::wstring& Path() const noexcept {
		return path_;
	}

	const std::wstring& Name() const noexcept {
		return name_;
	}

	unsigned long long Size() const noexcept {
		return size_;
	}

	const FILETIME& Date() const noexcept {
		return date_;
	}

	int Color() const noexcept {
		return color_;
	}

	size_t Id() const noexcept {
		//return HIWORD(style_);
		return id_;
	}

	//bool IsSeparator() const {
	//	return (style_ & SEPA) != 0;
	//}

	bool IsEmpty() const noexcept {
		return (style_ & EMPTY) != 0;
	}

	bool IsSelected() const noexcept {
		return (style_ & SEL) != 0;
	}

	bool IsLink() const noexcept {
		return (style_ & LINK) != 0;
	}

	bool IsDir() const noexcept {
		return (style_ & DIR) != 0;
	}

	bool IsHidden() const noexcept {
		return (style_ & HIDE) != 0;
	}

	bool IsHier() const noexcept {
		return (style_ & HIER) != 0;
	}

};
