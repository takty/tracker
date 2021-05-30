/**
 *
 * Drives
 *
 * @author Takuto Yanagida
 * @version 2021-05-30
 *
 */


#pragma once

#include <vector>
#include <array>
#include <string>
#include <windows.h>

#include "file_utils.hpp"


class Drives {

	static const wchar_t FIRST_LETTER = L'A';
	static const wchar_t LAST_LETTER  = L'Z';
	static const int WAITING_TIME     = 200;

	std::vector<std::wstring> paths_;
	std::array<bool, LAST_LETTER - FIRST_LETTER + 1> slow_drives_{};

public:

	const std::wstring PATH{ L":DRIVES" }, NAME{ L"Drives" };

	Drives() noexcept {}

	// ---------------------------------------------------------------------------

	auto size() const noexcept {
		return paths_.size();
	}

	auto at(size_t index) noexcept {
		return paths_.at(index);
	}

	auto begin() noexcept {
		return paths_.begin();
	}

	auto end() noexcept {
		return paths_.end();
	}

	// ---------------------------------------------------------------------------

	void clean_up() noexcept {
		paths_.clear();
		std::wstring path{ L"\\\\?\\A:\\" };

		for (wchar_t i = 0; i <= LAST_LETTER - FIRST_LETTER; ++i) {
			path.at(path.size() - 3) = FIRST_LETTER + i;
			const auto type = ::GetDriveType(path.c_str());
			if (type == DRIVE_NO_ROOT_DIR) continue;
			if (type == DRIVE_REMOVABLE) {
				if (!slow_drives_.at(i)) {
					const auto startTime = ::GetTickCount64();
					const bool can = (::GetDiskFreeSpace(path.c_str(), nullptr, nullptr, nullptr, nullptr) != 0);
					if (::GetTickCount64() - startTime > WAITING_TIME) {  // If it takes time
						slow_drives_.at(i) = true;
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
