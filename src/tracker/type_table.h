/**
 * Table for Managing File Types
 *
 * @author Takuto Yanagida
 * @version 2025-11-10
 */

#pragma once

#include <map>
#include <string>

#include "pref.hpp"

class TypeTable {

	inline static const std::wstring EXT_SECTION{ L"Extension" };
	inline static const std::wstring EXT_KEY{ L"Ext" };
	inline static const std::wstring COLOR_KEY{ L"Color" };
	inline static const std::wstring EMPTY{ L"" };
	inline static const wchar_t EXT_DIV{ L'|' };

	std::map<std::wstring, int> ext_to_id_;
	std::map<int, int>          id_to_color_;

	int convert_hex_to_color(const std::wstring& s) {
		if (s.empty()) return -1;
		if (s.front() == L'#') {
			std::wstring t(s.begin() + 1, s.end());
			if (t.size() == 3) t = { t.at(0),t.at(0), t.at(1),t.at(1), t.at(2),t.at(2) };
			if (t.size() != 6) return -1;
			try {
				const int rgb = std::stoi(t, nullptr, 16);
				return ((rgb & 0xFF) << 16) | (rgb & 0xFF00) | ((rgb >> 16) & 0xFF);
			} catch (...) { return -1; }
		}
		if (s.rfind(L"0x", 0) == 0 || s.rfind(L"0X", 0) == 0) {
			try { return std::stoi(std::wstring{s.begin() + 2, s.end()}, nullptr, 16); }
			catch (...) { return -1; }
		}
		if (s.size() == 6) {
			try {
				const int rgb = std::stoi(std::wstring{s}, nullptr, 16);
				return ((rgb & 0xFF) << 16) | (rgb & 0xFF00) | ((rgb >> 16) & 0xFF);
			} catch (...) { return -1; }
		}
		return -1;
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
			cmd = pref.item(L"Extension", L"OpenBy" + std::to_wstring(type), L"");
			return !cmd.empty();
		}
		return false;
	}

};
