/**
 *
 * Shell Context Menu
 *
 * @author Takuto Yanagida
 * @version 2025-10-21
 *
 */


#pragma once

#include <vector>
#include <string>

#include <windows.h>
#include <shlobj.h>

#include "Shell.hpp"


class ContextMenu {

	static constexpr const wchar_t * const PROP_INSTANCE = L"ContextMenuInstance";

	// Window Procedure that hooked to the original procedure while showing the menu
	static LRESULT CALLBACK MenuProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp) {
		const auto cm = (ContextMenu*)::GetProp(wnd, PROP_INSTANCE);
		switch (msg) {
		case WM_INITMENUPOPUP:
			cm->context_menu_->HandleMenuMsg(msg, wp, lp);
			return FALSE;
		case WM_DRAWITEM:
		case WM_MEASUREITEM:
			cm->context_menu_->HandleMenuMsg(msg, wp, lp);
			return TRUE;
		default:
			return CallWindowProc(cm->orig_proc_, wnd, msg, wp, lp);
		}
	}

	const HWND wnd_              = nullptr;
	LPCONTEXTMENU2 context_menu_ = nullptr;
	WNDPROC orig_proc_           = nullptr;

	HRESULT invoke(const LPCONTEXTMENU cm, const char* cmd) const {
		CMINVOKECOMMANDINFO ici;
		ici.cbSize       = sizeof(ici);
		ici.fMask        = 0;
		ici.hwnd         = nullptr;
		ici.lpVerb       = cmd;
		ici.lpParameters = nullptr;
		ici.lpDirectory  = nullptr;
		ici.nShow        = SW_SHOWNORMAL;
		return cm->InvokeCommand(&ici);
	}

	HRESULT execute(const std::vector<std::wstring>& paths, const char* cmd) const {
		auto cm = (LPCONTEXTMENU)Shell::get_ole_ui_object(paths, IID_IContextMenu);
		if (!cm) return 0;

		auto res = invoke(cm, cmd);
		cm->Release();
		return res;
	}

public:

	ContextMenu(HWND wnd) : wnd_(wnd) {}

	~ContextMenu() {}

	// Popup shell context menu
	bool popup(const std::vector<std::wstring>& paths, uint32_t flag, const POINT& pt) {
		auto cm = (LPCONTEXTMENU)Shell::get_ole_ui_object(paths, IID_IContextMenu);
		if (!cm) return false;

		// Get IContextMenu2
		LPCONTEXTMENU2 cm2 = nullptr;
		cm->QueryInterface(IID_IContextMenu2, (void**)&cm2);

		// Create a menu
		auto hMenu = ::CreatePopupMenu();
		cm->QueryContextMenu(hMenu, 0, 1, 0x7fff, CMF_NORMAL);

		// Popup the menu
		::SetProp(wnd_, PROP_INSTANCE, this);
		context_menu_ = cm2;
		orig_proc_ = (WNDPROC)::SetWindowLongPtr(wnd_, GWLP_WNDPROC, (LONG_PTR)MenuProc);
		auto id = ::TrackPopupMenu(hMenu, TPM_RETURNCMD | flag, pt.x, pt.y, 0, wnd_, nullptr);
		context_menu_ = nullptr;
		::SetWindowLongPtr(wnd_, GWLP_WNDPROC, (LONG_PTR)orig_proc_);
		::RemoveProp(wnd_, PROP_INSTANCE);

		bool res = false;
		if (id > 0) res = SUCCEEDED(invoke(cm, MAKEINTRESOURCEA(id - 1)));
		::DestroyMenu(hMenu);
		if (cm2) cm2->Release();
		cm->Release();
		return res;
	}

	// Open files by context menu command
	void open(const std::vector<std::wstring>& paths) const {
		execute(paths, "open");
	}

	// Copy files by context menu command
	void copy(const std::vector<std::wstring>& paths) const {
		execute(paths, "copy");
	}

	// Cut files by context menu command
	void cut(const std::vector<std::wstring>& paths) const {
		execute(paths, "cut");
	}

	// Paste files in the path_0 directory by context menu command
	void paste_in(const std::vector<std::wstring>& path_0) const {
		execute(path_0, "paste");
	}

	// Paste files as links in the path_0 directory by context menu command
	void paste_as_link_in(const std::vector<std::wstring>& path_0) const {
		execute(path_0, "pastelink");
	}

	// Delete files by context menu command
	void delete_files(const std::vector<std::wstring>& paths) const {
		execute(paths, "delete");
	}

	// Show the property of files
	void show_property(const std::vector<std::wstring>& objs) const {
		// Instead of calling execute function with command "properties"...
		auto cm = (LPDATAOBJECT)Shell::get_ole_ui_object(objs, IID_IDataObject);
		if (!cm) return;
		::SHMultiFileProperties(cm, 0);
		cm->Release();
	}

};
