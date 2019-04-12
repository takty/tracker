//
// Migemo Wrapper
// 2019-04-12
//


#pragma once

#include <windows.h>

#include "migemo.h"
#include "string_converter.h"


class Migemo {

	typedef migemo* (*MIGEMO_OPEN)(char* dict);
	typedef void(*MIGEMO_CLOSE)(migemo* object);
	typedef unsigned char* (*MIGEMO_QUERY)(migemo* object, unsigned char* query);
	typedef void(*MIGEMO_RELEASE)(migemo* object, unsigned char* string);

	MIGEMO_OPEN migemoOpen;
	MIGEMO_CLOSE migemoClose;
	MIGEMO_QUERY migemoQuery;
	MIGEMO_RELEASE migemoRelease;

	bool standBy_;
	HINSTANCE hMigemo_;
	migemo* m_;
	StringConverter sc_;

public:

	Migemo() : standBy_(false) {
	}

	~Migemo() {
		if (standBy_) freeLibrary();
	}

	bool loadLibrary(const wstring& dictPath = wstring()) {
		standBy_ = false;

		hMigemo_ = ::LoadLibrary(_T("Migemo.dll"));
		if (hMigemo_) {
			migemoOpen = (MIGEMO_OPEN)GetProcAddress(hMigemo_, "migemo_open");
			migemoClose = (MIGEMO_CLOSE)GetProcAddress(hMigemo_, "migemo_close");
			migemoQuery = (MIGEMO_QUERY)GetProcAddress(hMigemo_, "migemo_query");
			migemoRelease = (MIGEMO_RELEASE)GetProcAddress(hMigemo_, "migemo_release");

			wstring dp(dictPath);
			if (dp.empty()) {
				dp = Path::parent(FileSystem::module_file_path()).append(_T("\\Dict\\migemo-dict"));
			}
			m_ = migemoOpen((char*)sc_.convert(dp.c_str()));
			if (m_ == nullptr) return false;
			standBy_ = true;
		}
		return standBy_;
	}

	void query(const wstring& searchWord, string& query) {
		auto mbs = sc_.convert(searchWord);
		auto p = migemoQuery(m_, (unsigned char*)mbs);
		query.assign("/").append((const char*)p).append("/ki");  // Handle as Japanese, case-insensitive
		migemoRelease(m_, p);
	}

	void freeLibrary() {
		if (standBy_) {
			migemoClose(m_);
			FreeLibrary(hMigemo_);
			standBy_ = false;
		}
	}

	bool isStandBy() {
		return standBy_;
	}

};
