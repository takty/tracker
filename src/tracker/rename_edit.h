/**
 *
 * Rename Edit
 *
 * @author Takuto Yanagida
 * @version 2021-05-30
 *
 */


#pragma once

#include <string>
#include <windows.h>
#include <commctrl.h>

#include "file_utils.hpp"


class RenameEdit {

	int          msg_;
	HWND         hwnd_    = nullptr;
	HWND         hedit_   = nullptr;
	WNDPROC      orig_proc_ = nullptr;
	std::wstring renamed_path_;
	std::wstring new_file_name_;

	static LRESULT CALLBACK edit_proc(HWND hedit_, UINT msg, WPARAM wp, LPARAM lp) noexcept {
		auto p = (RenameEdit*) GetWindowLong(hedit_, GWL_USERDATA);

		switch (msg) {
		case WM_KEYDOWN:
			if (wp == VK_RETURN) {
				p->hide();
				break;
			} else if (wp == VK_ESCAPE) {
				::ShowWindow(hedit_, SW_HIDE);
				break;
			}
			[[fallthrough]];
		default:
			return ::CallWindowProc(p->orig_proc_, hedit_, msg, wp, lp);
		}
		return 0;
	}

public:

	RenameEdit(int msg) noexcept : msg_(msg) {}

	void initialize(HWND hwnd) noexcept {
		hwnd_ = hwnd;
		auto hInst = (HINSTANCE)::GetWindowLong(hwnd, GWL_HINSTANCE);
		hedit_ = ::CreateWindowEx(
			WS_EX_CLIENTEDGE,
			_T("EDIT"),
			_T(""),
			WS_CHILD | ES_AUTOHSCROLL,
			0, 0, 0, 0, hwnd, nullptr, hInst, nullptr
		);
		orig_proc_ = (WNDPROC)::GetWindowLong(hedit_, GWL_WNDPROC);
		::SetWindowLong(hedit_, GWL_USERDATA, (LONG)this);
		::SetWindowLong(hedit_, GWL_WNDPROC, (LONG)RenameEdit::edit_proc);
	}

	void finalize() noexcept {
		::SetWindowLong(hedit_, GWL_WNDPROC, (LONG)orig_proc_);
	}

	void set_font(const HFONT hItemFont) const noexcept {
		::SendMessage(hedit_, WM_SETFONT, (WPARAM)hItemFont, 0);
	}

	bool is_active() noexcept {
		return ::IsWindowVisible(hedit_) == TRUE;
	}

	void show(const std::wstring path, int y, int width, int height) noexcept {
		renamed_path_.clear();
		if (Path::is_root(path)) return;
		renamed_path_.assign(path);
		auto fname = Path::name(renamed_path_);

		auto len = fname.size();
		if (Link::is_link(renamed_path_)) {
			fname.resize(fname.size() - 4);  // If it is a shortcut, remove the extension from the file name
		} else {
			auto exe = Path::ext(fname);
			if (!exe.empty()) len -= exe.size() + 1;
		}
		::SetWindowText(hedit_, fname.c_str());
		::SendMessage(hedit_, EM_SETSEL, 0, len);

		::MoveWindow(hedit_, 0, y, width, height, TRUE);
		::ShowWindow(hedit_, SW_SHOWNORMAL);
		::SetFocus(hedit_);
	}

	// Close name change
	void hide() noexcept {
		if (!::IsWindowVisible(hedit_)) return;

		::ShowWindow(hedit_, SW_HIDE);
		if (!::SendMessage(hedit_, EM_GETMODIFY, 0, 0) || renamed_path_.empty()) return;
		const auto len = ::GetWindowTextLength(hedit_);  // Not including terminal NULL
		const auto fname = std::make_unique<wchar_t[]>(len + 1);  // Add terminal NULL
		::GetWindowText(hedit_, fname.get(), len + 1);  // Add terminal NULL
		new_file_name_.assign(fname.get());
		if (Link::is_link(renamed_path_)) new_file_name_.append(_T(".lnk"));
		::SendMessage(hwnd_, msg_, 0, 0);
	}

	const std::wstring& get_renamed_path() noexcept {
		return renamed_path_;
	}

	const std::wstring& get_new_file_name() noexcept {
		return new_file_name_;
	}

};
