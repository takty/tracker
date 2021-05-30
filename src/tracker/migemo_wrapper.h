/**
 *
 * Migemo Wrapper
 *
 * @author Takuto Yanagida
 * @version 2021-05-30
 *
 */


#pragma once

#include <regex>
#include <windows.h>

#include "migemo.h"
#include "string_converter.h"
#include "file_system.hpp"


class Migemo {

	typedef migemo* (*MIGEMO_OPEN)(char* dict);
	typedef void(*MIGEMO_CLOSE)(migemo* object);
	typedef unsigned char* (*MIGEMO_QUERY)(migemo* object, unsigned char* query);
	typedef void(*MIGEMO_RELEASE)(migemo* object, unsigned char* string);

	MIGEMO_OPEN    migemo_open_    = nullptr;
	MIGEMO_CLOSE   migemo_close_   = nullptr;
	MIGEMO_QUERY   migemo_query_   = nullptr;
	MIGEMO_RELEASE migemo_release_ = nullptr;

	bool      is_loaded_ = false;
	HINSTANCE hmigemo_  = nullptr;
	migemo*   m_        = nullptr;

public:

	Migemo() noexcept {}

	Migemo(const Migemo& inst)            = delete;
	Migemo(Migemo&& inst)                 = delete;
	Migemo& operator=(const Migemo& inst) = delete;
	Migemo& operator=(Migemo&& inst)      = delete;

	~Migemo() noexcept {
		if (is_loaded_) free_library();
	}

	bool load_library(const std::wstring& dictionaryPath = std::wstring()) noexcept {
		is_loaded_ = false;

		hmigemo_ = ::LoadLibrary(_T("Migemo.dll"));
		if (hmigemo_) {
			migemo_open_    = (MIGEMO_OPEN)   ::GetProcAddress(hmigemo_, "migemo_open");
			migemo_close_   = (MIGEMO_CLOSE)  ::GetProcAddress(hmigemo_, "migemo_close");
			migemo_query_   = (MIGEMO_QUERY)  ::GetProcAddress(hmigemo_, "migemo_query");
			migemo_release_ = (MIGEMO_RELEASE)::GetProcAddress(hmigemo_, "migemo_release");

			std::wstring dp(dictionaryPath);
			if (dp.empty()) {
				dp = Path::parent(FileSystem::module_file_path()).append(_T("\\Dict\\migemo-dict"));
			}
			m_ = migemo_open_((char*)StringConverter::to_ansi(dp.c_str()).c_str());
			if (m_ == nullptr) return false;
			is_loaded_ = true;
		}
		return is_loaded_;
	}

	void query(const std::wstring& searchStr, std::wstring& query) noexcept {
		auto ansi = StringConverter::to_ansi(searchStr);
		auto q = migemo_query_(m_, reinterpret_cast<unsigned char*>(ansi.data()));
		std::string temp(reinterpret_cast<const char*>(q));
		migemo_release_(m_, q);
		query.assign(StringConverter::to_wide(temp));
		query = std::regex_replace(query, std::wregex(L"\\{"), L"\\{");
		query = std::regex_replace(query, std::wregex(L"\\}"), L"\\}");
	}

	void free_library() noexcept {
		if (is_loaded_) {
			migemo_close_(m_);
			::FreeLibrary(hmigemo_);
			is_loaded_ = false;
		}
	}

	bool is_loaded() noexcept {
		return is_loaded_;
	}

};
