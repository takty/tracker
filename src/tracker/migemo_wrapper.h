/**
 *
 * Migemo Wrapper
 *
 * @author Takuto Yanagida
 * @version 2020-03-22
 *
 */


#pragma once

#include <windows.h>

#include "migemo.h"
#include "string_converter.h"


class Migemo {

	typedef migemo* (*MIGEMO_OPEN)(char* dict);
	typedef void(*MIGEMO_CLOSE)(migemo* object);
	typedef unsigned char* (*MIGEMO_QUERY)(migemo* object, unsigned char* query);
	typedef void(*MIGEMO_RELEASE)(migemo* object, unsigned char* string);

	MIGEMO_OPEN    migemoOpen_    = nullptr;
	MIGEMO_CLOSE   migemoClose_   = nullptr;
	MIGEMO_QUERY   migemoQuery_   = nullptr;
	MIGEMO_RELEASE migemoRelease_ = nullptr;

	bool            standBy_ = false;
	HINSTANCE       hMigemo_ = nullptr;
	migemo*         m_       = nullptr;
	StringConverter sc_;

public:

	Migemo() {
	}

	~Migemo() {
		if (standBy_) freeLibrary();
	}

	bool loadLibrary(const std::wstring& dictPath = std::wstring()) {
		standBy_ = false;

		hMigemo_ = ::LoadLibrary(_T("Migemo.dll"));
		if (hMigemo_) {
			migemoOpen_    = (MIGEMO_OPEN)GetProcAddress(hMigemo_, "migemo_open");
			migemoClose_   = (MIGEMO_CLOSE)GetProcAddress(hMigemo_, "migemo_close");
			migemoQuery_   = (MIGEMO_QUERY)GetProcAddress(hMigemo_, "migemo_query");
			migemoRelease_ = (MIGEMO_RELEASE)GetProcAddress(hMigemo_, "migemo_release");

			std::wstring dp(dictPath);
			if (dp.empty()) {
				dp = Path::parent(FileSystem::module_file_path()).append(_T("\\Dict\\migemo-dict"));
			}
			m_ = migemoOpen_((char*)sc_.convert(dp.c_str()));
			if (m_ == nullptr) return false;
			standBy_ = true;
		}
		return standBy_;
	}

	void query(const std::wstring& searchWord, std::string& query) {
		auto mbs = sc_.convert(searchWord);
		auto p = migemoQuery_(m_, (unsigned char*)mbs);
		query.assign("/").append((const char*)p).append("/ki");  // Handle as Japanese, case-insensitive
		migemoRelease_(m_, p);
	}

	void freeLibrary() {
		if (standBy_) {
			migemoClose_(m_);
			FreeLibrary(hMigemo_);
			standBy_ = false;
		}
	}

	bool isStandBy() {
		return standBy_;
	}

};
