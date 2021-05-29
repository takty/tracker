/**
 *
 * Table for Managing File Types
 *
 * @author Takuto Yanagida
 * @version 2021-05-29
 *
 */


#pragma once

#include <map>
#include <string>

#include "pref.hpp"


class TypeTable {

	const std::wstring EXT_SECTION{ L"Extension" };
	const std::wstring EXT_KEY{ L"Ext" };
	const std::wstring COLOR_KEY{ L"Color" };
	const std::wstring EMPTY{ L"" };
	const wchar_t EXT_DIV{ L'|' };

	std::map<std::wstring, int> ext_to_id_;
	std::map<int, int>          id_to_color_;

	int convert_hex_to_color(const std::wstring& hex) noexcept {
		if (hex.empty()) return -1;
		const std::wstring pre0{ L"0x" };
		const std::wstring pre1{ L"#" };
		std::wstring ro;
		if (hex.compare(0, pre0.size(), pre0) == 0) {
			ro.assign(hex);
		} else if (hex.compare(0, pre1.size(), pre1) == 0) {
			ro.assign(L"000000");
			std::wstring t{ hex };
			if (hex.size() <= 4) {
				t.append(L"000");
				ro.at(0) = ro.at(1) = t.at(3);
				ro.at(2) = ro.at(3) = t.at(2);
				ro.at(4) = ro.at(5) = t.at(1);
			} else {
				t.append(L"000000");
				ro.at(0) = t.at(5); ro.at(1) = t.at(6);
				ro.at(2) = t.at(3); ro.at(3) = t.at(4);
				ro.at(4) = t.at(1); ro.at(5) = t.at(2);
			}
		}
		try {
			return std::stoi(ro, nullptr, 16);
		} catch (...) {
			return -1;
		}
	}

public:

	TypeTable() noexcept {}

	void restore(const Pref& pref) noexcept(false) {
		ext_to_id_.clear();  // Because it may be called many times
		id_to_color_.clear();

		for (int i = 0; i < 32; ++i) {  // Allow up to 32 extension groups
			auto ext = pref.get(EXT_SECTION, EXT_KEY + std::to_wstring(i + 1), EMPTY);
			if (ext.empty()) continue;

			std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);  // Lower case
			std::wstring::size_type cur = 0, next = 0;
			while ((next = ext.find_first_of(EXT_DIV, cur)) != std::wstring::npos) {
				ext_to_id_[ext.substr(cur, next - cur)] = i;
				cur = next + 1;
			}
			ext_to_id_[ext.substr(cur)] = i;
			auto hex = pref.get(EXT_SECTION, COLOR_KEY + std::to_wstring(i + 1), L"");
			id_to_color_[i] = convert_hex_to_color(hex);
		}
	}

	int get_id(const std::wstring& ext) const noexcept(false) {
		const auto it = ext_to_id_.find(ext);
		if (it == ext_to_id_.end()) return -1;
		return it->second;
	}

	int get_color(const std::wstring& ext) const noexcept(false) {
		const int id = get_id(ext);
		if (id == -1) return -1;

		const auto it = id_to_color_.find(id);
		if (it == id_to_color_.end()) return -1;
		return it->second;
	}

	int get_color(int id) const noexcept(false) {
		const auto it = id_to_color_.find(id);
		if (it == id_to_color_.end()) return -1;
		return it->second;
	}

	bool get_command(const Pref& pref, const std::wstring& ext, std::wstring& cmd) const noexcept(false) {
		const int type = get_id(ext) + 1;
		if (type) {
			cmd = pref.get(L"Extension", L"OpenBy" + std::to_wstring(type), L"");
			return !cmd.empty();
		}
		return false;
	}

};
