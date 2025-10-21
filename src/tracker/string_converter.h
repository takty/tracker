/**
 *
 * String Converter
 *
 * @author Takuto Yanagida
 * @version 2025-10-21
 *
 */


#pragma once

#include <windows.h>
#include <tchar.h>
#include <string>


class StringConverter {

	char* buf_  = nullptr;
	int   size_ = 0;

public:

	StringConverter() noexcept = default;

	StringConverter(const StringConverter&) = delete;
	StringConverter& operator=(const StringConverter&) = delete;
	StringConverter(StringConverter&&) = delete;
	StringConverter& operator=(StringConverter&&) = delete;

	~StringConverter() {
		if (buf_ != nullptr) delete[] buf_;
	}

	const char* convert(const std::wstring& str) {
		const int size = ::WideCharToMultiByte(CP_THREAD_ACP, 0, str.c_str(), -1, nullptr, 0, nullptr, nullptr);
		if (size_ < size) {
			if (buf_ != nullptr) delete[] buf_;
			buf_  = new char[size];
			size_ = size;
		}
		// Calculates the number of characters including NULL by specifying -1
		::WideCharToMultiByte(CP_THREAD_ACP, 0, str.c_str(), -1, (LPSTR)buf_, size_, nullptr, nullptr);
		return buf_;
	}

};
