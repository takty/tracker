/**
 * Tool Tip
 *
 * @author Takuto Yanagida
 * @version 2025-11-20
 *
 * Need to add to stdafx.h
 * #pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
 */

#pragma once

#include <windows.h>
#include <commctrl.h>
#include <string>

#include "gsl/gsl"
#include "classes.h"

class ToolTip {

	HWND wnd_       = nullptr;
	HWND hint_      = nullptr;
	bool is_active_ = false;

public:

	ToolTip() = default;

	void initialize(HWND hWnd) noexcept {
		[[gsl::suppress(type.1)]]
		auto inst = reinterpret_cast<HINSTANCE>(::GetWindowLongPtr(hWnd, GWLP_HINSTANCE));

		wnd_  = hWnd;
		hint_ = ::CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr,
			WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_NOANIMATE | TTS_NOFADE,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT , CW_USEDEFAULT, hWnd, nullptr, inst, nullptr);
	}

	//  Display Tool Tips
	void activate(const std::wstring& str, const RECT& rect) noexcept {
		std::wstring temp = str;

		TOOLINFO ti{};
		ti.cbSize   = sizeof(TOOLINFO);
		ti.uFlags   = TTF_SUBCLASS | TTF_TRANSPARENT;
		ti.hwnd     = wnd_;
		ti.uId      = 1;
		ti.rect     = rect;
		ti.hinst    = nullptr;
		ti.lpszText = temp.data();

		[[gsl::suppress(type.1)]]
		::SendMessage(hint_, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&ti));

		is_active_ = true;
	}

	// Hide Tool Tips
	void inactivate() noexcept {
		if (!is_active_) return;
		is_active_ = false;

		TOOLINFO ti{};
		ti.cbSize = sizeof(TOOLINFO);
		ti.hwnd   = wnd_;
		ti.uId    = 1;

		[[gsl::suppress(type.1)]]
		::SendMessage(hint_, TTM_DELTOOL, 0, reinterpret_cast<LPARAM>(&ti));
	}

};
