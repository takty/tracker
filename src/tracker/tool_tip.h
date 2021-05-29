/**
 *
 * Tool Tip
 *
 * @author Takuto Yanagida
 * @version 2021-05-29
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

	HWND hWnd_     = nullptr;
	HWND hHint_    = nullptr;
	bool isActive_ = false;

public:

	ToolTip() noexcept {
	}

	void Initialize(HWND hWnd) noexcept {
		HINSTANCE hInst = (HINSTANCE)::GetWindowLong(hWnd, GWL_HINSTANCE);
		hWnd_ = hWnd;
		hHint_ = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr,
			WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_NOANIMATE | TTS_NOFADE,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hWnd, nullptr, hInst, nullptr);
	}

	//  Display Tool Tips
	void Activate(const std::wstring& str, const RECT& rect) noexcept {
		TOOLINFO ti{};
		ti.cbSize   = sizeof(TOOLINFO);
		ti.uFlags   = TTF_SUBCLASS | TTF_TRANSPARENT;
		ti.hwnd     = hWnd_;
		ti.uId      = 1;
		ti.rect     = rect;
		ti.hinst    = nullptr;
		ti.lpszText = (LPWSTR)str.data();
		::SendMessage(hHint_, TTM_ADDTOOL, 0, (LPARAM)(&ti));

		isActive_ = true;
	}

	// Hide Tool Tips
	void Inactivate() noexcept {
		if (!isActive_) return;
		isActive_ = false;

		TOOLINFO ti{};
		ti.cbSize = sizeof(TOOLINFO);
		ti.hwnd   = hWnd_;
		ti.uId    = 1;
		::SendMessage(hHint_, TTM_DELTOOL, 0, (LPARAM)(&ti));
	}

};
