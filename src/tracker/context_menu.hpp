/**
 * Shell Context Menu
 *
 * @author Takuto Yanagida
 * @version 2025-11-13
 */

#pragma once

#include <vector>
#include <string>

#include <windows.h>
#include <shlobj.h>

#include "gsl/gsl"
#include "shell.hpp"

class ContextMenu {

	static constexpr const wchar_t * const PROP_INSTANCE = L"ContextMenuInstance";

	// Window Procedure that hooked to the original procedure while showing the menu
	static LRESULT CALLBACK MenuProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp) {
		if (wnd == nullptr) {
			return 0;
		}
		auto prop = ::GetProp(wnd, PROP_INSTANCE);
		if (prop != nullptr) {
			auto cm = static_cast<ContextMenu*>(prop);
			if (cm != nullptr) {
				switch (msg) {
				case WM_INITMENUPOPUP:
					if (cm->context_menu_) cm->context_menu_->HandleMenuMsg(msg, wp, lp);
					return FALSE;
				case WM_DRAWITEM:
				case WM_MEASUREITEM:
					if (cm->context_menu_) cm->context_menu_->HandleMenuMsg(msg, wp, lp);
					return TRUE;
				default:
					if (cm->orig_proc_) return CallWindowProc(cm->orig_proc_, wnd, msg, wp, lp);
				}
			}
		}
		return CallWindowProc(DefWindowProc, wnd, msg, wp, lp);
	}

	const HWND wnd_              = nullptr;
	LPCONTEXTMENU2 context_menu_ = nullptr;
	WNDPROC orig_proc_           = nullptr;

public:

	ContextMenu(HWND wnd) noexcept : wnd_(wnd) {}
	ContextMenu(const ContextMenu&) = delete;
	ContextMenu& operator=(const ContextMenu&) = delete;
	ContextMenu(ContextMenu&&) = delete;
	ContextMenu& operator=(ContextMenu&&) = delete;
	~ContextMenu() = default;

	bool popup(const std::vector<std::wstring>& paths, uint32_t flag, const POINT& pt) {
		if (!wnd_) return false;
		auto cm = static_cast<LPCONTEXTMENU>(shell::get_ole_ui_object(paths, IID_IContextMenu));
		if (!cm) return false;

		// Get IContextMenu2
		LPVOID temp = nullptr;
		cm->QueryInterface(IID_IContextMenu2, &temp);
		if (!temp) {
			cm->Release();
			return false;
		}
		LPCONTEXTMENU2 cm2 = static_cast<LPCONTEXTMENU2>(temp);

		// Create a menu
		auto hMenu = ::CreatePopupMenu();
		if (hMenu == nullptr) {
			if (cm2) cm2->Release();
			cm->Release();
			return false;
		}
		cm->QueryContextMenu(hMenu, 0, 1, 0x7fff, CMF_NORMAL);

		// Popup the menu
		::SetProp(wnd_, PROP_INSTANCE, this);
		context_menu_ = cm2;
		[[gsl::suppress(type.1)]]
		orig_proc_ = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(wnd_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(MenuProc)));
		const int id = ::TrackPopupMenu(hMenu, TPM_RETURNCMD | flag, pt.x, pt.y, 0, wnd_, nullptr);
		context_menu_ = nullptr;
		[[gsl::suppress(type.1)]]
		::SetWindowLongPtr(wnd_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(orig_proc_));
		::RemoveProp(wnd_, PROP_INSTANCE);

		bool res = false;
		if (id > 0) res = SUCCEEDED(shell::invoke(cm, MAKEINTRESOURCEA(id - 1))) ? true : false;
		::DestroyMenu(hMenu);
		if (cm2) cm2->Release();
		cm->Release();
		return res;
	}

};
