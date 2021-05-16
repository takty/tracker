/**
 *
 * Popup Menu
 *
 * @author Takuto Yanagida
 * @version 2021-05-16
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
	void addTypeMenu(const std::wstring &sec, std::vector<std::wstring> &items, HMENU hMenu) {
		bool paste, pasteShortcut;
		std::wstring def;

		canPaste(paste, pasteShortcut);
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
				hMenus_.push_back(hSubMenu);
				addTypeMenu(sec.substr(1), items, hSubMenu);
				::AppendMenu(hMenu, MF_POPUP, (UINT)hSubMenu, name.c_str());
			} else if (path == _T("<New>")) {  // New file menu
				HMENU hSubMenu = ::CreateMenu();
				hMenus_.push_back(hSubMenu);
				addNewFileMenu(hSubMenu, items);
				::AppendMenu(hMenu, MF_POPUP, (UINT)hSubMenu, name.c_str());
			} else {  // Normal menu item
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
	void addNewFileMenu(HMENU hMenu, std::vector<std::wstring> &items) {
		std::wstring path;

		auto dir = Path::parent(pref_.path()) + L"\\newfile\\";
		FileSystem::find_first_file(dir, [&](const std::wstring& parent, const WIN32_FIND_DATA& wfd) {
			path.assign(L"<CreateNew>").append(parent).append(&wfd.cFileName[0]);
			OutputDebugString((path + L'\n').c_str());
			items.push_back(path);
			::AppendMenu(hMenu, MF_STRING, items.size(), &wfd.cFileName[0]);
			return true;  // continue
		});
	}

	// Whether you can paste files
	void canPaste(bool &canPaste, bool &canPasteShortcut) noexcept {
		canPaste = canPasteShortcut = false;
		if (!::OpenClipboard(hWnd_)) return;
		canPaste = (::GetClipboardData(CF_HDROP) != 0);
		if (canPaste) {
			const UINT CF_DROPEFFECT = ::RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT);
			HANDLE hDropEffect = ::GetClipboardData(CF_DROPEFFECT);
			if (hDropEffect) {
				const void* ptr = ::GlobalLock(hDropEffect);
				if (ptr != nullptr) {
					const DWORD dwEffect = *(DWORD*)ptr;
					::GlobalUnlock(hDropEffect);
					if (dwEffect & DROPEFFECT_LINK) canPasteShortcut = true;
				}
			}
		}
		::CloseClipboard();
	}

	// Search for specified accelerator command from menu item of INI file
	bool searchMenu(const std::wstring &sec, wchar_t accel, std::wstring &cmd) {
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

	PopupMenu(HWND hWnd, const Pref *pref) noexcept : hWnd_(hWnd), pref_(*pref) {}

	// Display pop-up menu and get command
	bool popup(int type, const POINT &pt, UINT f, std::wstring& cmd, const std::vector<std::wstring> &additional) {
		bool ret = false;
		std::vector<std::wstring> items;
		HMENU hMenu = ::CreatePopupMenu();  // Create menu
		hMenus_.push_back(hMenu);

		if (type) {  // When a menu number is specified
			addTypeMenu(L"Menu" + std::to_wstring(type), items, hMenu);
		}
		addTypeMenu(_T("CommonMenu"), items, hMenu);

		if (!additional.empty()) {
			::AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
			for (size_t i = 0; i < additional.size(); ++i) {
				::AppendMenu(hMenu, MF_STRING, i, &(additional.at(i).c_str())[0]);
			}
		}

		if (items.size()) {
			const int id = ::TrackPopupMenuEx(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON | f, pt.x, pt.y, hWnd_, nullptr);
			ret = (0 < id);  // -1, 0 if not selected
			int max{ static_cast<int>(std::forward<size_t>(items.size())) };
			if (0 < id && id <= max) {
				cmd.assign(items.at(id - 1));  // Ordinary command
			}
		}
		for (const auto& m : hMenus_) {
			::DestroyMenu(m);
		}
		return ret;
	}

	// Get command from accelerator
	bool getAccelCommand(int type, wchar_t acce, std::wstring& cmd) {
		bool ret = false;

		if (type) {  // When a menu number is specified
			ret = searchMenu(L"Menu" + std::to_wstring(type), acce, cmd);
		}
		if (!ret) ret = searchMenu(_T("CommonMenu"), acce, cmd);  // Search from common menu
		return ret;
	}

};