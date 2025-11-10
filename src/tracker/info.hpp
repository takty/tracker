/**
 * File Informations
 *
 * @author Takuto Yanagida
 * @version 2025-11-10
 */

#pragma once

#include <string>
#include <vector>

#include <windows.h>
#include <shlobj.h>

#include "gsl/gsl"

namespace info {

	std::wstring fmt(long long l) {
		std::wstring s;
		for (int d = 0; l; d++, l /= 10) {
			s = std::to_wstring(L'0' + (l % 10)) + (((!(d % 3) && d) ? L"," : L"") + s);
		}
		return s;
	}

	// Generate a string representing the file's timestamp
	std::wstring& file_time_to_str(const FILETIME& time, const wchar_t* prefix, std::wstring& dest) {
		FILETIME local;
		SYSTEMTIME st;

		::FileTimeToLocalFileTime(&time, &local);
		::FileTimeToSystemTime(&local, &st);

		wchar_t temp[100]{};  // pre + 19 characters
		swprintf_s(&temp[0], 100, L"%s%d-%02d-%02d (%02d:%02d:%02d)", prefix, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
		dest.assign(&temp[0]);
		return dest;
	}

	// Generate a string representing the size of the file or drive
	std::wstring file_size_to_str(const uint64_t& size, bool success, const wchar_t* prefix) {
		std::wstring dest;
		std::wstring u;
		bool ab{ false };
		double val{};

		if (size >= (1 << 30)) {
			val = static_cast<double>(size) / (1ULL << 30);
			u = L" GB";
		} else if (size >= (1 << 20)) {
			val = static_cast<double>(size) / (1ULL << 20);
			u = L" MB";
			ab = true;
		} else if (size >= (1 << 10)) {
			val = static_cast<double>(size) / (1ULL << 10);
			u = L" kB";
			ab = true;
		} else {
			val = static_cast<double>(size) / (1ULL << 0);
			u = L" Bytes";
		}

		int pre = 0;
		if (val < 100) ++pre;
		if (val < 10) ++pre;
		if (val < 1) ++pre;

		wchar_t format[100]{}, temp[100]{};
		swprintf_s(&format[0], 100, L"%s%s%%.%dlf%s", prefix, (!success ? L">" : L""), pre, u.c_str());
		swprintf_s(&temp[0], 100, &format[0], val);
		dest.assign(&temp[0]);
		if (ab) {
			dest.append(L" (").append(fmt(size)).append(L" Bytes)");
		}
		return dest;
	}

};
