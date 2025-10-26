/**
 * String Converter
 *
 * @author Takuto Yanagida
 * @version 2025-10-26
 */

#pragma once

#include <memory>

#include <windows.h>
#include <tchar.h>
#include <string>

#include "classes.h"

class StringConverter {

	std::shared_ptr<char[]> cbuf_;
	int csize_ = 0;

	std::shared_ptr<wchar_t[]> wbuf_;
	int wsize_ = 0;

public:

	StringConverter() noexcept = default;

	StringConverter(const StringConverter&) = delete;
	StringConverter& operator=(const StringConverter&) = delete;
	StringConverter(StringConverter&&) = delete;
	StringConverter& operator=(StringConverter&&) = delete;

	~StringConverter() = default;

	std::shared_ptr<char[]> wc2mb(const std::wstring& str) {
		const int s = ::WideCharToMultiByte(CP_THREAD_ACP, 0, str.c_str(), -1, nullptr, 0, nullptr, nullptr);
		if (csize_ < s) {
			cbuf_  = std::make_shared<char[]>(s);
			csize_ = s;
		}
		::WideCharToMultiByte(CP_THREAD_ACP, 0, str.c_str(), -1, cbuf_.get(), csize_, nullptr, nullptr);
		return cbuf_;
	}

	std::shared_ptr<wchar_t[]> mb2wc(const std::string& str) {
		const int s = ::MultiByteToWideChar(CP_THREAD_ACP, 0, str.c_str(), -1, nullptr, 0);
		if (wsize_ < s) {
			wbuf_  = std::make_shared<wchar_t[]>(s);
			wsize_ = s;
		}
		::MultiByteToWideChar(CP_THREAD_ACP, 0, str.c_str(), -1, wbuf_.get(), wsize_);
		return wbuf_;
	}

};
