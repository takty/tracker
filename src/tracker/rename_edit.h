/**
 * Rename Edit
 *
 * @author Takuto Yanagida
 * @version 2025-11-20
 */

#pragma once

#include <memory>
#include <vector>

#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include <string>

#include "gsl/gsl"
#include "file_utils.hpp"

class RenameEdit {

	int          msg_;
	HWND         hwnd_     = nullptr;
	HWND         hedit_    = nullptr;
	WNDPROC      org_proc_ = nullptr;
	std::wstring renamed_path_;
	std::wstring new_file_name_;

	static LRESULT CALLBACK edit_proc(HWND hedit, UINT msg, WPARAM wp, LPARAM lp) {
		[[gsl::suppress(type.1)]]
		auto p = reinterpret_cast<RenameEdit*>(::GetWindowLongPtr(hedit, GWLP_USERDATA));

		switch (msg) {
		case WM_KEYDOWN:
			if (wp == VK_RETURN) {
				p->close();
				break;
			} else if (wp == VK_ESCAPE) {
				::ShowWindow(hedit, SW_HIDE);
				break;
			}
			[[fallthrough]];
		default:
			return ::CallWindowProc(p->org_proc_, hedit, msg, wp, lp);
		}
		return 0;
	}

public:

	RenameEdit(int msg) noexcept : msg_(msg) {
	}

	void initialize(HWND hwnd) noexcept {
		hwnd_ = hwnd;
		[[gsl::suppress(type.1)]]
		auto hinst = reinterpret_cast<HINSTANCE>(::GetWindowLongPtr(hwnd, GWLP_HINSTANCE));
		hedit_ = ::CreateWindowEx(WS_EX_CLIENTEDGE, _T("EDIT"), _T(""), WS_CHILD | ES_AUTOHSCROLL,
			0, 0, 0, 0, hwnd, nullptr, hinst, nullptr);
		[[gsl::suppress(type.1)]]
		org_proc_ = reinterpret_cast<WNDPROC>(::GetWindowLongPtr(hedit_, GWLP_WNDPROC));
		[[gsl::suppress(type.1)]]
		::SetWindowLongPtr(hedit_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
		[[gsl::suppress(type.1)]]
		::SetWindowLongPtr(hedit_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(RenameEdit::edit_proc));
	}

	void finalize() const noexcept {
		[[gsl::suppress(type.1)]]
		::SetWindowLongPtr(hedit_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(org_proc_));
	}

	void set_font(HFONT hfont) const noexcept {
		[[gsl::suppress(type.1)]]
		::SendMessage(hedit_, WM_SETFONT, reinterpret_cast<WPARAM>(hfont), 0);
	}

	bool is_active() const noexcept {
		return ::IsWindowVisible(hedit_) == TRUE;
	}

	void open(const std::wstring path, int y, int width, int height) {
		renamed_path_.clear();
		if (path::is_root(path)) return;
		renamed_path_.assign(path);
		auto fname = path::name(renamed_path_);

		auto len = fname.size();
		if (link::is_link(renamed_path_)) {
			fname.resize(fname.size() - 4);  // If it is a shortcut, remove the extension from the file name
		} else {
			auto exe = path::ext(fname);
			if (!exe.empty()) len -= exe.size() + 1;
		}
		::SetWindowText(hedit_, fname.c_str());
		::SendMessage(hedit_, EM_SETSEL, 0, len);

		::MoveWindow(hedit_, 0, y, width, height, TRUE);
		::ShowWindow(hedit_, SW_SHOWNORMAL);
		::SetFocus(hedit_);
	}

	// Close name change
	void close() {
		if (!::IsWindowVisible(hedit_)) return;

		::ShowWindow(hedit_, SW_HIDE);
		if (!::SendMessage(hedit_, EM_GETMODIFY, 0, 0) || renamed_path_.empty()) {
			return;
		}
		const auto len = ::GetWindowTextLength(hedit_);  // Not including terminal NULL
		auto fname = std::vector<wchar_t>(gsl::narrow<size_t>(len) + 1);  // Add terminal NULL
		::GetWindowText(hedit_, fname.data(), len + 1);  // Add terminal NULL
		new_file_name_.assign(fname.data());
		if (link::is_link(renamed_path_)) {
			new_file_name_.append(_T(".lnk"));
		}
		::SendMessage(hwnd_, msg_, 0, 0);
	}

	const std::wstring& get_rename_path() noexcept {
		return renamed_path_;
	}

	const std::wstring& get_new_file_name() noexcept {
		return new_file_name_;
	}

};
