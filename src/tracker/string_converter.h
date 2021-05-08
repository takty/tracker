/**
 *
 * String Converter
 *
 * @author Takuto Yanagida
 * @version 2021-05-09
 *
 */


#pragma once

#include <windows.h>
#include <string>


class StringConverter {

public:

	static inline std::string to_ansi(const std::wstring& str) {
		int in_len = (int)str.length();
		int out_len = ::WideCharToMultiByte(CP_THREAD_ACP, 0, str.c_str(), in_len, nullptr, 0, nullptr, nullptr);
		std::vector<char> buf(out_len);
		if (out_len) {
			::WideCharToMultiByte(CP_THREAD_ACP, 0, str.c_str(), in_len, &buf[0], out_len, nullptr, nullptr);
		}
		std::string result(buf.begin(), buf.end());
		return result;
	}

	static inline std::wstring to_wide(const std::string& str) {
		int in_len = (int)str.length();
		int out_len = ::MultiByteToWideChar(CP_THREAD_ACP, 0, str.c_str(), in_len, nullptr, 0);
		std::vector<wchar_t> buf(out_len);
		if (out_len) {
			::MultiByteToWideChar(CP_THREAD_ACP, 0, str.c_str(), in_len, &buf[0], out_len);
		}
		std::wstring result(buf.begin(), buf.end());
		return result;
	}

};
