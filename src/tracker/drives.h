/**
 *
 * Drives
 *
 * @author Takuto Yanagida
 * @version 2025-10-24
 *
 */


#pragma once

#include <vector>
#include <string>

#include <windows.h>

#include "file_utils.hpp"


class Drives {

	static const wchar_t FIRST_LETTER = L'A';
	static const wchar_t LAST_LETTER  = L'Z';
	static const int WAITING_TIME     = 200;

	std::vector<std::wstring> paths_;
	std::vector<bool> slowDrives_;

public:

	const std::wstring PATH{ L":DRIVES" }, NAME{ L"Drives" };

	Drives() noexcept : slowDrives_(LAST_LETTER - FIRST_LETTER + 1, false) {}

	size_t size() noexcept {
		return paths_.size();
	}

	std::wstring& operator[](size_t index) {
		return paths_.at(index);
	}

	void clean_up() {
		paths_.clear();
		std::wstring path(L"A:\\");

		for (wchar_t c = FIRST_LETTER; c <= LAST_LETTER; ++c) {
			const auto idx = c - FIRST_LETTER;
			path.at(path.size() - 3) = c;
			const auto type = ::GetDriveType(path.c_str());
			if (type == DRIVE_NO_ROOT_DIR) continue;
			if (type == DRIVE_REMOVABLE) {
				if (!slowDrives_.at(idx)) {
					const auto startTime = ::GetTickCount64();
					const bool can = (::GetDiskFreeSpace(path.c_str(), nullptr, nullptr, nullptr, nullptr) != 0);
					if (::GetTickCount64() - startTime > WAITING_TIME) {  // If it takes time
						slowDrives_.at(idx) = true;
					} else {
						if (!can) continue;
					}
				}
			} else {
				if (!::GetDiskFreeSpace(path.c_str(), nullptr, nullptr, nullptr, nullptr)) continue;
			}
			paths_.push_back(path);
		}
	}

};
