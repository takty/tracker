#pragma once

#include <map>
#include <string>

#include "Pref.hpp"


//
// Table for Managing File Types
// 2019-05-19
//

class TypeTable {

	const std::wstring EXT_SECTION{ L"Extention" };
	const std::wstring EXT_KEY{ L"Ext" };
	const std::wstring COLOR_KEY{ L"Color" };
	const std::wstring EMPTY{ L"" };
	const wchar_t EXT_DIV{ L'|' };

	std::map<std::wstring, int> ext_to_id_;
	std::map<int, int> id_to_color_;

public:

	TypeTable() {}

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
			id_to_color_[i] = pref.item_int(COLOR_KEY + std::to_wstring(i + 1), -1);
		}
	}

	int get_id(const std::wstring& ext) const {
		auto it = ext_to_id_.find(ext);
		if (it == ext_to_id_.end()) return -1;
		return it->second;
	}

	int get_color(const std::wstring& ext) const {
		int id = get_id(ext);
		if (id == -1) return -1;

		auto it = id_to_color_.find(id);
		if (it == id_to_color_.end()) return -1;
		return it->second;
	}

	int get_color(int id) const {
		auto it = id_to_color_.find(id);
		if (it == id_to_color_.end()) return -1;
		return it->second;
	}

	bool get_command(const Pref& pref, const std::wstring& ext, std::wstring& cmd) const {
		int type = get_id(ext) + 1;
		if (type) {
			cmd = pref.item(L"Extention", L"OpenBy" + std::to_wstring(type), L"");
			return !cmd.empty();
		}
		return false;
	}

};
