/**
 * Rename Edit
 *
 * @author Takuto Yanagida
 * @version 2025-11-10
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
	HWND         hWnd_    = nullptr;
	HWND         hEdit_   = nullptr;
	WNDPROC      orgProc_ = nullptr;
	std::wstring renamedPath_;
	std::wstring newFileName_;

	static LRESULT CALLBACK editProc(HWND hEdit_, UINT msg, WPARAM wp, LPARAM lp) {
		[[gsl::suppress(type.1)]]
		auto p = reinterpret_cast<RenameEdit*>(::GetWindowLongPtr(hEdit_, GWLP_USERDATA));

		switch (msg) {
		case WM_KEYDOWN:
			if (wp == VK_RETURN) {
				p->Close();
				break;
			} else if (wp == VK_ESCAPE) {
				::ShowWindow(hEdit_, SW_HIDE);
				break;
			}
			[[fallthrough]];
		default:
			return ::CallWindowProc(p->orgProc_, hEdit_, msg, wp, lp);
		}
		return 0;
	}

public:

	RenameEdit(int msg) noexcept : msg_(msg) {
	}

	void Initialize(HWND hWnd) noexcept {
		hWnd_ = hWnd;
		[[gsl::suppress(type.1)]]
		auto hInst = reinterpret_cast<HINSTANCE>(::GetWindowLongPtr(hWnd, GWLP_HINSTANCE));
		hEdit_ = ::CreateWindowEx(WS_EX_CLIENTEDGE, _T("EDIT"), _T(""), WS_CHILD | ES_AUTOHSCROLL,
			0, 0, 0, 0, hWnd, nullptr, hInst, nullptr);
		[[gsl::suppress(type.1)]]
		orgProc_ = reinterpret_cast<WNDPROC>(::GetWindowLongPtr(hEdit_, GWLP_WNDPROC));
		[[gsl::suppress(type.1)]]
		::SetWindowLongPtr(hEdit_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
		[[gsl::suppress(type.1)]]
		::SetWindowLongPtr(hEdit_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(RenameEdit::editProc));
	}

	void Finalize() const noexcept {
		[[gsl::suppress(type.1)]]
		::SetWindowLongPtr(hEdit_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(orgProc_));
	}

	void SetFont(HFONT hItemFont) const noexcept {
		[[gsl::suppress(type.1)]]
		::SendMessage(hEdit_, WM_SETFONT, reinterpret_cast<WPARAM>(hItemFont), 0);
	}

	bool IsActive() const noexcept {
		return ::IsWindowVisible(hEdit_) == TRUE;
	}

	void Open(const std::wstring path, int y, int width, int height) {
		renamedPath_.clear();
		if (path::is_root(path)) return;
		renamedPath_.assign(path);
		auto fname = path::name(renamedPath_);

		auto len = fname.size();
		if (link::is_link(renamedPath_)) {
			fname.resize(fname.size() - 4);  // If it is a shortcut, remove the extension from the file name
		} else {
			auto exe = path::ext(fname);
			if (!exe.empty()) len -= exe.size() + 1;
		}
		::SetWindowText(hEdit_, fname.c_str());
		::SendMessage(hEdit_, EM_SETSEL, 0, len);

		::MoveWindow(hEdit_, 0, y, width, height, TRUE);
		::ShowWindow(hEdit_, SW_SHOWNORMAL);
		::SetFocus(hEdit_);
	}

	// Close name change
	void Close() {
		if (!::IsWindowVisible(hEdit_)) return;

		::ShowWindow(hEdit_, SW_HIDE);
		if (!::SendMessage(hEdit_, EM_GETMODIFY, 0, 0) || renamedPath_.empty()) {
			return;
		}
		const auto len = ::GetWindowTextLength(hEdit_);  // Not including terminal NULL
		auto fname = std::vector<wchar_t>(gsl::narrow<size_t>(len) + 1);  // Add terminal NULL
		::GetWindowText(hEdit_, fname.data(), len + 1);  // Add terminal NULL
		newFileName_.assign(fname.data());
		if (link::is_link(renamedPath_)) {
			newFileName_.append(_T(".lnk"));
		}
		::SendMessage(hWnd_, msg_, 0, 0);
	}

	const std::wstring& GetRenamePath() noexcept {
		return renamedPath_;
	}

	const std::wstring& GetNewFileName() noexcept {
		return newFileName_;
	}

};
