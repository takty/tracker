/**
 * File Item
 *
 * @author Takuto Yanagida
 * @version 2025-11-10
 */

#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <memory>

#include "file_utils.hpp"
#include "type_table.h"

class Item {

	inline static const std::wstring EMPTY_STR{ L"empty" };

	inline static const std::wstring EXT_FOLDER{ L"<folder>" };

	inline static std::vector<std::shared_ptr<Item>> cache_;

public:

	static std::shared_ptr<Item> create() {
		if (cache_.empty()) {
			auto it = std::make_shared<Item>();
			return it;
		}
		std::shared_ptr<Item> it = cache_.back();
		cache_.pop_back();
		return it;
	}

	static void destroy(std::shared_ptr<Item> it) {
		it->clear();
		cache_.push_back(it);
	}

private:

	enum { DIR = 2, HIDE = 4, LINK = 8, HIER = 16, SEL = 32, EMPTY = 64 };

	std::wstring path_{};
	std::wstring name_{};
	FILETIME time_{};
	unsigned long long size_{};

	int color_{};
	int style_{};
	int data_{};
	size_t id_{};

	void clear() noexcept {
		path_.clear();
		name_.clear();
		time_.dwLowDateTime = 0;
		time_.dwHighDateTime = 0;
		size_ = 0UL;

		style_ = 0;
		color_ = 0;
		data_ = 0;
		id_ = 0;
	}

	void check_file(bool isDir, bool isHidden, const TypeTable& exts) {
		auto ext = path::ext(name_);  // Get extension
		style_ = 0;
		if (!isDir && ext == L"lnk") {  // When it is a link
			name_.resize(name_.size() - 4);  // remove .lnk
			auto linkPath   = link::resolve(path_);
			const auto attr = ::GetFileAttributes(path::ensure_unc_prefix(path_).c_str());

			if (attr == INVALID_FILE_ATTRIBUTES) {  // When the link is broken
				style_ = LINK | HIDE;
				color_ = -1;
				return;
			}
			isDir = (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
			if (!isDir) ext = path::ext(linkPath);  // Acquisition of extension of link destination
			style_ = LINK;
		}
		style_ |= (isDir ? DIR : 0) | (isHidden ? HIDE : 0);
		color_ = exts.get_color(isDir ? EXT_FOLDER : ext);
	}

public:

	Item() noexcept {}
	Item(const Item&) = delete;
	Item& operator=(const Item&) = delete;
	Item(Item&&) = delete;
	Item& operator=(Item&&) = delete;
	~Item() = default;

	void set_file(const std::wstring& parentPath, const WIN32_FIND_DATA& wfd, const TypeTable& exts) {
		// parentPath must include \ at the end
		path_ = parentPath + std::wstring{ &wfd.cFileName[0] };
		name_ = std::wstring{ &wfd.cFileName[0] };
		time_ = wfd.ftLastWriteTime;
		size_ = (static_cast<unsigned long long>(wfd.nFileSizeHigh) << 32) | wfd.nFileSizeLow;

		const auto isDir    = (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		const auto isHidden = (wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)    != 0;

		check_file(isDir, isHidden, exts);
	}

	void set_file(const std::wstring& path, const TypeTable& exts, size_t id = 0) {
		path_ = path;
		name_ = path::name(path_);
		id_   = id;

		const auto attr = ::GetFileAttributes(path::ensure_unc_prefix(path_).c_str());
		auto isDir      = (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
		auto isHidden   = (attr & FILE_ATTRIBUTE_HIDDEN) != 0;

		// When there is no file
		if (attr == INVALID_FILE_ATTRIBUTES) {
			isDir    = false;
			isHidden = true;
		}
		// Measures to prevent drive from appearing as hidden file
		if (path::is_root(path_)) isHidden = false;

		check_file(isDir, isHidden, exts);  // File item check
	}

	void set_empty() {
		path_.clear();
		name_  = Item::EMPTY_STR;
		style_ = Item::EMPTY;
	}

	void set_special(const std::wstring& path, const std::wstring& name) {
		path_  = path;
		name_  = name;
		style_ = Item::DIR;
		color_ = ::GetSysColor(COLOR_GRAYTEXT);
	}

	// ----

	const std::wstring& path() const noexcept {
		return path_;
	}

	const std::wstring& name() const noexcept {
		return name_;
	}

	const FILETIME& time() const noexcept {
		return time_;
	}

	unsigned long long size() const noexcept {
		return size_;
	}

	// ----

	int color() const noexcept {
		return color_;
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

	bool is_hier() const noexcept {
		return (style_ & HIER) != 0;
	}

	void set_sel(bool f) noexcept {
		f ? (style_ |= SEL) : (style_ &= ~SEL);
	}

	bool is_sel() const noexcept {
		return (style_ & SEL) != 0;
	}

	bool is_empty() const noexcept {
		return (style_ & EMPTY) != 0;
	}

	int& data() noexcept {
		return data_;
	}

	const int& data() const noexcept {
		return data_;
	}

	size_t id() const noexcept {
		return id_;
	}

};
