/**
 *
 * Popup Menu
 *
 * @author Takuto Yanagida
 * @version 2020-03-22
 *
 */


#pragma once

#include <windows.h>
#include <vector>
#include <string>

#include "Pref.hpp"


class PopupMenu {

	HWND hWnd_ = nullptr;
	std::vector<HMENU> hMenus_;

	const Pref &pref_;

	// Add Menu Items in INI File to Menu
	void addTypeMenu(const wstring &sec, std::vector<wstring> &items, HMENU hMenu) {
		TCHAR key[16];
		bool paste, pasteShortcut;
		wstring def;

		canPaste(paste, pasteShortcut);
		for (int i = 0; i < 32; ++i) {
			wsprintf(key, _T("Name%d"), i + 1);
			auto name = pref_.item(sec, key, def);
			if (name.empty()) continue;
			wsprintf(key, _T("Path%d"), i + 1);
			auto path = pref_.item(sec, key, def);
			if (name.size() == 2 && name[0] == '&') continue;  // Hidden item
			if (i == 0 && items.size() > 0) {  // When connecting to another menu
				::AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
			}
			if (name == _T("-")) {  // Separator
				::AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
			}
			else if (path == _T(">")) {  // Sub-menu
				HMENU hSubMenu = ::CreateMenu();
				hMenus_.push_back(hSubMenu);
				addTypeMenu(sec.substr(1), items, hSubMenu);
				::AppendMenu(hMenu, MF_POPUP, (UINT)hSubMenu, name.c_str());
			}
			else if (path == _T("<New>")) {  // New file menu
				HMENU hSubMenu = ::CreateMenu();
				hMenus_.push_back(hSubMenu);
				addNewFileMenu(hSubMenu, items);
				::AppendMenu(hMenu, MF_POPUP, (UINT)hSubMenu, name.c_str());
			}
			else {  // Normal menu item
				UINT flag = MF_STRING;
				if ((!paste && path == _T("<Paste>")) || (!pasteShortcut && path == _T("<PasteShortcut>"))) {
					flag |= MF_GRAYED;
				}
				items.push_back(path);
				::AppendMenu(hMenu, flag, items.size(), name.c_str());
			}
		}
	}

	// Create new file menu
	void addNewFileMenu(HMENU hMenu, std::vector<wstring> &items) {
		wstring path;

		auto dir = Path::parent(pref_.path()) + L"\\newfile\\";
		FileSystem::find_first_file(dir, [&](const std::wstring& parent, const WIN32_FIND_DATA& wfd) {
			path.assign(L"<CreateNew>").append(parent).append(wfd.cFileName);
			OutputDebugString((path + L'\n').c_str());
			items.push_back(path);
			::AppendMenu(hMenu, MF_STRING, items.size(), wfd.cFileName);
			return true;  // continue
		});
	}

	// Whether you can paste files
	void canPaste(bool &canPaste, bool &canPasteShortcut) {
		canPaste = canPasteShortcut = false;
		if (!::OpenClipboard(hWnd_)) return;
		canPaste = (::GetClipboardData(CF_HDROP) != 0);
		if (canPaste) {
			UINT CF_DROPEFFECT = ::RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT);
			HANDLE hDropEffect = ::GetClipboardData(CF_DROPEFFECT);
			if (hDropEffect) {
				void* ptr = ::GlobalLock(hDropEffect);
				if (ptr != nullptr) {
					DWORD dwEffect = *(DWORD*)ptr;
					::GlobalUnlock(hDropEffect);
					if (dwEffect & DROPEFFECT_LINK) canPasteShortcut = true;
				}
			}
		}
		::CloseClipboard();
	}

	// Search for specified accelerator command from menu item of INI file
	bool searchMenu(const wstring &sec, TCHAR accel, wstring &cmd) {
		TCHAR key[16];
		wstring a, def;

		a.assign(_T("&")).append(1, accel);  // Make search string
		for (int i = 0; i < 32; i++) {
			wsprintf(key, _T("Name%d"), i + 1);
			auto name = pref_.item(sec, key, def);
			if (name.empty()) continue;
			wsprintf(key, _T("Path%d"), i + 1);
			auto path = pref_.item(sec, key, def);
			if (name.find(a) != wstring::npos) {  // Find
				cmd.assign(path);
				return true;
			}
		}
		return false;
	}

public:

	PopupMenu(HWND hWnd, const Pref *pref) : hWnd_(hWnd), pref_(*pref) {}

	// Display pop-up menu and get command
	bool popup(int type, const POINT &pt, UINT f, wstring& cmd, const std::vector<wstring> &additional) {
		TCHAR key[16];
		bool ret = false;
		std::vector<wstring> items;
		HMENU hMenu = ::CreatePopupMenu();  // Create menu
		hMenus_.push_back(hMenu);

		if (type) {  // When a menu number is specified
			wsprintf(key, _T("Menu%d"), type);
			addTypeMenu(key, items, hMenu);
		}
		addTypeMenu(_T("CommonMenu"), items, hMenu);

		if (!additional.empty()) {
			::AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
			for (unsigned int i = 0; i < additional.size(); ++i) {
				::AppendMenu(hMenu, MF_STRING, i, additional[i].c_str());
			}
		}

		if (items.size()) {
			int id = ::TrackPopupMenuEx(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON | f, pt.x, pt.y, hWnd_, nullptr);
			ret = (0 < id);  // -1, 0 if not selected
			if (0 < id && id <= (int)items.size()) {
				cmd.assign(items[id - 1]);  // Ordinary command
			}
		}
		for (unsigned long i = 0; i < hMenus_.size(); ++i) {
			::DestroyMenu(hMenus_[i]);
		}
		return ret;
	}

	// Get command from accelerator
	bool getAccelCommand(int type, TCHAR acce, wstring& cmd) {
		TCHAR key[16];
		bool ret = false;

		if (type) {  // When a menu number is specified
			wsprintf(key, _T("Menu%d"), type);
			ret = searchMenu(key, acce, cmd);
		}
		if (!ret) ret = searchMenu(_T("CommonMenu"), acce, cmd);  // Search from common menu
		return ret;
	}

};