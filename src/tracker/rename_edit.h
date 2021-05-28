/**
 *
 * Rename Edit
 *
 * @author Takuto Yanagida
 * @version 2021-05-29
 *
 */


#pragma once

#include <string>
#include <windows.h>
#include <commctrl.h>

#include "file_utils.hpp"


class RenameEdit {

	int          msg_;
	HWND         hWnd_    = nullptr;
	HWND         hEdit_   = nullptr;
	WNDPROC      orgProc_ = nullptr;
	std::wstring renamedPath_;
	std::wstring newFileName_;

	static LRESULT CALLBACK editProc(HWND hEdit_, UINT msg, WPARAM wp, LPARAM lp) {
		auto p = (RenameEdit*) GetWindowLong(hEdit_, GWL_USERDATA);

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
		auto hInst = (HINSTANCE)::GetWindowLong(hWnd, GWL_HINSTANCE);
		hEdit_ = ::CreateWindowEx(WS_EX_CLIENTEDGE, _T("EDIT"), _T(""), WS_CHILD | ES_AUTOHSCROLL,
			0, 0, 0, 0, hWnd, nullptr, hInst, nullptr);
		orgProc_ = (WNDPROC)::GetWindowLong(hEdit_, GWL_WNDPROC);
		::SetWindowLong(hEdit_, GWL_USERDATA, (LONG)this);
		::SetWindowLong(hEdit_, GWL_WNDPROC, (LONG)RenameEdit::editProc);
	}

	void Finalize() noexcept {
		::SetWindowLong(hEdit_, GWL_WNDPROC, (LONG)orgProc_);
	}

	void SetFont(const HFONT hItemFont) const noexcept {
		::SendMessage(hEdit_, WM_SETFONT, (WPARAM)hItemFont, 0);
	}

	bool IsActive() noexcept {
		return ::IsWindowVisible(hEdit_) == TRUE;
	}

	void Open(const std::wstring path, int y, int width, int height) {
		renamedPath_.clear();
		if (Path::is_root(path)) return;
		renamedPath_.assign(path);
		auto fname = Path::name(renamedPath_);

		auto len = fname.size();
		if (Link::is_link(renamedPath_)) {
			fname.resize(fname.size() - 4);  // If it is a shortcut, remove the extension from the file name
		} else {
			auto exe = Path::ext(fname);
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
		if (!::SendMessage(hEdit_, EM_GETMODIFY, 0, 0) || renamedPath_.empty()) return;
		const auto len = ::GetWindowTextLength(hEdit_);  // Not including terminal NULL
		const auto fname = std::make_unique<wchar_t[]>(len + 1);  // Add terminal NULL
		::GetWindowText(hEdit_, fname.get(), len + 1);  // Add terminal NULL
		newFileName_.assign(fname.get());
		if (Link::is_link(renamedPath_)) newFileName_.append(_T(".lnk"));
		::SendMessage(hWnd_, msg_, 0, 0);
	}

	const std::wstring& GetRenamePath() noexcept {
		return renamedPath_;
	}

	const std::wstring& GetNewFileName() noexcept {
		return newFileName_;
	}

};
