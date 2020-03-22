/**
 *
 * Window Utilities
 *
 * @author Takuto Yanagida
 * @version 2020-03-22
 *
 */


#pragma once

#include "Windows.h"


class WindowUtils {

public:

	WindowUtils() {
	}

	// Whether the topmost window is full screen
	static bool IsForegroundWindowFullScreen() {
		HWND fw = ::GetForegroundWindow();
		RECT wr;
		::GetWindowRect(fw, &wr);
		int sw = ::GetSystemMetrics(SM_CXSCREEN);
		int sh = ::GetSystemMetrics(SM_CYSCREEN);

		if (wr.left <= 0 && wr.top <= 0 && sw <= wr.right && sh <= wr.bottom) {
			TCHAR cn[256];
			::GetClassName(fw, cn, 256);
			if (::lstrcmp(cn, _T("Progman")) != 0 && ::lstrcmp(cn, _T("WorkerW")) != 0) return true;
		}
		return false;
	}

	// Move the window to the corner of the screen (Win10 compatible)
	static void MoveWindowToCorner(HWND hWnd, int popupPos) {
		int sw = ::GetSystemMetrics(SM_CXSCREEN), sh = ::GetSystemMetrics(SM_CYSCREEN);
		RECT out, in;
		::GetWindowRect(hWnd, &out);
		::DwmGetWindowAttribute(hWnd, DWMWA_EXTENDED_FRAME_BOUNDS, &in, sizeof(RECT));

		int w    = out.right  - out.left;
		int h    = out.bottom - out.top;
		int offL = in.left    - out.left;
		int offT = in.top     - out.top;
		int offR = out.right  - in.right;
		int offB = out.bottom - in.bottom;

		int x = (popupPos == 0 || popupPos == 3) ? (0 - offL) /* Left Edge */ : (sw - w + offR);  // Right Edge
		int y = (popupPos == 0 || popupPos == 1) ? (0 - offT) /* Top Edge */ : (sh - h + offB);  // Bottom Edge
		::MoveWindow(hWnd, x, y, w, h, FALSE);
	}

	// Bring the window completely to the front
	static void ForegroundWindow(HWND hWnd) {
		DWORD t;
		DWORD forId = ::GetWindowThreadProcessId(::GetForegroundWindow(), nullptr);
		DWORD tarId = ::GetWindowThreadProcessId(hWnd, nullptr);

		::AttachThreadInput(tarId, forId, TRUE);
		::SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &t, 0);
		::SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, nullptr, 0);
		::SetForegroundWindow(hWnd);
		::SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, t, nullptr, 0);
		::AttachThreadInput(tarId, forId, FALSE);
	}

	static bool CtrlPressed() {
		return (::GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
	}

	static POINT GetDPI(HWND hWnd) {
		auto monitor = ::MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		UINT fx, fy;
		::GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &fx, &fy);
		POINT dpi;
		dpi.x = fx;
		dpi.y = fy;
		return dpi;
	}

	static void DrawGrayText(HDC dc, RECT r, const TCHAR* str) {
		::SetBkMode(dc, TRANSPARENT);
		r.left += 2, r.top += 1;
		::SetTextColor(dc, GetSysColor(COLOR_HIGHLIGHTTEXT));
		::DrawText(dc, str, -1, &r, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		r.left -= 2, r.top -= 1;
		::SetTextColor(dc, GetSysColor(COLOR_GRAYTEXT));
		::DrawText(dc, str, -1, &r, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		r.left += 1;
		::DrawText(dc, str, -1, &r, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	}

};
