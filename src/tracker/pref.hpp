/**
 *
 * Preference (Reading and writing file)
 *
 * @author Takuto Yanagida
 * @version 2021-05-16
 *
 */


#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <locale>
#include <windows.h>

#include "file_utils.hpp"


class Pref {

	static std::wstring get_user_name() {
		DWORD len{ MAX_PATH };
		std::vector<wchar_t> buf(len);

		while (true) {
			::GetUserNameW(buf.data(), &len);
			if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) break;
		}
		return std::wstring{ buf.data(), len - 1 };  // The len contains '\0'
	}

	static std::wstring trim(const std::wstring& str, const wchar_t* chars = L" \t\v\r\n") {
		auto left = str.find_first_not_of(chars);
		if (left == std::string::npos) {
			return {};
		}
		auto right = str.find_last_not_of(chars);
		return str.substr(left, right - left + 1);
	}


	// ------------------------------------------------------------------------


	std::wstring file_path_{};
	std::vector<std::wstring> cache_{};
	std::wstring last_section_{};
	int last_section_index_{};

	int get_section_index(const std::wstring& sec) {
		if (last_section_ == sec) {
			return last_section_index_;
		}
		last_section_index_ = -1;
		const auto s = L"[" + sec + L"]";
		for (auto i = 0U; i < cache_.size(); ++i) {
			auto line = trim(cache_[i]);
			if (line == s) {
				last_section_index_ = i + 1;
				break;
			}
		}
		if (last_section_index_ != -1) {
			last_section_ = sec;
		}
		return last_section_index_;
	}

	std::wstring retreive_item(const std::wstring& sec, const std::wstring& key, const std::wstring& def = L"") {
		if (cache_.empty()) {
			cache_ = load_lines<std::vector<std::wstring>>();
		}
		const int sidx = get_section_index(sec);
		if (sidx == -1) return def;


		for (auto i = size_t(sidx); i < cache_.size(); ++i) {
			auto line = trim(cache_[i]);
			if (line.empty()) {
				continue;
			}
			if (line.front() == L'[') {
				return def;
			}
			if (line.compare(0, key.size(), key) != 0) {
				continue;
			}
			auto pos = line.find_first_not_of(L" \t\v\r\n", key.size());
			if (pos != std::string::npos && line.at(pos) == L'=') {
				return trim(line.substr(pos + 1));
			}
		}
		return def;
	}

	void assign_item(const std::wstring& sec, const std::wstring& key, const std::wstring& val) {
		if (cache_.empty()) {
			cache_ = load_lines<std::vector<std::wstring>>();
		}
		const int sidx = get_section_index(sec);
		if (sidx == -1) {
			cache_.push_back(L"[" + sec + L"]");
			cache_.push_back(key + L"=" + val);
			last_section_.clear();
			last_section_index_ = -1;
			return;
		}
		for (auto i = size_t(sidx); i < cache_.size(); ++i) {
			auto line = trim(cache_[i]);
			if (line.empty()) {
				continue;
			}
			if (line.front() == L'[') {
				cache_.insert(cache_.begin() + i, key + L"=" + val);
				last_section_.clear();
				last_section_index_ = -1;
				return;
			}
			if (line.compare(0, key.size(), key) != 0) {
				continue;
			}
			auto pos = line.find_first_not_of(L" \t\v\r\n", key.size());
			if (pos != std::string::npos && line.at(pos) == L'=') {
				cache_.at(i) = key + L"=" + val;
				return;
			}
		}
	}

public:

	Pref() noexcept(false) {
		auto mp = FileSystem::module_file_path();
		mp.insert(0, Path::UNC_PREFIX);  // To handle long paths
		file_path_.assign(mp.begin(), mp.end() - 3).append(L"ini");
		cache_ = load_lines<std::vector<std::wstring>>();
	}

	Pref(const std::wstring& file_name) noexcept(false) {
		auto mp = FileSystem::module_file_path();
		mp.insert(0, Path::UNC_PREFIX);  // To handle long paths
		file_path_.assign(Path::parent(mp)).append(L"\\").append(file_name);
	}

	// Make the file path for multiple user
	void enable_multiple_user_mode() {
		auto orig{ file_path_ };

		// Create a file path for current user
		auto path = Path::parent(orig).append(L"\\").append(get_user_name());
		file_path_.assign(path).append(L"\\").append(Path::name(orig));

		// When a normal file exists and there is no file for the current user
		if (FileSystem::exists(orig) && !FileSystem::exists(file_path_)) {
			FileSystem::create_directory(path);
			FileSystem::copy_file(orig, file_path_);
		}
	}

	// Get the file path
	const std::wstring& path() const noexcept {
		return file_path_;
	}

	void store() {
		if (!cache_.empty()) {
			save_lines(cache_);
		}
	}


	// ------------------------------------------------------------------------


	// Get string item
	std::wstring get(const std::wstring& sec, const std::wstring& key, const std::wstring& def) const {
		return const_cast<Pref*>(this)->retreive_item(sec, key, def);
	}

	// Get integer item
	int get(const std::wstring& sec, const std::wstring& key, int def) noexcept {
		const auto temp = const_cast<Pref*>(this)->retreive_item(sec, key);
		return temp.empty() ? def : std::stoi(temp);
	}

	// Write a string item
	void set(const std::wstring& str, const std::wstring& sec, const std::wstring& key) noexcept {
		assign_item(sec, key, str);
	}

	// Write an integer item
	void set(const std::wstring& sec, const std::wstring& key, int val) noexcept(false) {
		assign_item(sec, key, std::to_wstring(val));
	}


	// ------------------------------------------------------------------------


	template <typename Container> auto load_lines() -> Container {
		Container c{};
		std::wifstream fs(file_path_);
		if (!fs.is_open()) {
			return c;
		}
		fs.imbue(std::locale(".utf8", std::locale::ctype));

		std::wstring line{};
		while (std::getline(fs, line)) {
			c.push_back(line);
		}
		fs.close();
		return c;
	}

	template <typename Container> void save_lines(const Container& c) {
		std::wofstream fs(file_path_);
		if (!fs.is_open()) {
			return;
		}
		fs.imbue(std::locale(".utf8", std::locale::ctype));

		for (const auto& line : c) {
			fs << line << std::endl;
		}
		fs.close();
	}

};
