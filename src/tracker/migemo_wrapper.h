/**
 * Migemo Wrapper
 *
 * @author Takuto Yanagida
 * @version 2025-11-19
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

	MIGEMO_OPEN    migemo_open_    = nullptr;
	MIGEMO_CLOSE   migemo_close_   = nullptr;
	MIGEMO_QUERY   migemo_query_   = nullptr;
	MIGEMO_RELEASE migemo_release_ = nullptr;

	bool            stand_by_ = false;
	HINSTANCE       hmigemo_ = nullptr;
	migemo*         m_       = nullptr;
	StringConverter sc_;

public:

	Migemo() noexcept = default;
	Migemo(const Migemo&) = delete;
	Migemo& operator=(const Migemo&) = delete;
	Migemo(Migemo&&) = delete;
	Migemo& operator=(Migemo&&) = delete;

	~Migemo() {
		if (stand_by_) free_library();
	}

	bool load_library(const std::wstring& dictPath = std::wstring()) {
		hmigemo_ = ::LoadLibrary(_T("Migemo.dll"));
		if (hmigemo_) {
			[[gsl::suppress(type.1)]]
			migemo_open_    = reinterpret_cast<MIGEMO_OPEN>(GetProcAddress(hmigemo_, "migemo_open"));
			[[gsl::suppress(type.1)]]
			migemo_close_   = reinterpret_cast<MIGEMO_CLOSE>(GetProcAddress(hmigemo_, "migemo_close"));
			[[gsl::suppress(type.1)]]
			migemo_query_   = reinterpret_cast<MIGEMO_QUERY>(GetProcAddress(hmigemo_, "migemo_query"));
			[[gsl::suppress(type.1)]]
			migemo_release_ = reinterpret_cast<MIGEMO_RELEASE>(GetProcAddress(hmigemo_, "migemo_release"));

			std::wstring dp(dictPath);
			if (dp.empty()) {
				dp = path::parent(file_system::module_file_path()).append(_T("\\Dict\\migemo-dict"));
			}
			auto mbs = sc_.wc2mb(dp);
			m_ = migemo_open_(mbs.get());
			if (m_) return true;
		}
		return false;
	}

	void query(const std::wstring& searchWord, std::wstring& query) {
		auto mbs = sc_.wc2mb(searchWord);
		[[gsl::suppress(type.1)]]
		auto p = migemo_query_(m_, reinterpret_cast<unsigned char*>(mbs.get()));
		[[gsl::suppress(type.1)]]
		std::string temp{ reinterpret_cast<const char*>(p) };

		query.assign(sc_.mb2wc(temp).get());

		migemo_release_(m_, p);
	}

	void free_library() noexcept {
		if (stand_by_) {
			migemo_close_(m_);
			FreeLibrary(hmigemo_);
			stand_by_ = false;
		}
	}

	bool is_stand_by() const noexcept {
		return stand_by_;
	}

};
