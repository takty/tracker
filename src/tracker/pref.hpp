#pragma once

#include <vector>
#include <string>
#include <sstream>

#include <windows.h>

#include "file_utils.hpp"


//
// Reading and writing preferences (INI file)
// 2019-04-12
//

class Pref {

	std::wstring iniPath_;
	std::wstring curSec_;

	static std::wstring get_user_name() {
		DWORD bufLen = MAX_PATH;
		std::vector<wchar_t> buf(bufLen);

		while (true) {
			::GetUserName(buf.data(), &bufLen);
			if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) break;
			bufLen *= 2;
			buf.resize(bufLen);
		}
		return std::wstring{ buf.data() };
	}

public:

	Pref() {
		iniPath_ = FileSystem::module_file_path();
		iniPath_.insert(0, Path::UNC_PREFIX);  // To handle long paths
		iniPath_.resize(iniPath_.size() - 3);
		iniPath_.append(L"ini");
	}

	// Make INI file path for multi user
	void set_multi_user_mode() {
		// Save the path of the normal INI file
		auto normalPath{ iniPath_ };

		// Create INI file path for current user
		auto name = Path::name(normalPath);
		auto path = Path::parent(normalPath);
		auto user = get_user_name();
		path.append(L"\\").append(user);
		iniPath_.assign(path).append(L"\\").append(name);

		// When a normal INI file exists and there is no INI file for the current user
		if (FileSystem::is_existing(normalPath) && !FileSystem::is_existing(iniPath_)) {
			::CreateDirectory(path.c_str(), nullptr);  // Make a directory
			::CopyFile(normalPath.c_str(), iniPath_.c_str(), TRUE);  // Copy
		}
	}

	// Get INI file path
	const std::wstring& path() const { return iniPath_; }

	// Set the current section
	void set_current_section(const std::wstring& sec) { curSec_.assign(sec); }

	// Get string item
	std::wstring item(const wchar_t* sec, const wchar_t* key, const wchar_t* def) const {
		std::vector<wchar_t> buf(MAX_PATH);

		while (true) {
			auto outLen = ::GetPrivateProfileString(sec, key, def, buf.data(), buf.size(), iniPath_.c_str());
			if (outLen != buf.size() - 1) break;
			buf.resize(buf.size() * 2);
		}
		return{ buf.data() };
	}

	// Get string item
	std::wstring item(const std::wstring& sec, const std::wstring& key, const std::wstring& def) const {
		return item(sec.c_str(), key.c_str(), def.c_str());
	}

	// Get string item
	std::wstring item(const wchar_t* key, const wchar_t* def) const {
		return item(curSec_.c_str(), key, def);
	}

	// Get string item
	std::wstring item(const std::wstring& key, const std::wstring& def) const {
		return item(curSec_.c_str(), key.c_str(), def.c_str());
	}

	// Write a string item
	void set_item(const std::wstring& str, const std::wstring& sec, const std::wstring& key) {
		::WritePrivateProfileString(sec.c_str(), key.c_str(), str.c_str(), iniPath_.c_str());
	}

	// Write a string item
	void set_item(const std::wstring& str, const std::wstring& key) {
		set_item(str, curSec_, key);
	}

	// Get integer item
	int item_int(const wchar_t* sec, const wchar_t* key, int def) {
		return ::GetPrivateProfileInt(sec, key, def, iniPath_.c_str());
	}

	// Get integer item
	int item_int(const std::wstring& sec, const std::wstring& key, int def) {
		return item_int(sec.c_str(), key.c_str(), def);
	}

	// Get integer item
	int item_int(const wchar_t* key, int def) {
		return item_int(curSec_.c_str(), key, def);
	}

	// Get integer item
	int item_int(const std::wstring& key, int def) {
		return item_int(curSec_.c_str(), key.c_str(), def);
	}

	// Write an integer item
	void set_item_int(const std::wstring& sec, const std::wstring& key, int val) {
		::WritePrivateProfileString(sec.c_str(), key.c_str(), std::to_wstring(val).c_str(), iniPath_.c_str());
	}

	// Write an integer item
	void set_item_int(const std::wstring& key, int val) {
		set_item_int(curSec_, key, val);
	}

	// Get section content
	template <typename Container> auto items(const std::wstring& sec, const std::wstring& key, int max) -> Container {
		Container c;
		std::wstring def;
		for (int i = 0; i < max; ++i) {
			auto temp = item(sec, key + std::to_wstring(i + 1), def);
			if (temp.empty()) continue;
			c.resize(c.size() + 1);
			c.back() = temp;
		}
		return c;
	}

	// Write the content of the section
	template <typename Container> void set_items(const Container& c, const std::wstring& sec, const std::wstring& key) {
		::WritePrivateProfileString(sec.c_str(), nullptr, nullptr, iniPath_.c_str());
		int i = 0;
		for (const auto& it : c) {
			std::wostringstream vss;
			vss << it;
			set_item(vss.str(), sec, key + std::to_wstring(i + 1));
			i += 1;
		}
	}

};
