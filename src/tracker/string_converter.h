/**
 * String Converter
 *
 * @author Takuto Yanagida
 * @version 2026-04-29
 */

#pragma once

#include <memory>

#include <windows.h>
#include <tchar.h>
#include <string>

#include "classes.h"

class StringConverter {

	template<typename T> struct Buf {
		std::shared_ptr<T[]> data;
		int size = 0;
		void ensure(int n) {
			if (size < n) {
				data = std::make_shared<T[]>(n);
				size = n;
			}
		}
	};

	Buf<char>    cbuf_;
	Buf<wchar_t> wbuf_;

public:

	StringConverter() noexcept = default;
	StringConverter(const StringConverter&) = delete;
	StringConverter& operator=(const StringConverter&) = delete;
	StringConverter(StringConverter&&) = delete;
	StringConverter& operator=(StringConverter&&) = delete;
	~StringConverter() = default;

	std::shared_ptr<char[]> wc2mb(const std::wstring& str) {
		const int s = ::WideCharToMultiByte(CP_THREAD_ACP, 0, str.c_str(), -1, nullptr, 0, nullptr, nullptr);
		cbuf_.ensure(s);
		::WideCharToMultiByte(CP_THREAD_ACP, 0, str.c_str(), -1, cbuf_.data.get(), cbuf_.size, nullptr, nullptr);
		return cbuf_.data;
	}

	std::shared_ptr<wchar_t[]> mb2wc(const std::string& str) {
		const int s = ::MultiByteToWideChar(CP_THREAD_ACP, 0, str.c_str(), -1, nullptr, 0);
		wbuf_.ensure(s);
		::MultiByteToWideChar(CP_THREAD_ACP, 0, str.c_str(), -1, wbuf_.data.get(), wbuf_.size);
		return wbuf_.data;
	}

};
