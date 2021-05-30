/**
 *
 * Popup Menu
 *
 * @author Takuto Yanagida
 * @version 2021-05-30
 *
 */


#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include <iterator>

#include "pref.hpp"


class PopupMenu {

	HWND hwnd_ = nullptr;
	std::vector<HMENU> hmenus_;

	const Pref &pref_;

	// Add Menu Items in INI File to Menu
	void add_type_menu(const std::wstring &sec, std::vector<std::wstring> &items, HMENU hMenu) noexcept(false) {
		bool paste, paste_shortcut;
		std::wstring def;

		can_paste(paste, paste_shortcut);
		for (int i = 0; i < 32; ++i) {
			auto name = pref_.get(sec, L"Name" + std::to_wstring(i + 1), def);
			if (name.empty()) continue;

			auto path = pref_.get(sec, L"Path" + std::to_wstring(i + 1), def);
			if (name.size() == 2 && name.at(0) == '&') continue;  // Hidden item

			if (i == 0 && items.size() > 0) {  // When connecting to another menu
				::AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
			}
			if (name == _T("-")) {  // Separator
				::AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
			} else if (path == _T(">")) {  // Sub-menu
				HMENU hSubMenu = ::CreateMenu();
				hmenus_.push_back(hSubMenu);
				add_type_menu(sec.substr(1), items, hSubMenu);
				::AppendMenu(hMenu, MF_POPUP, (UINT)hSubMenu, name.c_str());
			} else if (path == _T("<New>")) {  // New file menu
				HMENU hSubMenu = ::CreateMenu();
				hmenus_.push_back(hSubMenu);
				add_new_file_menu(hSubMenu, items);
				::AppendMenu(hMenu, MF_POPUP, (UINT)hSubMenu, name.c_str());
			} else {  // Normal menu item
				UINT flag = MF_STRING;
				if ((!paste && path == _T("<Paste>")) || (!paste_shortcut && path == _T("<PasteShortcut>"))) {
					flag |= MF_GRAYED;
				}
				items.push_back(path);
				::AppendMenu(hMenu, flag, items.size(), name.c_str());
			}
		}
	}

	// Create new file menu
	void add_new_file_menu(HMENU hmenu, std::vector<std::wstring> &items) noexcept {
		std::wstring path;

		auto dir = Path::parent(pref_.path()) + L"\\newfile\\";
		FileSystem::find_first_file(dir, [&](const std::wstring& parent, const WIN32_FIND_DATA& wfd) {
			path.assign(L"<CreateNew>").append(parent).append(&wfd.cFileName[0]);
			OutputDebugString((path + L'\n').c_str());
			items.push_back(path);
			::AppendMenu(hmenu, MF_STRING, items.size(), &wfd.cFileName[0]);
			return true;  // continue
		});
	}

	// Whether you can paste files
	void can_paste(bool &can_paste, bool &can_paste_shortcut) noexcept {
		can_paste = can_paste_shortcut = false;
		if (!::OpenClipboard(hwnd_)) return;

		can_paste = (::GetClipboardData(CF_HDROP) != 0);
		if (can_paste) {
			const UINT CF_DROPEFFECT = ::RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT);
			HANDLE hDropEffect = ::GetClipboardData(CF_DROPEFFECT);
			if (hDropEffect) {
				const void* ptr = ::GlobalLock(hDropEffect);
				if (ptr != nullptr) {
					const DWORD dwEffect = *(DWORD*)ptr;
					::GlobalUnlock(hDropEffect);
					if (dwEffect & DROPEFFECT_LINK) can_paste_shortcut = true;
				}
			}
		}
		::CloseClipboard();
	}

	// Search for specified accelerator command from menu item of INI file
	bool search_menu(const std::wstring &sec, wchar_t accel, std::wstring &cmd) noexcept {
		std::wstring a, def;

		a.assign(_T("&")).append(1, accel);  // Make search string
		for (int i = 0; i < 32; i++) {
			auto name = pref_.get(sec, L"Name" + std::to_wstring(i + 1), def);
			if (name.empty()) continue;
			auto path = pref_.get(sec, L"Path" + std::to_wstring(i + 1), def);
			if (name.find(a) != std::wstring::npos) {  // Find
				cmd.assign(path);
				return true;
			}
		}
		return false;
	}

public:

	PopupMenu(HWND hwnd, const Pref *pref) noexcept : hwnd_(hwnd), pref_(*pref) {}

	// Display pop-up menu and get command
	bool popup(int type, const POINT &pt, UINT f, std::wstring& cmd, const std::vector<std::wstring> &additional) noexcept {
		bool ret = false;
		std::vector<std::wstring> items;
		HMENU hMenu = ::CreatePopupMenu();  // Create menu
		hmenus_.push_back(hMenu);

		if (type) {  // When a menu number is specified
			add_type_menu(L"Menu" + std::to_wstring(type), items, hMenu);
		}
		add_type_menu(_T("CommonMenu"), items, hMenu);

		if (!additional.empty()) {
			::AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
			for (size_t i = 0; i < additional.size(); ++i) {
				::AppendMenu(hMenu, MF_STRING, i, &(additional.at(i).c_str())[0]);
			}
		}

		if (items.size()) {
			const int id = ::TrackPopupMenuEx(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON | f, pt.x, pt.y, hwnd_, nullptr);
			ret = (0 < id);  // -1, 0 if not selected
			const int max{ std::ssize(items) };
			if (0 < id && id <= max) {
				cmd.assign(items.at(id - 1));  // Ordinary command
			}
		}
		for (const auto& m : hmenus_) {
			::DestroyMenu(m);
		}
		return ret;
	}

	// Get command from accelerator
	bool get_accel_command(int type, wchar_t acce, std::wstring& cmd) noexcept {
		bool ret = false;

		if (type) {  // When a menu number is specified
			ret = search_menu(L"Menu" + std::to_wstring(type), acce, cmd);
		}
		if (!ret) {
			ret = search_menu(_T("CommonMenu"), acce, cmd);  // Search from common menu
		}
		return ret;
	}

};
