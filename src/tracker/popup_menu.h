/**
 * Popup Menu
 *
 * @author Takuto Yanagida
 * @version 2025-11-19
 */

#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include <format>

#include "gsl/gsl"
#include "pref.hpp"

class PopupMenu {

	HWND hwnd_ = nullptr;
	std::vector<HMENU> hmenus_;

	const Pref &pref_;

	// Add Menu Items in INI File to Menu
	void add_type_menu(const std::wstring &sec, std::vector<std::wstring> &items, HMENU hmenu) {
		bool paste, paste_shortcut;
		std::wstring def;

		can_paste(paste, paste_shortcut);
		for (int i = 0; i < 32; ++i) {
			auto name = pref_.item(sec, std::format(L"Name{}", i + 1), def);
			if (name.empty()) continue;

			auto path = pref_.item(sec, std::format(L"Path{}", i + 1), def);
			if (name.size() == 2 && name.front() == '&') continue;  // Hidden item

			if (i == 0 && items.size() > 0) {  // When connecting to another menu
				::AppendMenu(hmenu, MF_SEPARATOR, 0, nullptr);
			}
			if (name == _T("-")) {  // Separator
				::AppendMenu(hmenu, MF_SEPARATOR, 0, nullptr);
			} else if (path == _T(">")) {  // Sub-menu
				HMENU hsub_menu = ::CreateMenu();
				hmenus_.push_back(hsub_menu);
				add_type_menu(sec.substr(1), items, hsub_menu);

				[[gsl::suppress(type.1)]]
				::AppendMenu(hmenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hsub_menu), name.c_str());
			} else if (path == _T("<New>")) {  // New file menu
				HMENU hsub_menu = ::CreateMenu();
				hmenus_.push_back(hsub_menu);
				add_new_file_menu(hsub_menu, items);

				[[gsl::suppress(type.1)]]
				::AppendMenu(hmenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hsub_menu), name.c_str());
			} else {  // Normal menu item
				UINT flag = MF_STRING;
				if ((!paste && path == _T("<Paste>")) || (!paste_shortcut && path == _T("<PasteShortcut>"))) {
					flag |= MF_GRAYED;
				}
				items.push_back(path);
				::AppendMenu(hmenu, flag, items.size(), name.c_str());
			}
		}
	}

	// Create new file menu
	void add_new_file_menu(HMENU hMenu, std::vector<std::wstring> &items) {
		std::wstring path;

		const auto dir = path::parent(pref_.path()) + L"\\newfile\\";
		file_system::find_first_file(dir, [&](const std::wstring& parent, const WIN32_FIND_DATA& wfd) {
			path.assign(L"<CreateNew>").append(parent).append(&wfd.cFileName[0]);
			items.push_back(path);
			::AppendMenu(hMenu, MF_STRING, items.size(), &wfd.cFileName[0]);
			return true;  // continue
		});
	}

	// Whether you can paste files
	void can_paste(bool &can_paste, bool &can_paste_shortcut) const noexcept {
		can_paste = can_paste_shortcut = false;
		if (!::OpenClipboard(hwnd_)) return;
		can_paste = (::GetClipboardData(CF_HDROP) != nullptr);
		if (can_paste) {
			const UINT CF_DROPEFFECT = ::RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT);
			const HANDLE hde         = ::GetClipboardData(CF_DROPEFFECT);
			if (hde) {
				void* ptr = ::GlobalLock(hde);
				if (ptr != nullptr) {
					const DWORD e = *static_cast<DWORD*>(ptr);
					::GlobalUnlock(hde);
					if (e & DROPEFFECT_LINK) can_paste_shortcut = true;
				}
			}
		}
		::CloseClipboard();
	}

	// Search for specified accelerator command from menu item of INI file
	bool search_menu(const std::wstring &sec, TCHAR accel, std::wstring &cmd) {
		std::wstring a, def;

		a.assign(_T("&")).append(1, accel);  // Make search string
		for (int i = 0; i < 32; i++) {
			auto name = pref_.item(sec, std::format(L"Name{}", i + 1), def);
			if (name.empty()) continue;

			auto path = pref_.item(sec, std::format(L"Path{}", i + 1), def);
			if (name.find(a) != std::wstring::npos) {  // Find
				cmd.assign(path);
				return true;
			}
		}
		return false;
	}

public:

	PopupMenu(HWND hWnd, const Pref* pref) noexcept : hwnd_(hWnd), pref_(*pref) {}

	// Display pop-up menu and get command
	bool popup(int type, const POINT &pt, UINT f, std::wstring& cmd, const std::vector<std::wstring> &additional) {
		bool ret = false;
		std::vector<std::wstring> items;
		HMENU hmenu = ::CreatePopupMenu();  // Create menu
		if (hmenu == nullptr) {
			return false;
		}
		hmenus_.push_back(hmenu);

		if (type) {  // When a menu number is specified
			add_type_menu(std::format(L"Menu{}", type), items, hmenu);
		}
		add_type_menu(_T("CommonMenu"), items, hmenu);

		if (!additional.empty()) {
			::AppendMenu(hmenu, MF_SEPARATOR, 0, nullptr);
			for (size_t i = 0; i < additional.size(); ++i) {
				::AppendMenu(hmenu, MF_STRING, i, additional.at(i).c_str());
			}
		}

		if (items.size()) {
			const int id = ::TrackPopupMenuEx(hmenu, TPM_RETURNCMD | TPM_RIGHTBUTTON | f, pt.x, pt.y, hwnd_, nullptr);
			ret = (0 < id);  // -1, 0 if not selected
			if (0 < id) {
				const auto idx = gsl::narrow<size_t>(id);
				if (idx <= items.size()) {
					cmd.assign(items.at(idx - 1));  // Ordinary command
				}
			}
		}
		for (const auto& m : hmenus_) {
			::DestroyMenu(m);
		}
		return ret;
	}

	// Get command from accelerator
	bool get_accel_command(int type, TCHAR acce, std::wstring& cmd) {
		bool ret = false;

		if (type) {  // When a menu number is specified
			ret = search_menu(std::format(L"Menu{}", type), acce, cmd);
		}
		if (!ret) ret = search_menu(_T("CommonMenu"), acce, cmd);  // Search from common menu
		return ret;
	}

};
