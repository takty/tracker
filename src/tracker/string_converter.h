/**
 *
 * String Converter
 *
 * @author Takuto Yanagida
 * @version 2021-05-16
 *
 */


#pragma once

#include <string>
#include <vector>
#include <windows.h>


class StringConverter {

public:

	static inline std::string to_ansi(const std::wstring& str) {
		const auto in_len{ str.length() };
		const int out_len = ::WideCharToMultiByte(CP_THREAD_ACP, 0, str.c_str(), in_len, nullptr, 0, nullptr, nullptr);
		std::vector<char> buf(out_len);
		if (out_len) {
			::WideCharToMultiByte(CP_THREAD_ACP, 0, str.c_str(), in_len, buf.data(), out_len, nullptr, nullptr);
		}
		std::string result(buf.begin(), buf.end());
		return result;
	}

	static inline std::wstring to_wide(const std::string& str) {
		const auto in_len{ str.length() };
		const int out_len = ::MultiByteToWideChar(CP_THREAD_ACP, 0, str.c_str(), in_len, nullptr, 0);
		std::vector<wchar_t> buf(out_len);
		if (out_len) {
			::MultiByteToWideChar(CP_THREAD_ACP, 0, str.c_str(), in_len, buf.data(), out_len);
		}
		std::wstring result(buf.begin(), buf.end());
		return result;
	}

};
