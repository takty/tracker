/**
 *
 * String Converter
 *
 * @author Takuto Yanagida
 * @version 2020-03-22
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

	StringConverter() {
	}

	~StringConverter() {
		if (buf_ != nullptr) delete[] buf_;
	}

	const char* convert(const std::wstring& str) {
		int size = ::WideCharToMultiByte(CP_THREAD_ACP, 0, str.c_str(), -1, nullptr, 0, nullptr, nullptr);
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
