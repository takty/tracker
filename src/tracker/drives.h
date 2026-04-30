/**
 * Drives
 *
 * @author Takuto Yanagida
 * @version 2026-04-30
 */

#pragma once

#include <vector>
#include <string>

#include <windows.h>

#include "file_utils.hpp"

class Drives {

	inline static const wchar_t FIRST_LETTER = L'A';
	inline static const wchar_t LAST_LETTER  = L'Z';
	inline static const int WAITING_TIME     = 200;

	std::vector<std::wstring> paths_;
	std::vector<bool> slow_drives_;

public:

	inline static const std::wstring PATH{ L":DRIVES" }, NAME{ L"Drives" };

	Drives() noexcept : slow_drives_(LAST_LETTER - FIRST_LETTER + 1, false) {}

	size_t size() const noexcept {
		return paths_.size();
	}

	std::wstring& operator[](size_t index) {
		return paths_.at(index);
	}

	void clean_up() {
		paths_.clear();
		std::wstring path(L"A:\\");

		auto can_access = [&]() noexcept {
			return ::GetDiskFreeSpace(path.c_str(), nullptr, nullptr, nullptr, nullptr) != 0;
		};

		for (wchar_t c = FIRST_LETTER; c <= LAST_LETTER; ++c) {
			const auto idx = c - FIRST_LETTER;
			path.at(path.size() - 3) = c;
			const auto type = ::GetDriveType(path.c_str());
			if (type == DRIVE_NO_ROOT_DIR) continue;
			if (type == DRIVE_REMOVABLE) {
				if (!slow_drives_.at(idx)) {
					const auto start_time = ::GetTickCount64();
					const bool can = can_access();
					if (::GetTickCount64() - start_time > WAITING_TIME) {
						slow_drives_.at(idx) = true;
					} else if (!can) {
						continue;
					}
				}
			} else if (!can_access()) {
				continue;
			}
			paths_.emplace_back(path);
		}
	}

};
