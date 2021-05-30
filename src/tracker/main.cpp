/**
 *
 * Main Function
 *
 * @author Takuto Yanagida
 * @version 2021-05-30
 *
 */


#include "stdafx.h"
#include "view.h"

const wchar_t MUTEX[]       = _T("TRACKER5_21_5");
const wchar_t CLASS_NAME[]  = _T("Tracker");
extern const wchar_t WINDOW_NAME[] = _T("Tracker");


int WINAPI WinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	::CreateMutex(nullptr, FALSE, &MUTEX[0]);
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

	if (!InitApplication(hInst, &CLASS_NAME[0])) {
		::MessageBeep(MB_ICONHAND);
		return 0;
	}
	// Create main window
	const HWND hWnd = ::CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, &CLASS_NAME[0], &WINDOW_NAME[0], WS_CAPTION | WS_SYSMENU | WS_THICKFRAME, 0, 0, 0, 0, nullptr, nullptr, hInst, nullptr);
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
	wc.lpfnWndProc   = (WNDPROC)WndProc;
	wc.hInstance     = hInst;
	wc.hIcon         = nullptr;
	wc.hCursor       = ::LoadCursor(nullptr, IDC_ARROW);  // Mouse cursor (standard arrow)
	wc.hbrBackground = nullptr;
	wc.lpszMenuName  = nullptr;
	wc.lpszClassName = className;
	return ::RegisterClass(&wc);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
	auto view = (View*)::GetWindowLong(hWnd, GWL_USERDATA);

	switch (msg) {
	case WM_CREATE:            new View(hWnd); break;
	case WM_DESTROY:           delete view; break;
	case WM_DPICHANGED:        view->wm_dpi_changed(LOWORD(wp), HIWORD(wp)); break;
	case WM_WINDOWPOSCHANGING: view->wm_window_pos_changing((LPWINDOWPOS)lp); break;
	case WM_SIZE:              view->wm_size(LOWORD(lp), HIWORD(lp)); break;
	case WM_PAINT:             view->wm_paint(); break;
	case WM_ACTIVATEAPP:       view->wm_activate_app((BOOL)wp == TRUE); break;
	case WM_TIMER:             view->wm_timer(); break;
	case WM_HOTKEY:            view->wm_hot_key(wp); break;
	case WM_SHOWWINDOW:        view->wm_show_window((BOOL)wp); break;
	case WM_LBUTTONDOWN:       view->wm_button_down(VK_LBUTTON, LOWORD(lp), HIWORD(lp)); break;
	case WM_RBUTTONDOWN:       view->wm_button_down(VK_RBUTTON, LOWORD(lp), HIWORD(lp)); break;
	case WM_MBUTTONDOWN:       view->wm_button_down(VK_MBUTTON, LOWORD(lp), HIWORD(lp)); break;
	case WM_MOUSEMOVE:         view->wm_mouse_move(wp, LOWORD(lp), HIWORD(lp)); break;
	case WM_LBUTTONUP:         view->wm_button_up(VK_LBUTTON, LOWORD(lp), HIWORD(lp), wp); break;
	case WM_RBUTTONUP:         view->wm_button_up(VK_RBUTTON, LOWORD(lp), HIWORD(lp), wp); break;
	case WM_MBUTTONUP:         view->wm_button_up(VK_MBUTTON, LOWORD(lp), HIWORD(lp), wp); break;
	case WM_MOUSEWHEEL:        view->wm_mouse_wheel((short)HIWORD(wp)); break;
	case WM_VSCROLL:           view->wm_mouse_wheel((wp == SB_LINEUP) ? 1 : -1); break;  // Temporary
	case WM_ENDSESSION:        view->wm_end_session(); break;
	case WM_REQUESTUPDATE:     view->wm_request_update(); break;
	case WM_RENAMEEDITCLOSED:  view->wm_rename_edit_closed(); break;
	case WM_KEYDOWN:           view->wm_key_down(wp); break;
	case WM_ENTERMENULOOP:     view->wm_menu_loop(true); break;
	case WM_EXITMENULOOP:      view->wm_menu_loop(false); break;
	case WM_CLOSE:             view->wm_close(); break;
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
	case WM_NCRBUTTONDOWN:     break;
	default:                   return DefWindowProc(hWnd, msg, wp, lp);
	}
	return 0;
}
