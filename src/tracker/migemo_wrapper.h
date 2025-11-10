/**
 * Migemo Wrapper
 *
 * @author Takuto Yanagida
 * @version 2025-11-10
 */

#pragma once

#include <windows.h>

#include "gsl/gsl"
#include "migemo.h"
#include "string_converter.h"

class Migemo {

	typedef migemo* (__stdcall *MIGEMO_OPEN)(const char* dict);
	typedef void(__stdcall *MIGEMO_CLOSE)(migemo* object);
	typedef unsigned char* (__stdcall *MIGEMO_QUERY)(migemo* object, const unsigned char* query);
	typedef void(__stdcall *MIGEMO_RELEASE)(migemo* object, unsigned char* string);

	MIGEMO_OPEN    migemoOpen_    = nullptr;
	MIGEMO_CLOSE   migemoClose_   = nullptr;
	MIGEMO_QUERY   migemoQuery_   = nullptr;
	MIGEMO_RELEASE migemoRelease_ = nullptr;

	bool            standBy_ = false;
	HINSTANCE       hMigemo_ = nullptr;
	migemo*         m_       = nullptr;
	StringConverter sc_;

public:

	Migemo() noexcept = default;

	Migemo(const Migemo&) = delete;
	Migemo& operator=(const Migemo&) = delete;
	Migemo(Migemo&&) = delete;
	Migemo& operator=(Migemo&&) = delete;

	~Migemo() {
		if (standBy_) freeLibrary();
	}

	bool loadLibrary(const std::wstring& dictPath = std::wstring()) {
		hMigemo_ = ::LoadLibrary(_T("Migemo.dll"));
		if (hMigemo_) {
			[[gsl::suppress(type.1)]]
			migemoOpen_    = reinterpret_cast<MIGEMO_OPEN>(GetProcAddress(hMigemo_, "migemo_open"));
			[[gsl::suppress(type.1)]]
			migemoClose_   = reinterpret_cast<MIGEMO_CLOSE>(GetProcAddress(hMigemo_, "migemo_close"));
			[[gsl::suppress(type.1)]]
			migemoQuery_   = reinterpret_cast<MIGEMO_QUERY>(GetProcAddress(hMigemo_, "migemo_query"));
			[[gsl::suppress(type.1)]]
			migemoRelease_ = reinterpret_cast<MIGEMO_RELEASE>(GetProcAddress(hMigemo_, "migemo_release"));

			std::wstring dp(dictPath);
			if (dp.empty()) {
				dp = path::parent(file_system::module_file_path()).append(_T("\\Dict\\migemo-dict"));
			}
			auto mbs = sc_.wc2mb(dp);
			m_ = migemoOpen_(mbs.get());
			if (m_) return true;
		}
		return false;
	}

	void query(const std::wstring& searchWord, std::wstring& query) {
		auto mbs = sc_.wc2mb(searchWord);
		[[gsl::suppress(type.1)]]
		auto p = migemoQuery_(m_, reinterpret_cast<unsigned char*>(mbs.get()));
		[[gsl::suppress(type.1)]]
		std::string temp{ reinterpret_cast<const char*>(p) };

		StringConverter sc{};
		query.assign(sc.mb2wc(temp).get());

		migemoRelease_(m_, p);
	}

	void freeLibrary() noexcept {
		if (standBy_) {
			migemoClose_(m_);
			FreeLibrary(hMigemo_);
			standBy_ = false;
		}
	}

	bool isStandBy() const noexcept {
		return standBy_;
	}

};
