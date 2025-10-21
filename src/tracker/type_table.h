/**
 *
 * Table for Managing File Types
 *
 * @author Takuto Yanagida
 * @version 2025-10-21
 *
 */


#pragma once

#include <map>
#include <string>

#include "Pref.hpp"


class TypeTable {

	const std::wstring EXT_SECTION{ L"Extention" };
	const std::wstring EXT_KEY{ L"Ext" };
	const std::wstring COLOR_KEY{ L"Color" };
	const std::wstring EMPTY{ L"" };
	const wchar_t EXT_DIV{ L'|' };

	std::map<std::wstring, int> ext_to_id_;
	std::map<int, int>          id_to_color_;

	int convert_hex_to_color(const std::wstring& hex) {
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
				ro[0] = ro[1] = t[3];
				ro[2] = ro[3] = t[2];
				ro[4] = ro[5] = t[1];
			} else {
				t.append(L"000000");
				ro[0] = t[5]; ro[1] = t[6];
				ro[2] = t[3]; ro[3] = t[4];
				ro[4] = t[1]; ro[5] = t[2];
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

	void restore(Pref& pref) {
		pref.set_current_section(EXT_SECTION);
		ext_to_id_.clear();  // Because it may be called many times
		id_to_color_.clear();

		for (int i = 0; i < 32; ++i) {  // Allow up to 32 extension groups
			auto ext = pref.item(EXT_KEY + std::to_wstring(i + 1), EMPTY);
			if (ext.empty()) continue;

			std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);  // Lower case
			std::wstring::size_type cur = 0, next = 0;
			while ((next = ext.find_first_of(EXT_DIV, cur)) != std::wstring::npos) {
				ext_to_id_[ext.substr(cur, next - cur)] = i;
				cur = next + 1;
			}
			ext_to_id_[ext.substr(cur)] = i;
			auto hex = pref.item(COLOR_KEY + std::to_wstring(i + 1), L"");
			id_to_color_[i] = convert_hex_to_color(hex);
		}
	}

	int get_id(const std::wstring& ext) const {
		const auto it = ext_to_id_.find(ext);
		if (it == ext_to_id_.end()) return -1;
		return it->second;
	}

	int get_color(const std::wstring& ext) const {
		const int id = get_id(ext);
		if (id == -1) return -1;

		const auto it = id_to_color_.find(id);
		if (it == id_to_color_.end()) return -1;
		return it->second;
	}

	int get_color(int id) const {
		const auto it = id_to_color_.find(id);
		if (it == id_to_color_.end()) return -1;
		return it->second;
	}

	bool get_command(const Pref& pref, const std::wstring& ext, std::wstring& cmd) const {
		const int type = get_id(ext) + 1;
		if (type) {
			cmd = pref.item(L"Extention", L"OpenBy" + std::to_wstring(type), L"");
			return !cmd.empty();
		}
		return false;
	}

};
