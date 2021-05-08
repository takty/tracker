/**
 *
 * Main Function
 *
 * @author Takuto Yanagida
 * @version 2021-05-08
 *
 */


#include "stdafx.h"
#include "View.h"

const wchar_t MUTEX[]       = _T("TRACKER500_20200323");
const wchar_t CLASS_NAME[]  = _T("Tracker");
const wchar_t WINDOW_NAME[] = _T("Tracker");


int WINAPI WinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	::CreateMutex(nullptr, FALSE, (const wchar_t*) MUTEX);
	if (::GetLastError() == ERROR_ALREADY_EXISTS) {
		::MessageBeep(MB_ICONHAND);
		return 0;
	}
	const auto res = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);  // For shell context menu
	if (FAILED(res)) return 0;
	const auto hr = ::OleInitialize(nullptr);  // For supporting drag
	if (FAILED(hr)) return 0;

	// For supporting tool tip
	INITCOMMONCONTROLSEX iccex{};
	iccex.dwICC  = ICC_WIN95_CLASSES;
	iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	::InitCommonControlsEx(&iccex);

	if (!InitApplication(hInst, CLASS_NAME)) {
		::MessageBeep(MB_ICONHAND);
		return 0;
	}
	// Create main window
	const HWND hWnd = ::CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, CLASS_NAME, WINDOW_NAME, WS_CAPTION | WS_SYSMENU | WS_THICKFRAME, 0, 0, 0, 0, nullptr, nullptr, hInst, nullptr);
	if (!hWnd) {
		::MessageBeep(MB_ICONHAND);
		return 0;
	}
	MSG msg;
	while (::GetMessage(&msg, nullptr, 0, 0) > 0) {  // Exit with WM_QUIT or error
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}
	::OleUninitialize();  // For supporting drag
	::CoUninitialize();
	return msg.wParam;
}

BOOL InitApplication(HINSTANCE hInst, const wchar_t* className) noexcept {
	WNDCLASS wc{};
	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = (WNDPROC)wndProc;
	// wc.cbClsExtra    = 0;
	// wc.cbWndExtra    = 0;
	wc.hInstance     = hInst;
	wc.hIcon         = nullptr;
	wc.hCursor       = ::LoadCursor(nullptr, IDC_ARROW);  // Mouse cursor (standard arrow)
	wc.hbrBackground = nullptr;
	wc.lpszMenuName  = nullptr;
	wc.lpszClassName = className;
	return ::RegisterClass(&wc);
}

LRESULT CALLBACK wndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
	auto view = (View*)::GetWindowLong(hWnd, GWL_USERDATA);

	switch (msg) {
	case WM_CREATE:            new View(hWnd); break;
	case WM_DESTROY:           delete view; break;
	case WM_DPICHANGED:        view->wmDpiChanged(LOWORD(wp), HIWORD(wp)); break;
	case WM_WINDOWPOSCHANGING: view->wmWindowPosChanging((LPWINDOWPOS)lp); break;
	case WM_SIZE:              view->wmSize(LOWORD(lp), HIWORD(lp)); break;
	case WM_PAINT:             view->wmPaint(); break;
	case WM_ACTIVATEAPP:       if (!(BOOL)wp && ::GetCapture() != hWnd) ::ShowWindow(hWnd, SW_HIDE);  break;
	case WM_TIMER:             view->wmTimer(); break;
	case WM_HOTKEY:            view->wmHotKey(wp); break;
	case WM_SHOWWINDOW:        view->wmShowWindow((BOOL)wp); break;
	case WM_LBUTTONDOWN:       view->wmButtonDown(VK_LBUTTON, LOWORD(lp), HIWORD(lp)); break;
	case WM_RBUTTONDOWN:       view->wmButtonDown(VK_RBUTTON, LOWORD(lp), HIWORD(lp)); break;
	case WM_MBUTTONDOWN:       view->wmButtonDown(VK_MBUTTON, LOWORD(lp), HIWORD(lp)); break;
	case WM_MOUSEMOVE:         view->wmMouseMove(wp, LOWORD(lp), HIWORD(lp)); break;
	case WM_LBUTTONUP:         view->wmButtonUp(VK_LBUTTON, LOWORD(lp), HIWORD(lp), wp); break;
	case WM_RBUTTONUP:         view->wmButtonUp(VK_RBUTTON, LOWORD(lp), HIWORD(lp), wp); break;
	case WM_MBUTTONUP:         view->wmButtonUp(VK_MBUTTON, LOWORD(lp), HIWORD(lp), wp); break;
	case WM_MOUSEWHEEL:        view->wmMouseWheel((short)HIWORD(wp)); break;
	case WM_VSCROLL:           view->wmMouseWheel((wp == SB_LINEUP) ? 1 : -1); break;  // Temporary
	case WM_ENDSESSION:        view->wmEndSession(); break;
	case WM_REQUESTUPDATE:     view->wmRequestUpdate(); break;
	case WM_RENAMEEDITCLOSED:  view->wmRenameEditClosed(); break;
	case WM_KEYDOWN:           view->wmKeyDown(wp); break;
	case WM_ENTERMENULOOP:     view->wmMenuLoop(true); break;
	case WM_EXITMENULOOP:      view->wmMenuLoop(false); break;
	case WM_CLOSE:             view->wmClose(); break;
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
	case WM_NCRBUTTONDOWN:     break;
	default:                   return DefWindowProc(hWnd, msg, wp, lp);
	}
	return 0;
}
