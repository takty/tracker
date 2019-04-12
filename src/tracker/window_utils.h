//
// Utilities of Window Operation
// 2019-04-12
//

#pragma once

#include "Windows.h"

using namespace std;

class WindowUtils {

public:

	WindowUtils()
	{
	}

	// Whether the topmost window is full screen
	static bool IsForegroundWindowFullScreen()
	{
		HWND fw = ::GetForegroundWindow();
		RECT wr;
		::GetWindowRect(fw, &wr);
		int sw = ::GetSystemMetrics(SM_CXSCREEN), sh = ::GetSystemMetrics(SM_CYSCREEN);

		if (wr.left <= 0 && wr.top <= 0 && sw <= wr.right && sh <= wr.bottom) {
			TCHAR cn[256];
			::GetClassName(fw, cn, 256);
			if (::lstrcmp(cn, _T("Progman")) != 0 && ::lstrcmp(cn, _T("WorkerW")) != 0) return true;
		}
		return false;
	}

	// Move the window to the corner of the screen (Win10 compatible)
	static void MoveWindowToCorner(HWND hWnd, int popupPos)
	{
		int sw = ::GetSystemMetrics(SM_CXSCREEN), sh = ::GetSystemMetrics(SM_CYSCREEN);
		RECT out, in;
		::GetWindowRect(hWnd, &out);
		::DwmGetWindowAttribute(hWnd, DWMWA_EXTENDED_FRAME_BOUNDS, &in, sizeof(RECT));

		int w = out.right - out.left;
		int h = out.bottom - out.top;
		int offL = in.left - out.left, offT = in.top - out.top;
		int offR = out.right - in.right, offB = out.bottom - in.bottom;

		int x = (popupPos == 0 || popupPos == 3) ? (0 - offL) /* Left Edge */ : (sw - w + offR);  // Right Edge
		int y = (popupPos == 0 || popupPos == 1) ? (0 - offT) /* Top Edge */ : (sh - h + offB);  // Bottom Edge
		::MoveWindow(hWnd, x, y, w, h, FALSE);
	}

	// Bring the window completely to the front
	static void ForegroundWindow(HWND hWnd)
	{
		DWORD t;
		DWORD foregID = ::GetWindowThreadProcessId(::GetForegroundWindow(), nullptr);
		DWORD targetID = ::GetWindowThreadProcessId(hWnd, nullptr);

		::AttachThreadInput(targetID, foregID, TRUE);
		::SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &t, 0);
		::SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, nullptr, 0);
		::SetForegroundWindow(hWnd);
		::SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, t, nullptr, 0);
		::AttachThreadInput(targetID, foregID, FALSE);
	}

	// Control key is pressed
	static bool CtrlPressed()
	{
		return (::GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
	}

	// Get DPI
	static POINT GetDPI(HWND hWnd) {
		auto monitor = ::MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		UINT fx, fy;
		::GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &fx, &fy);
		POINT dpi;
		dpi.x = fx;
		dpi.y = fy;
		return dpi;
	}

};
