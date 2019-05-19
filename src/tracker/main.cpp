//
// Main Function
// 2019-04-12
//

#include "stdafx.h"
#include "View.h"

const wchar_t MUTEX[] = _T("TRACKER400_20190519");
const wchar_t CLASS_NAME[] = _T("Tracker");
const wchar_t WINDOW_NAME[] = _T("Tracker");

int WINAPI WinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	::CreateMutex(nullptr, FALSE, MUTEX);
	if (::GetLastError() == ERROR_ALREADY_EXISTS) {
		::MessageBeep(MB_ICONHAND);
		return 0;
	}
	auto res = ::CoInitialize(nullptr);  // For shell context menu
	if (FAILED(res)) return 0;
	auto hr = ::OleInitialize(nullptr);  // For supporting drag
	if (FAILED(hr)) return 0;

	// For supporting tool tip
	INITCOMMONCONTROLSEX iccex;
	iccex.dwICC = ICC_WIN95_CLASSES;
	iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	::InitCommonControlsEx(&iccex);

	if (!View::InitApplication(hInst, CLASS_NAME)) {
		::MessageBeep(MB_ICONHAND);
		return 0;
	}
	// Create main window
	HWND hWnd = ::CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, CLASS_NAME, WINDOW_NAME, WS_CAPTION | WS_SYSMENU | WS_THICKFRAME, 0, 0, 0, 0, nullptr, nullptr, hInst, nullptr);
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
