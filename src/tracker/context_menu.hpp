/**
 *
 * Shell Context Menu
 *
 * @author Takuto Yanagida
 * @version 2021-05-30
 *
 */


#pragma once

#include <vector>
#include <string>
#include <windows.h>
#include <shlobj.h>

#include "Shell.hpp"

class ContextMenu;

using ContextMenuPtr = ContextMenu*;

class ContextMenu {

	static constexpr const wchar_t* const PROP_INSTANCE = L"ContextMenuInstance";

	// Window Procedure that hooked to the original procedure while showing the menu
	static LRESULT CALLBACK menu_proc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp) noexcept(false) {
		const auto cm = static_cast<ContextMenuPtr>(::GetProp(wnd, PROP_INSTANCE));
		if (!cm) return FALSE;
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

	const HWND wnd_ = nullptr;
	LPCONTEXTMENU2 context_menu_ = nullptr;
	WNDPROC orig_proc_ = nullptr;

	HRESULT invoke(const LPCONTEXTMENU cm, const char* cmd) const noexcept(false) {
		if (!cm) return 0;
		CMINVOKECOMMANDINFO ici{};
		ici.cbSize       = sizeof(ici);
		ici.fMask        = 0;
		ici.hwnd         = nullptr;
		ici.lpVerb       = cmd;
		ici.lpParameters = nullptr;
		ici.lpDirectory  = nullptr;
		ici.nShow        = SW_SHOWNORMAL;
		return cm->InvokeCommand(&ici);
	}

	HRESULT execute(const std::vector<std::wstring>& paths, const char* cmd) const noexcept(false) {
		const auto cm = static_cast<LPCONTEXTMENU>(Shell::get_ole_ui_object(paths, IID_IContextMenu));
		if (!cm) return 0;

		const auto res = invoke(cm, cmd);
		cm->Release();
		return res;
	}

public:

	ContextMenu(HWND wnd) noexcept : wnd_(wnd) {}

	ContextMenu(const ContextMenu& inst)            = delete;
	ContextMenu(ContextMenu&& inst)                 = delete;
	ContextMenu& operator=(const ContextMenu& inst) = delete;
	ContextMenu& operator=(ContextMenu&& inst)      = delete;

	~ContextMenu() {}

	// Popup shell context menu
	bool popup(const std::vector<std::wstring>& paths, uint32_t flag, const POINT& pt) noexcept(false) {
		const auto cm = static_cast<LPCONTEXTMENU>(Shell::get_ole_ui_object(paths, IID_IContextMenu));
		if (!cm) return false;

		// Get IContextMenu2
		LPCONTEXTMENU2 cm2 = nullptr;
		cm->QueryInterface(IID_IContextMenu2, (void**)(&cm2));

		// Create a menu
		auto hmenu = ::CreatePopupMenu();
		cm->QueryContextMenu(hmenu, 0, 1, 0x7fff, CMF_NORMAL);

		// Popup the menu
		::SetProp(wnd_, PROP_INSTANCE, this);
		context_menu_ = cm2;
		orig_proc_ = (WNDPROC)(::SetWindowLong(wnd_, GWL_WNDPROC, (LONG)menu_proc));
		const int id = ::TrackPopupMenu(hmenu, TPM_RETURNCMD | flag, pt.x, pt.y, 0, wnd_, nullptr);
		context_menu_ = nullptr;
		::SetWindowLong(wnd_, GWL_WNDPROC, (LONG)orig_proc_);
		::RemoveProp(wnd_, PROP_INSTANCE);

		bool res = false;
		if (id > 0) {
			if (S_OK == invoke(cm, MAKEINTRESOURCEA(id - 1))) {
				res = true;
			}
		}
		::DestroyMenu(hmenu);
		if (cm2) cm2->Release();
		cm->Release();
		return res;
	}

	// Open files by context menu command
	void open(const std::vector<std::wstring>& paths) const noexcept(false) {
		execute(paths, "open");
	}

	// Copy files by context menu command
	void copy(const std::vector<std::wstring>& paths) const noexcept(false) {
		execute(paths, "copy");
	}

	// Cut files by context menu command
	void cut(const std::vector<std::wstring>& paths) const noexcept(false) {
		execute(paths, "cut");
	}

	// Paste files in the path_0 directory by context menu command
	void paste_in(const std::vector<std::wstring>& path_0) const noexcept(false) {
		execute(path_0, "paste");
	}

	// Paste files as links in the path_0 directory by context menu command
	void paste_as_link_in(const std::vector<std::wstring>& path_0) const noexcept(false) {
		execute(path_0, "pastelink");
	}

	// Delete files by context menu command
	void delete_files(const std::vector<std::wstring>& paths) const noexcept(false) {
		execute(paths, "delete");
	}

	// Show the property of files
	void show_property(const std::vector<std::wstring>& objs) const noexcept(false) {
		// Instead of calling execute function with command "properties"...
		const auto obj = static_cast<LPDATAOBJECT>(Shell::get_ole_ui_object(objs, IID_IDataObject));
		if (!obj) return;
		::SHMultiFileProperties(obj, 0);
		obj->Release();
	}

};
