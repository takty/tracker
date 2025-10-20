/**
 *
 * Drives
 *
 * @author Takuto Yanagida
 * @version 2025-10-20
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
	bool slowDrives_[LAST_LETTER - FIRST_LETTER + 1];

public:

	const std::wstring PATH{ L":DRIVES" }, NAME{ L"Drives" };

	Drives() : slowDrives_() {}

	int size() {
		return paths_.size();
	}

	std::wstring& operator[](int index) {
		return paths_[index];
	}

	void clean_up() {
		paths_.clear();
		std::wstring path(L"A:\\");

		for (wchar_t c = FIRST_LETTER; c <= LAST_LETTER; ++c) {
			path[path.size() - 3] = c;
			auto type = ::GetDriveType(path.c_str());
			if (type == DRIVE_NO_ROOT_DIR) continue;
			if (type == DRIVE_REMOVABLE) {
				if (!slowDrives_[c - FIRST_LETTER]) {
					auto startTime = ::GetTickCount64();
					bool can = (::GetDiskFreeSpace(path.c_str(), nullptr, nullptr, nullptr, nullptr) != 0);
					if (::GetTickCount64() - startTime > WAITING_TIME) {  // If it takes time
						slowDrives_[c - FIRST_LETTER] = true;
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
