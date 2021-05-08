/**
 *
 * String Converter
 *
 * @author Takuto Yanagida
 * @version 2021-05-08
 *
 */


#pragma once

#include <windows.h>
#include <tchar.h>
#include <string>


class StringConverter {

	std::unique_ptr<char[]> buf_;
	int size_{};

public:

	StringConverter() noexcept {}

	StringConverter(const StringConverter& inst) = delete;
	StringConverter(StringConverter&& inst) = delete;
	StringConverter& operator=(const StringConverter& inst) = delete;
	StringConverter& operator=(StringConverter&& inst) = delete;

	~StringConverter() {}

	const char* convert(const std::wstring& str) {
		const int size = ::WideCharToMultiByte(CP_THREAD_ACP, 0, str.c_str(), -1, nullptr, 0, nullptr, nullptr);
		if (size_ < size) {
			buf_.reset(new char[size]);
			size_ = size;
		}
		// Calculates the number of characters including NULL by specifying -1
		::WideCharToMultiByte(CP_THREAD_ACP, 0, str.c_str(), -1, (LPSTR) buf_.get(), size_, nullptr, nullptr);
		return buf_.get();
	}

};
