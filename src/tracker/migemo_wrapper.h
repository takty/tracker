/**
 *
 * Migemo Wrapper
 *
 * @author Takuto Yanagida
 * @version 2021-05-09
 *
 */


#pragma once

#include <windows.h>
#include <regex>

#include "migemo.h"
#include "string_converter.h"
#include "file_system.hpp"


class Migemo {

	typedef migemo* (*MIGEMO_OPEN)(char* dict);
	typedef void(*MIGEMO_CLOSE)(migemo* object);
	typedef unsigned char* (*MIGEMO_QUERY)(migemo* object, unsigned char* query);
	typedef void(*MIGEMO_RELEASE)(migemo* object, unsigned char* string);

	MIGEMO_OPEN    migemoOpen_    = nullptr;
	MIGEMO_CLOSE   migemoClose_   = nullptr;
	MIGEMO_QUERY   migemoQuery_   = nullptr;
	MIGEMO_RELEASE migemoRelease_ = nullptr;

	bool      isLoaded_ = false;
	HINSTANCE hMigemo_  = nullptr;
	migemo*   m_        = nullptr;

public:

	Migemo() noexcept {}

	Migemo(const Migemo& inst) = delete;
	Migemo(Migemo&& inst) = delete;
	Migemo& operator=(const Migemo& inst) = delete;
	Migemo& operator=(Migemo&& inst) = delete;

	~Migemo() noexcept(false) {
		if (isLoaded_) freeLibrary();
	}

	bool loadLibrary(const std::wstring& dictionaryPath = std::wstring()) {
		isLoaded_ = false;

		hMigemo_ = ::LoadLibrary(_T("Migemo.dll"));
		if (hMigemo_) {
			migemoOpen_ = (MIGEMO_OPEN) ::GetProcAddress(hMigemo_, "migemo_open");
			migemoClose_ = (MIGEMO_CLOSE) ::GetProcAddress(hMigemo_, "migemo_close");
			migemoQuery_ = (MIGEMO_QUERY) ::GetProcAddress(hMigemo_, "migemo_query");
			migemoRelease_ = (MIGEMO_RELEASE) ::GetProcAddress(hMigemo_, "migemo_release");

			std::wstring dp(dictionaryPath);
			if (dp.empty()) {
				dp = Path::parent(FileSystem::module_file_path()).append(_T("\\Dict\\migemo-dict"));
			}
			m_ = migemoOpen_((char*)StringConverter::to_ansi(dp.c_str()).c_str());
			if (m_ == nullptr) return false;
			isLoaded_ = true;
		}
		return isLoaded_;
	}

	void query(const std::wstring& searchStr, std::wstring& query) {
		auto ansi = StringConverter::to_ansi(searchStr);
		auto q = migemoQuery_(m_, (unsigned char*)ansi.c_str());
		std::string temp{ (const char*)q };
		migemoRelease_(m_, q);
		query.assign(StringConverter::to_wide(temp));
		query = std::regex_replace(query, std::wregex(L"\\{"), L"\\{");
		query = std::regex_replace(query, std::wregex(L"\\}"), L"\\}");
	}

	void freeLibrary() noexcept {
		if (isLoaded_) {
			migemoClose_(m_);
			::FreeLibrary(hMigemo_);
			isLoaded_ = false;
		}
	}

	bool isStandBy() noexcept {
		return isLoaded_;
	}

};
