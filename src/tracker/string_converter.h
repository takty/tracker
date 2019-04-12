//
// String Converter
// 2019-04-12
//

#pragma once

#include <windows.h>
#include <tchar.h>
#include <string>

using namespace std;

class StringConverter {

	char* buf_;
	int size_;

public:

	StringConverter() : buf_(nullptr), size_(0) {
	}

	~StringConverter() {
		if(buf_ != nullptr) delete[] buf_;
	}

	const char* convert(const wstring& str) {
		int size = ::WideCharToMultiByte(CP_THREAD_ACP, 0, str.c_str(), -1, nullptr, 0, nullptr, nullptr);
		if(size_ < size) {
			if(buf_ != nullptr) delete[] buf_;
			buf_ = new char[size];
			size_ = size;
		}
		::WideCharToMultiByte(CP_THREAD_ACP, 0, str.c_str(), -1, (LPSTR)buf_, size_, nullptr, nullptr);  // Calculates the number of characters including NULL by specifying -1
		return buf_;
	}

};
