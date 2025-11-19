/**
 * Window Utilities
 *
 * @author Takuto Yanagida
 * @version 2025-11-19
 */

#pragma once

#include "Windows.h"

namespace window_utils {

	// Whether the topmost window is full screen
	bool is_foreground_window_full_screen() noexcept {
		const int sw = ::GetSystemMetrics(SM_CXSCREEN);
		const int sh = ::GetSystemMetrics(SM_CYSCREEN);
		HWND fw = ::GetForegroundWindow();
		RECT wr{};
		::GetWindowRect(fw, &wr);

		if (wr.left <= 0 && wr.top <= 0 && sw <= wr.right && sh <= wr.bottom) {
			TCHAR cn[256]{};
			::GetClassName(fw, &cn[0], 256);
			if (::lstrcmp(&cn[0], _T("Progman")) != 0 && ::lstrcmp(&cn[0], _T("WorkerW")) != 0) {
				return true;
			}
		}
		return false;
	}

	// Move the window to the corner of the screen (Win10 compatible)
	void move_window_to_corner(HWND hwnd, int popupPos) noexcept {
		const int sw = ::GetSystemMetrics(SM_CXSCREEN);
		const int sh = ::GetSystemMetrics(SM_CYSCREEN);
		RECT out{}, in{};
		::GetWindowRect(hwnd, &out);
		::DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &in, sizeof(RECT));

		const int w     = out.right  - out.left;
		const int h     = out.bottom - out.top;
		const int off_l = in.left    - out.left;
		const int off_t = in.top     - out.top;
		const int off_r = out.right  - in.right;
		const int off_b = out.bottom - in.bottom;

		const int x = (popupPos == 0 || popupPos == 3) ? (0 - off_l) /* Left Edge */ : (sw - w + off_r);  // Right Edge
		const int y = (popupPos == 0 || popupPos == 1) ? (0 - off_t) /* Top Edge */  : (sh - h + off_b);  // Bottom Edge
		::MoveWindow(hwnd, x, y, w, h, FALSE);
	}

	// Bring the window completely to the front
	void foreground_window(HWND hWnd) noexcept {
		DWORD t{};
		const DWORD for_id = ::GetWindowThreadProcessId(::GetForegroundWindow(), nullptr);
		const DWORD tar_id = ::GetWindowThreadProcessId(hWnd, nullptr);

		::AttachThreadInput(tar_id, for_id, TRUE);
		::SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &t, 0);
		::SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, nullptr, 0);
		::SetForegroundWindow(hWnd);
		::SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, t, nullptr, 0);
		::AttachThreadInput(tar_id, for_id, FALSE);
	}

	bool ctrl_pressed() noexcept {
		return (::GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
	}

	HFONT get_ui_message_font(HWND hwnd) noexcept {
		NONCLIENTMETRICSW ncm{};
		ncm.cbSize = sizeof(ncm);
		const int dpi = ::GetDpiForWindow(hwnd);
		if (!::SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0, dpi)) {
			return nullptr;
		}
		return ::CreateFontIndirect(&ncm.lfMessageFont);
	}

	void draw_gray_text(HDC dc, RECT r, const TCHAR* str) noexcept {
		::SetBkMode(dc, TRANSPARENT);
		r.left += 2;
		r.top  += 1;
		::SetTextColor(dc, GetSysColor(COLOR_HIGHLIGHTTEXT));
		::DrawText(dc, str, -1, &r, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		r.left -= 2;
		r.top  -= 1;
		::SetTextColor(dc, GetSysColor(COLOR_GRAYTEXT));
		::DrawText(dc, str, -1, &r, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		r.left += 1;
		::DrawText(dc, str, -1, &r, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	}

};
