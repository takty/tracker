/**
 *
 * Tool Tip
 *
 * @author Takuto Yanagida
 * @version 2021-05-30
 *
 * Need to add to stdafx.h
 * #pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
 *
 */


#pragma once

#include <string>
#include <windows.h>
#include <commctrl.h>


class ToolTip {

	HWND hwnd_      = nullptr;
	HWND hhint_     = nullptr;
	bool is_active_ = false;

public:

	ToolTip() noexcept {}

	void initialize(HWND hwnd) noexcept {
		HINSTANCE hinst = (HINSTANCE)::GetWindowLong(hwnd, GWL_HINSTANCE);
		hwnd_ = hwnd;
		hhint_ = ::CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr,
			WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_NOANIMATE | TTS_NOFADE,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hwnd, nullptr, hinst, nullptr);
	}

	//  Display Tool Tips
	void activate(const std::wstring& str, const RECT& rect) noexcept {
		TOOLINFO ti{};
		ti.cbSize   = sizeof(TOOLINFO);
		ti.uFlags   = TTF_SUBCLASS | TTF_TRANSPARENT;
		ti.hwnd     = hwnd_;
		ti.uId      = 1;
		ti.rect     = rect;
		ti.hinst    = nullptr;
		ti.lpszText = (LPWSTR)str.data();
		::SendMessage(hhint_, TTM_ADDTOOL, 0, (LPARAM)(&ti));

		is_active_ = true;
	}

	// Hide Tool Tips
	void inactivate() noexcept {
		if (!is_active_) return;
		is_active_ = false;

		TOOLINFO ti{};
		ti.cbSize = sizeof(TOOLINFO);
		ti.hwnd   = hwnd_;
		ti.uId    = 1;
		::SendMessage(hhint_, TTM_DELTOOL, 0, (LPARAM)(&ti));
	}

};
