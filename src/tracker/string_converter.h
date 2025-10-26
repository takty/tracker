/**
 * String Converter
 *
 * @author Takuto Yanagida
 * @version 2025-10-24
 */

#pragma once

#include <memory>

#include <windows.h>
#include <tchar.h>
#include <string>

#include "classes.h"

class StringConverter {

	std::shared_ptr<char[]> buf_;
	int size_ = 0;

public:

	StringConverter() noexcept = default;

	StringConverter(const StringConverter&) = delete;
	StringConverter& operator=(const StringConverter&) = delete;
	StringConverter(StringConverter&&) = delete;
	StringConverter& operator=(StringConverter&&) = delete;

	~StringConverter() = default;

	std::shared_ptr<char[]> convert(const std::wstring& str) {
		const int size = ::WideCharToMultiByte(CP_THREAD_ACP, 0, str.c_str(), -1, nullptr, 0, nullptr, nullptr);
		if (size_ < size) {
			buf_  = std::make_shared<char[]>(size);
			size_ = size;
		}
		// Calculates the number of characters including NULL by specifying -1
		::WideCharToMultiByte(CP_THREAD_ACP, 0, str.c_str(), -1, buf_.get(), size_, nullptr, nullptr);
		return buf_;
	}

};
