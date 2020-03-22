/**
 *
 * View
 *
 * @author Takuto Yanagida
 * @version 2020-03-22
 *
 */


#pragma once

#include <windows.h>
#include <vector>
#include <string>

#include "tracker.h"
#include "file_utils.hpp"
#include "pref.hpp"
#include "document.h"
#include "selection.h"
#include "type_table.h"
#include "hier_transition.h"
#include "observer.h"
#include "popup_menu.h"
#include "window_utils.h"
#include "rename_edit.h"
#include "search.h"
#include "tool_tip.h"

using namespace std;

#define IDHK 1


class View : public Observer {

	enum { SEPA = 1, DIR = 2, HIDE = 4, LINK = 8, HIER = 16, SEL = 32, EMPTY = 64 };

	int mouseDownY_        = -1;
	int mouseDownArea_     = -1;
	int mouseDownIndex_    = -1;
	int mouseDownTopIndex_ = -1;
	Document::ListType mouseDownListType_ = Document::FILE;

	int scrollListTopIndex_ = 0;
	int listCursorIndex_    = 0;
	Document::ListType listCursorSwitch_ = Document::FILE;

	int cxSide_ = 0, cyItem_ = 0, cxScrollBar_ = 0;
	HFONT hMarkFont_ = nullptr, hItemFont_ = nullptr;
	int scrollListLineNum_ = 0;
	RECT listRect_;

	Pref pref_;
	int popupPos_;
	bool fullScreenCheck_;

	static int& menuTop_() {
		static int menuTop_;
		return menuTop_;
	}
	bool curSelIsLong_ = false, suppressPopup_ = false;
	UINT cursorAlign_ = 0;
	HWND hWnd_;
	double dpiFactX_, dpiFactY_;

	HierTransition ht_;
	Document doc_;
	TypeTable extentions_;
	Selection ope_;
	RenameEdit re_;
	Search search_;
	ToolTip tt_;

	// Procedure
	static LRESULT CALLBACK wndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
	{
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
		case WM_ENDSESSION:        view->doc_.Finalize(); break;
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

	static LRESULT CALLBACK menuProc(HWND hMenu, UINT msg, WPARAM wp, LPARAM lp)
	{
		switch (msg) {
		case WM_WINDOWPOSCHANGING:
			auto pos = (WINDOWPOS*)lp;
			if ((pos->flags & SWP_NOSIZE) && pos->y < menuTop_()) {
				RECT r;
				::GetWindowRect(hMenu, &r);
				int h = ::GetSystemMetrics(SM_CYSCREEN);
				pos->y = h - (r.bottom - r.top);
			}
			return ::CallWindowProc((WNDPROC)::GetWindowLong(hMenu, GWL_USERDATA), hMenu, msg, wp, lp);
		}
		return ::CallWindowProc((WNDPROC)::GetWindowLong(hMenu, GWL_USERDATA), hMenu, msg, wp, lp);
	}

	// Constructor
	View(const HWND hWnd) : doc_(extentions_, pref_, SEPA, (SEPA | HIER)), ope_(extentions_, pref_), re_(WM_RENAMEEDITCLOSED)
	{
		doc_.SetView(this);

		hWnd_ = hWnd;
		::SetWindowLong(hWnd, GWL_USERDATA, (LONG)this);  // Set the window a property
		ope_.SetWindowHandle(hWnd);
		re_.Initialize(hWnd);
		tt_.Initialize(hWnd);

		auto dpi = WindowUtils::GetDPI(hWnd_);
		dpiFactX_ = dpi.x / 96.0;
		dpiFactY_ = dpi.y / 96.0;

		// Whether to make multi-user (call first)
		if (pref_.item_int(SECTION_WINDOW, KEY_MULTI_USER, VAL_MULTI_USER)) pref_.set_multi_user_mode();
		loadPropData(true);  // Read and set from Ini file

		::SetTimer(hWnd_, 1, 300, nullptr);
	}

	// Read INI file
	void loadPropData(const bool firstTime)
	{
		pref_.set_current_section(SECTION_WINDOW);

		cxSide_      = (int)(pref_.item_int(KEY_SIDE_AREA_WIDTH, VAL_SIDE_AREA_WIDTH) * dpiFactX_);
		cyItem_      = (int)(pref_.item_int(KEY_LINE_HEIGHT,     VAL_LINE_HEIGHT)     * dpiFactY_);
		cxScrollBar_ = (int)(6 * dpiFactX_);

		popupPos_        = pref_.item_int(KEY_POPUP_POSITION, VAL_POPUP_POSITION);
		fullScreenCheck_ = pref_.item_int(KEY_FULL_SCREEN_CHECK, VAL_FULL_SCREEN_CHECK) != 0;

		int width  = (int)(pref_.item_int(KEY_WIDTH,  VAL_WIDTH)  * dpiFactX_);
		int height = (int)(pref_.item_int(KEY_HEIGHT, VAL_HEIGHT) * dpiFactY_);

		wstring defOpener = pref_.item(KEY_NO_LINKED, VAL_NO_LINKED);
		wstring hotKey    = pref_.item(KEY_POPUP_HOT_KEY, VAL_POPUP_HOT_KEY);

		wstring fontName = pref_.item(KEY_FONT_NAME, VAL_FONT_NAME);
		int     fontSize = (int)(pref_.item_int(KEY_FONT_SIZE, VAL_FONT_SIZE) * dpiFactX_);

		bool useMigemo = pref_.item_int(KEY_USE_MIGEMO, VAL_USE_MIGEMO) != 0;

		::MoveWindow(hWnd_, 0, 0, width, height, FALSE);
		::ShowWindow(hWnd_, SW_SHOW);  // Once display, and calculate the size etc.
		::ShowWindow(hWnd_, SW_HIDE);  // Hide immediately

		ope_.SetDefaultOpener(defOpener);
		SetHotkey(hotKey, IDHK);

		::DeleteObject(hItemFont_);
		hItemFont_ = ::CreateFont(fontSize, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE, fontName.c_str());
		if (fontName.empty() || !hItemFont_) hItemFont_ = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
		hMarkFont_ = ::CreateFont((int)(14 * dpiFactX_), 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, SYMBOL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _T("Marlett"));
		re_.SetFont(hItemFont_);

		search_.Initialize(useMigemo);
		extentions_.restore(pref_);  // Load extension color

		doc_.Initialize(firstTime);
	}

	void SetHotkey(const wstring& key, int id)
	{
		UINT flag = 0;
		if (key.size() < 5) return;
		if (key[0] == _T('1')) flag |= MOD_ALT;
		if (key[1] == _T('1')) flag |= MOD_CONTROL;
		if (key[2] == _T('1')) flag |= MOD_SHIFT;
		if (key[3] == _T('1')) flag |= MOD_WIN;
		if (flag) RegisterHotKey(hWnd_, id, flag, key[4]);
	}

	~View()
	{
		re_.Finalize();

		RECT rw;
		GetWindowRect(hWnd_, &rw);
		pref_.set_current_section(SECTION_WINDOW);
		pref_.set_item_int(KEY_WIDTH, (int)((rw.right - rw.left) / dpiFactX_));
		pref_.set_item_int(KEY_HEIGHT, (int)((rw.bottom - rw.top) / dpiFactY_));

		doc_.Finalize();
		::UnregisterHotKey(hWnd_, IDHK);  // Cancel hot key
		::DeleteObject(hMarkFont_);
		::DeleteObject(hItemFont_);

		::PostQuitMessage(0);
	}

	void wmClose()
	{
		if (WindowUtils::CtrlPressed()) {
			::ShowWindow(hWnd_, SW_HIDE);
			loadPropData(false);
			::ShowWindow(hWnd_, SW_SHOW);
		}
		else {
			::DestroyWindow(hWnd_);
		}
	}

	void wmDpiChanged(int dpiX, int dpiY) {
		dpiFactX_ = dpiX / 96;
		dpiFactY_ = dpiY / 96;

		auto visible = ::IsWindowVisible(hWnd_);
		if (visible) {
			::ShowWindow(hWnd_, SW_HIDE);
		}
		loadPropData(false);
		if (visible) {
			::ShowWindow(hWnd_, SW_SHOW);
		}
	}

	void wmWindowPosChanging(WINDOWPOS *wpos)
	{
		int edge = ::GetSystemMetrics(SM_CYSIZEFRAME);
		int upNC = ::GetSystemMetrics(SM_CYSMCAPTION) + edge;

		wpos->cy = (wpos->cy + cyItem_ / 2 - edge - upNC) / cyItem_ * cyItem_ + edge + upNC;
		if (wpos->cy - edge - upNC < cyItem_ * 8) wpos->cy = cyItem_ * 8 + edge + upNC;
		if (wpos->cx < 96) wpos->cx = 96;
	}

	void wmSize(int cwidth, int cheight)
	{
		::SetRect(&listRect_, 0, 0, cwidth - cxScrollBar_, cheight);
		scrollListLineNum_ = (cheight - doc_.GetNavis().Count() * cyItem_) / cyItem_;

		const ItemList& files = doc_.GetFiles();

		if (scrollListLineNum_ <= files.Count() && scrollListTopIndex_ > files.Count() - scrollListLineNum_) {
			setScrollListTopIndex(files.Count() - scrollListLineNum_);
		}
	}

	void wmHotKey(int id)
	{
		if (id == IDHK) {
			if (::IsWindowVisible(hWnd_)) suppressPopup_ = true;
			::ShowWindow(hWnd_, ::IsWindowVisible(hWnd_) ? SW_HIDE : SW_SHOW);
		}
	}

	void wmRequestUpdate()
	{
		ope_.DoneRequest();
		doc_.Update();
	}

	void wmRenameEditClosed()
	{
		auto renamedPath = re_.GetRenamePath();
		auto newFileName = re_.GetNewFileName();
		auto ok = ope_.Rename(renamedPath, newFileName);
		auto newPath = Path::parent(renamedPath);
		newPath.append(_T("\\")).append(newFileName);
		if (ok && doc_.CurrentPath() == renamedPath) doc_.SetCurrentDirectory(newPath);
		else doc_.Update();
	}

	// Event handler of WM_*MENULOOP
	void wmMenuLoop(bool enter)
	{
		auto hMenu = ::FindWindow(TEXT("#32768"), nullptr);

		if (!::IsWindow(hMenu)) return;
		if (enter) {
			::SetWindowLong(hMenu, GWL_USERDATA, (LONG)::GetWindowLong(hMenu, GWL_WNDPROC));
			::SetWindowLong(hMenu, GWL_WNDPROC, (LONG)View::menuProc);
		}
		else {
			::SetWindowLong(hMenu, GWL_WNDPROC, (LONG)::GetWindowLong(hMenu, GWL_USERDATA));
		}
	}

	void wmMouseWheel(int delta)
	{
		if (re_.IsActive()) return;  // Rejected while renaming
		setScrollListTopIndex(scrollListTopIndex_ - ((delta > 0) ? 3 : -3));
	}

	enum IconType { NONE, SQUARE, CIRCLE, SCIRCLE };

	void wmPaint()
	{
		RECT rc;
		PAINTSTRUCT ps;

		::GetClientRect(hWnd_, &rc);
		auto dc = ::BeginPaint(hWnd_, &ps);

		if (ps.rcPaint.right > rc.right - cxScrollBar_) drawScrollBar(dc);
		if (ps.rcPaint.left < rc.right - cxScrollBar_) {
			int begin = ps.rcPaint.top / cyItem_;
			int end = ps.rcPaint.bottom / cyItem_;
			RECT r;
			r.top = begin * cyItem_, r.bottom = r.top + cyItem_, r.left = 0, r.right = rc.right - cxScrollBar_;

			const ItemList &navis = doc_.GetNavis(), &files = doc_.GetFiles();

			for (int i = begin; i <= end; ++i) {
				if (i < navis.Count()) {
					const Item *fd = navis[i];
					if ((fd->data() & SEPA) != 0) {
						drawSeparator(dc, r, /*fd->IsHier()*/(fd->data() == (SEPA | HIER)));
					}
					else {
						DrawItem(dc, r, fd, listCursorSwitch_ == Document::HIER && i == listCursorIndex_);
					}
				}
				else if (i - navis.Count() + scrollListTopIndex_ < files.Count()) {
					int t = i - navis.Count() + scrollListTopIndex_;
					DrawItem(dc, r, files[t], listCursorSwitch_ == Document::FILE && t == listCursorIndex_);
				}
				else {
					::FillRect(dc, &r, (HBRUSH)(COLOR_MENU + 1));
				}
				r.top += cyItem_, r.bottom += cyItem_;
			}
		}
		::EndPaint(hWnd_, &ps);
	}

	// Draw a scroll bar
	void drawScrollBar(HDC dc) {
		RECT rc;
		const ItemList& files = doc_.GetFiles();

		::GetClientRect(hWnd_, &rc);
		rc.left = rc.right - cxScrollBar_;
		if (files.Count() <= scrollListLineNum_) {
			::FillRect(dc, &rc, (HBRUSH)(COLOR_MENU + 1));
			return;
		}
		double d = (double)rc.bottom / files.Count();
		RECT t = rc;
		t.bottom = (LONG)(d * scrollListTopIndex_);
		::FillRect(dc, &t, (HBRUSH)(COLOR_BTNSHADOW + 1));
		if (scrollListLineNum_ + scrollListTopIndex_ < files.Count()) {
			t.top = t.bottom;
			t.bottom += (LONG)(d * scrollListLineNum_);
			::FillRect(dc, &t, (HBRUSH)(COLOR_MENU + 1));
			t.top = t.bottom;
			t.bottom = rc.bottom;
			::FillRect(dc, &t, (HBRUSH)(COLOR_BTNSHADOW + 1));
		}
		else {
			t.top = t.bottom;
			t.bottom = rc.bottom;
			::FillRect(dc, &t, (HBRUSH)(COLOR_MENU + 1));
		}
	}

	// Draw a separator
	void drawSeparator(HDC dc, RECT r, bool isHier)
	{
		TCHAR str[4], *sortBy = _T("nedsNEDS");
		SIZE font;
		const ItemList& files = doc_.GetFiles();

		::FillRect(dc, &r, (HBRUSH)(COLOR_MENU + 1));
		::SelectObject(dc, hItemFont_);  // Font selection (do here because we measure the size below)
		if (isHier) {
			wstring num;
			if (doc_.SelectedCount()) {
				if (doc_.SelectedCount() == files.Count()) {
					num.assign(_T("ALL / "));
				}
				else {
					num.assign(std::to_wstring(doc_.SelectedCount())).append(_T(" / "));
				}
			}
			num.append(std::to_wstring(files.Count()));
			::GetTextExtentPoint32(dc, num.c_str(), num.size(), &font);
			RECT nr = r;
			nr.left = nr.right - font.cx - 3;
			WindowUtils::DrawGrayText(dc, nr, num.c_str());
			r.right = nr.left - 1;
			str[0] = sortBy[doc_.GetOpt().GetSortType() + doc_.GetOpt().GetSortOrder() * 4], str[1] = _T('\0');

			r.left += 3;
			WindowUtils::DrawGrayText(dc, r, str);
			::GetTextExtentPoint32(dc, str, ::_tcslen(str), &font);
			r.left = font.cx + 2;
		}
		if (doc_.InHistory()) {
			r.left += 3;
			WindowUtils::DrawGrayText(dc, r, _T("x"));
			::GetTextExtentPoint32(dc, _T("x"), 1, &font);
			r.left = font.cx + 2;
		}
		::InflateRect(&r, -3, cyItem_ / -2);
		::DrawEdge(dc, &r, EDGE_ETCHED, BF_TOP);
	}

	// Draw an item
	void DrawItem(HDC dc, RECT r, const Item *fd, bool cur)
	{
		::FillRect(dc, &r, (HBRUSH)((cur ? COLOR_HIGHLIGHT : COLOR_MENU) + 1));  // Draw the background
		if (fd->IsEmpty()) {
			::SetTextColor(dc, GetSysColor(COLOR_GRAYTEXT));
			::SetBkMode(dc, TRANSPARENT);
			::SelectObject(dc, hItemFont_);
			r.left = cxSide_;
			curSelIsLong_ = false;
			::DrawText(dc, _T("empty"), -1, &r, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
			return;
		}
		IconType type;
		int color = fd->Color();
		if (fd->IsLink()) {  // Is shortcut?
			if (color == -1) color = ::GetSysColor(COLOR_GRAYTEXT), type = SCIRCLE;
			else type = CIRCLE;
		}
		else if (color != -1) {
			type = SQUARE;
		}
		else {
			type = NONE;
		}
		drawMark(dc, r, type, color, cur, fd->IsSelected(), fd->IsDir());

		// Set text color and draw file name
		if (fd->IsHidden()) color = COLOR_GRAYTEXT;
		else color = (cur ? COLOR_HIGHLIGHTTEXT : COLOR_MENUTEXT);
		::SetTextColor(dc, GetSysColor(color));
		::SetBkMode(dc, TRANSPARENT);
		::SelectObject(dc, hItemFont_);
		r.left = cxSide_;
		if (fd->IsDir()) r.right -= cxSide_;
		SIZE font;
		if (cur) {
			::GetTextExtentPoint32(dc, fd->Name().c_str(), fd->Name().size(), &font);
			curSelIsLong_ = font.cx > r.right - r.left;  // File name at cursor position is out
		}
		::DrawText(dc, fd->Name().c_str(), -1, &r, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | (cursorAlign_ * curSelIsLong_ * cur));
	}

	// Draw a mark
	void drawMark(HDC dc, RECT r, IconType type, int color, bool cur, bool sel, bool dir)
	{
		TCHAR *c = nullptr;
		RECT rl = r, rr = r;

		rl.right = cxSide_, rr.left = r.right - cxSide_;
		::SetBkMode(dc, TRANSPARENT);
		::SelectObject(dc, hMarkFont_);  // Select font for symbols
		if (type == SQUARE)       c = _T("g");
		else if (type == CIRCLE)  c = _T("n");
		else if (type == SCIRCLE) c = _T("i");
		if (c) {
			::SetTextColor(dc, color);
			::DrawText(dc, c, 1, &rl, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
		}
		// Draw selection marks and folder marks
		::SetTextColor(dc, ::GetSysColor(cur ? COLOR_HIGHLIGHTTEXT : COLOR_MENUTEXT));
		if (sel) ::DrawText(dc, _T("a"), 1, &rl, 0x0025);
		if (dir) ::DrawText(dc, _T("4"), 1, &rr, 0x0025);
	}

	void wmTimer()
	{
		if (::IsWindowVisible(hWnd_)) {
			if (hWnd_ != ::GetForegroundWindow()) {  // If the window is displayed but somehow it is not the front
				DWORD id, fid;
				::GetWindowThreadProcessId(hWnd_, &id);
				::GetWindowThreadProcessId(::GetForegroundWindow(), &fid);
				if (id != fid) WindowUtils::ForegroundWindow(hWnd_);  // Bring the window to the front
			}
			// Search delay processing
			if (search_.IsReserved()) {
				setCursorIndex(search_.FindFirst(listCursorIndex_, doc_.GetFiles()), Document::FILE);
			}
			return;
		}
		// Pop-up function with mouse_
		int r = ::GetSystemMetrics(SM_CXSCREEN) - 1;
		int b = ::GetSystemMetrics(SM_CYSCREEN) - 1;
		POINT pt;
		::GetCursorPos(&pt);
		if (!(
			(popupPos_ == 0 && pt.x <= 1 && pt.y <= 1) || (popupPos_ == 1 && pt.x == r && pt.y == 0) ||
			(popupPos_ == 2 && pt.x == r && pt.y == b) || (popupPos_ == 3 && pt.x == 0 && pt.y == b)
			)) {
			suppressPopup_ = false;
			return;
		}
		if (suppressPopup_) return;
		// Does not pop up while dragging
		if (::GetAsyncKeyState(VK_LBUTTON) < 0 || ::GetAsyncKeyState(VK_RBUTTON) < 0) return;
		// Does not pop up if the current frontmost window is full screen
		if (fullScreenCheck_ && WindowUtils::IsForegroundWindowFullScreen()) return;
		::ShowWindow(hWnd_, SW_SHOW);
	}

	void wmShowWindow(BOOL show)
	{
		if (show) {
			doc_.Update();
			WindowUtils::MoveWindowToCorner(hWnd_, popupPos_);
			WindowUtils::ForegroundWindow(hWnd_);  // Bring the window to the front
		}
		else {
			ht_.clearIndexes();
			re_.Close();
		}
	}

	// Event Handler of WM_*BUTTONDOWN
	void wmButtonDown(int vkey, int x, int y)
	{
		if (re_.IsActive()) return;  // Reject while renaming

		mouseDownY_        = y;
		mouseDownArea_     = getItemArea(x);
		mouseDownIndex_    = lineToIndex(y / cyItem_, mouseDownListType_);
		mouseDownTopIndex_ = scrollListTopIndex_;

		if (mouseDownArea_ == 1) {  // Scroller
			SetCapture(hWnd_);
			return;
		}
		if (mouseDownIndex_ == -1) return;

		// Check button long press
		POINT pt, gp;
		auto timeOut = ::GetTickCount64() + 500;
		::GetCursorPos(&gp);
		while (timeOut > ::GetTickCount64()) {
			::GetCursorPos(&pt);
			if (abs(pt.x - gp.x) > 2 || abs(pt.y - gp.y) > 2) return;
			if (!(::GetAsyncKeyState(vkey) < 0)) return;
		}
		if (vkey == VK_LBUTTON || vkey == VK_RBUTTON) action(COM_START_DRAG, mouseDownListType_, mouseDownIndex_);
		mouseDownY_ = mouseDownArea_ = mouseDownIndex_ = mouseDownTopIndex_ = -1;
	}

	void wmMouseMove(int mkey, int x, int y)
	{
		if (re_.IsActive()) return;  // Reject while renaming

		if (mouseDownY_ != -1 && mouseDownArea_ == 1) {  // Scroller
			int t = (int)(mouseDownTopIndex_ - (mouseDownY_ - y) / ((double)listRect_.bottom / doc_.GetFiles().Count()));
			setScrollListTopIndex(t);
			return;
		}
		Document::ListType type;
		int cursor = lineToIndex(y / cyItem_, type);
		int dir = pointerMovingDir(x, y);
		int area = getItemArea(x);
		static int lastArea = 2;
		static ULONGLONG lastTime;

		if (area == 2) {  // The mouse moved on the item
			if (cursor != listCursorIndex_) {
				setCursorIndex(cursor, type);
			}
			else {
				if (lastArea == 2) itemAlignChange(x, y);
				else if (::GetTickCount64() - lastTime < 400) {
					if (lastArea == 0 && dir == 1 && ht_.canGoBack()) {
						doc_.SetCurrentDirectory(ht_.goBack());
					}
					else if (lastArea == 1 && dir == 0 && doc_.MovableToLower(type, listCursorIndex_)) {
						ht_.goForward(scrollListTopIndex_, doc_.CurrentPath());
						doc_.MoveToLower(type, listCursorIndex_);
					}
				}
			}
			lastArea = 2;
			return;
		}
		// Sidebar processing
		if (mkey == 0) {  // The button is not pressed
			if (lastArea == 2) {
				lastTime = ::GetTickCount64();
				if (area == 0 && dir == 0) lastArea = 0;  // Can return even if the cursor is not active
				else if (area == 1 && dir == 1 && listCursorIndex_ != -1) lastArea = 1;
			}
		}
		else if (!(mkey & MK_MBUTTON)) {  // L or R button
			if (dir == 2 || dir == 3 || listCursorIndex_ == -1) return;
			if (type == Document::FILE && mouseDownIndex_ != listCursorIndex_ && type == mouseDownListType_) {
				selectFile(mouseDownIndex_, listCursorIndex_);
			}
			if (mkey & MK_LBUTTON) action(COM_POPUP_INFO, type, listCursorIndex_);
			else if (WindowUtils::CtrlPressed()) action(COM_SHELL_MENU, type, listCursorIndex_);
			else popupMenu(type, listCursorIndex_);
			lastArea = -1;
		}
	}

	// Sense the direction the pointer is moving
	int pointerMovingDir(int x, int y) {
		static int lastX, lastY;
		int dir;

		int w = abs(x - lastX), h = abs(y - lastY);
		if (w > h)      dir = (x - lastX < 0) ? 0 : 1;
		else if (w < h) dir = (y - lastY < 0) ? 2 : 3;
		else           dir = -1;
		lastX = x, lastY = y;
		return dir;
	}

	// Right justify file name by cursor position
	void itemAlignChange(int x, int y) {
		if (!curSelIsLong_) return;
		RECT r = listRect_;
		unsigned int align = 0;
		if (r.right / 4 * 3 < x) align = DT_RIGHT;
		if (align != cursorAlign_) {  // Redraw
			cursorAlign_ = align;
			r.top = y / cyItem_ * cyItem_;
			r.bottom = r.top + cyItem_;
			::InvalidateRect(hWnd_, &r, FALSE);
			::UpdateWindow(hWnd_);
		}
	}

	// Event Handler of WM_*BUTTONUP
	void wmButtonUp(int vkey, int x, int y, int mkey)
	{
		if (re_.IsActive()) {
			re_.Close();
			return;  // Reject while renaming
		}
		if (mouseDownArea_ == 1) {  // Scroller
			ReleaseCapture();
			mouseDownY_ = mouseDownArea_ = mouseDownIndex_ = mouseDownTopIndex_ = -1;
			return;
		}
		Document::ListType type;
		int cursor = lineToIndex(y / cyItem_, type);
		if (
			cursor == -1 ||
			separatorClick(vkey, cursor, x, type) ||
			(type == Document::FILE && doc_.GetFiles()[0]->IsEmpty()) ||
			(listCursorIndex_ == -1 || mouseDownIndex_ == -1) ||
			type != mouseDownListType_ ||
			getItemArea(x) != 2
			) {
			mouseDownY_ = mouseDownArea_ = mouseDownIndex_ = mouseDownTopIndex_ = -1;
			return;
		}

		bool lbtn = (mkey & MK_LBUTTON) != 0;
		bool rbtn = (mkey & MK_RBUTTON) != 0;

		if ((vkey == VK_RBUTTON && lbtn) || (vkey == VK_LBUTTON && rbtn)) {
			if (type == Document::FILE && listCursorIndex_ != mouseDownIndex_) selectFile(mouseDownIndex_, listCursorIndex_);
			action(COM_SHELL_MENU, type, listCursorIndex_);

		}
		else if (vkey == VK_LBUTTON) {
			if (type == Document::FILE && (listCursorIndex_ != mouseDownIndex_ || WindowUtils::CtrlPressed())) {
				selectFile(mouseDownIndex_, listCursorIndex_);
			}
			else action(COM_OPEN, type, listCursorIndex_);
		}
		else if (vkey == VK_RBUTTON) {
			if (type == Document::FILE && listCursorIndex_ != mouseDownIndex_) selectFile(mouseDownIndex_, listCursorIndex_);
			if (WindowUtils::CtrlPressed()) action(COM_SHELL_MENU, type, listCursorIndex_);
			else popupMenu(type, listCursorIndex_);
		}
		else if (vkey == VK_MBUTTON) {
			if (type == Document::FILE && listCursorIndex_ != mouseDownIndex_ && doc_.ArrangeFavorites(mouseDownIndex_, listCursorIndex_)) {
				doc_.Update();
			}
			else action(COM_FAVORITE, type, listCursorIndex_);
		}
		mouseDownY_ = mouseDownArea_ = mouseDownIndex_ = mouseDownTopIndex_ = -1;
	}

	// Check the position of the pointer
	int getItemArea(int x)
	{
		if (x < cxSide_) return 0;
		if (x >= listRect_.right - cxSide_) return 1;
		return 2;
	}

	// Click on the separator
	bool separatorClick(int vkey, int index, int x, Document::ListType type)
	{
		if ((doc_.GetItem(type, index)->data() & SEPA) == 0) return false;

		if (doc_.InHistory()) {  // Click history separator
			if (vkey == VK_LBUTTON) action(COM_CLEAR_HISTORY, type, index);
			return true;
		}
		if (doc_.GetItem(type, index)->data() == (SEPA | HIER)) {  // Click the hierarchy separator
			if (listRect_.right * 2 / 3 < x) {  // If it is more than two thirds
				selectFile(0, doc_.GetFiles().Count() - 1);
			}
			else {
				int sortBy = doc_.GetOpt().GetSortType();
				switch (vkey) {
				case VK_LBUTTON:
					if (++sortBy > 3) sortBy = 0;
					doc_.GetOpt().SetSortType(sortBy);
					break;
				case VK_RBUTTON: doc_.GetOpt().SetSortOrder(!doc_.GetOpt().GetSortOrder()); break;
				case VK_MBUTTON: doc_.GetOpt().SetShowHidden(!doc_.GetOpt().IsShowHidden()); break;
				}
				ht_.setIndex(0U);
				doc_.Update();
			}
		}
		return true;
	}

	void wmKeyDown(int key)
	{
		bool ctrl = WindowUtils::CtrlPressed();
		if (ctrl || key == VK_APPS || key == VK_DELETE || key == VK_RETURN) {
			if (listCursorIndex_ == -1) return;
			if (ctrl) {
				if (key == VK_APPS) action(COM_SHELL_MENU, listCursorSwitch_, listCursorIndex_);
				else if (_T('A') <= key && key <= _T('Z')) {
					accelerator((char)key, listCursorSwitch_, listCursorIndex_);
				}
			}
			else {
				switch (key) {
				case VK_APPS:   popupMenu(listCursorSwitch_, listCursorIndex_); break;
				case VK_DELETE: action(COM_DELETE, listCursorSwitch_, listCursorIndex_); break;
				case VK_RETURN: action(COM_OPEN, listCursorSwitch_, listCursorIndex_); break;
				}
			}
		}
		else {
			if (key == VK_F3) {
				setCursorIndex(search_.FindNext(listCursorIndex_, doc_.GetFiles()), Document::FILE);
				return;
			}
			keyCursor(key);  // Cursor movement by key operation
			if (_T('A') <= key && key <= _T('Z')) search_.KeySearch(key);  // Key input search
		}
	}

	// TODO Allow cursor movement to the navigation pane
	// Cursor key input
	void keyCursor(int key)
	{
		int index = listCursorIndex_;
		const ItemList& files = doc_.GetFiles();

		if (index == -1 || listCursorSwitch_ == Document::HIER) {
			setCursorIndex(scrollListTopIndex_, Document::FILE);
			return;
		}
		switch (key) {
		case VK_SPACE:  // It's not a cursor but it looks like it
			selectFile(index, index);  // Through
		case VK_DOWN:
			index++;
			if (index >= files.Count()) index = 0;
			if ((doc_.GetItem(Document::FILE, index)->data() & SEPA) != 0) index++;
			setCursorIndex(index, Document::FILE);
			return;
		case VK_UP:
			index--;
			if (index < 0) index = files.Count() - 1;
			if ((doc_.GetItem(Document::FILE, index)->data() & SEPA) != 0) index--;
			setCursorIndex(index, Document::FILE);
			return;
		}
		if (key == VK_LEFT || key == VK_RIGHT) {
			if (key == VK_LEFT) {
				if (!ht_.canGoBack()) return;
				doc_.SetCurrentDirectory(ht_.goBack());
			}
			else {
				if (!doc_.MovableToLower(Document::FILE, index)) return;
				ht_.goForward(scrollListTopIndex_, doc_.CurrentPath());
				doc_.MoveToLower(Document::FILE, index);
			}
			setCursorIndex(scrollListTopIndex_, Document::FILE);
		}
	}

	// Specify cursor position
	void setCursorIndex(int index, Document::ListType type)
	{
		if (index != -1 && type == Document::FILE) {
			if (index < scrollListTopIndex_) setScrollListTopIndex(index, false);
			else if (scrollListTopIndex_ + scrollListLineNum_ <= index) setScrollListTopIndex(index - scrollListLineNum_ + 1, false);
		}
		if (listCursorIndex_ != -1 && (listCursorSwitch_ != type || listCursorIndex_ != index)) {
			RECT r = listRect_;
			r.top = indexToLine(listCursorIndex_, listCursorSwitch_) * cyItem_;
			r.bottom = r.top + cyItem_;
			::InvalidateRect(hWnd_, &r, FALSE);
		}
		if (index == -1 || (doc_.GetItem(type, index)->data() & SEPA) != 0) {
			listCursorIndex_ = -1;
			tt_.Inactivate();  // Hide tool tip
			::UpdateWindow(hWnd_);
			return;
		}
		if (listCursorSwitch_ != type || listCursorIndex_ != index) {
			listCursorIndex_ = index;
			listCursorSwitch_ = type;
			RECT r = listRect_;
			r.top = indexToLine(index, type) * cyItem_;
			r.bottom = r.top + cyItem_;
			::InvalidateRect(hWnd_, &r, FALSE);
		}
		if (listCursorSwitch_ != type) mouseDownIndex_ = -1;
		::UpdateWindow(hWnd_);  // Update here as curSelIsLong_ is referred below
		tt_.Inactivate();  // Hide tool tip
		if (curSelIsLong_) {  // Show tool tip
			tt_.Activate(doc_.GetItem(type, index)->Name(), listRect_);
		}
		else if (doc_.InBookmark() || doc_.InHistory()) {
			tt_.Activate(doc_.GetItem(type, index)->Path(), listRect_);
		}
	}

	// Specify the start index of the scroll list
	void setScrollListTopIndex(int i, bool update = true)
	{
		const ItemList& files = doc_.GetFiles();

		int old = scrollListTopIndex_;
		if (i < 0) scrollListTopIndex_ = 0;
		else if (files.Count() <= scrollListLineNum_) scrollListTopIndex_ = 0;
		else if (files.Count() - i < scrollListLineNum_) scrollListTopIndex_ = files.Count() - scrollListLineNum_;
		else scrollListTopIndex_ = i;

		RECT r;
		GetClientRect(hWnd_, &r);
		r.right -= cxScrollBar_;
		r.top = cyItem_ * doc_.GetNavis().Count();
		::ScrollWindow(hWnd_, 0, (old - scrollListTopIndex_) * cyItem_, &r, &r);
		GetClientRect(hWnd_, &r);
		r.left = r.right - cxScrollBar_;
		::InvalidateRect(hWnd_, &r, FALSE);
		if (update) ::UpdateWindow(hWnd_);
	}

	// Display pop-up menu and execute command
	void popupMenu(Document::ListType w, int index)
	{
		doc_.SetOperator(index, w, ope_);
		if (ope_.Count() == 0 || ope_[0].empty()) return;  // Reject if objs is empty
		auto ext = FileSystem::is_directory(ope_[0]) ? PATH_EXT_DIR : Path::ext(ope_[0]);
//		auto ext = Path::ext(ope_[0], true);
		int type = extentions_.get_id(ext) + 1;
		UINT f;
		POINT pt = popupPt(w, index, f);
		wstring cmd;
		PopupMenu pm(hWnd_, &pref_);

		std::vector<wstring> items;
		if (pm.popup(type, pt, f, cmd, items)) action(ope_, cmd, w, index);
	}

	// Execution of command by accelerator
	void accelerator(char accelerator, Document::ListType w, int index)
	{
		doc_.SetOperator(index, w, ope_);
		if (ope_.Count() == 0 || ope_[0].empty()) return;  // Reject if objs is empty
		auto ext = FileSystem::is_directory(ope_[0]) ? PATH_EXT_DIR : Path::ext(ope_[0]);
		int type = extentions_.get_id(ext) + 1;
		wstring cmd;
		PopupMenu pm(hWnd_, &pref_);
		if (pm.getAccelCommand(type, accelerator, cmd)) action(ope_, cmd, w, index);
	}

	// Command execution
	void action(const wstring& cmd, Document::ListType w, int index)
	{
		doc_.SetOperator(index, w, ope_);
		action(ope_, cmd, w, index);
	}

	void action(const Selection &objs, const wstring& cmd, Document::ListType w, int index) {
		bool hasObj = objs.Count() != 0 && !objs[0].empty();
		wstring oldCurrent;

		if (hasObj) {
			oldCurrent.assign(FileSystem::current_directory_path());
			::SetCurrentDirectory(Path::parent(objs[0]).c_str());
			doc_.SetHistory(objs[0]);
		}
		if (cmd[0] == _T('<')) {
			systemCommand(cmd, objs, w, index);
		}
		else {
			if (!hasObj) return;
			::ShowWindow(hWnd_, SW_HIDE);  // Hide in advance
			ope_.OpenBy(cmd);
		}
		if (hasObj) ::SetCurrentDirectory(oldCurrent.c_str());
	}

	// System command execution
	void systemCommand(const wstring& cmd, const Selection& objs, Document::ListType w, int index)
	{
		if (cmd == COM_SELECT_ALL)    { selectFile(0, doc_.GetFiles().Count() - 1, true); return; }
		if (cmd == COM_RENAME)        {
			re_.Open(objs[0], indexToLine(index, w) * cyItem_, listRect_.right, cyItem_);
			return;
		}
		if (cmd == COM_POPUP_INFO)    { popupInfo(objs, w, index); return; }
		if (cmd == COM_CLEAR_HISTORY) { doc_.ClearHistory(); return; }
		if (cmd == COM_FAVORITE)      { doc_.AddOrRemoveFavorite(objs[0], w, index); return; }  // Update here, so do nothing after return
		if (cmd == COM_START_DRAG)    { ::SetCursor(::LoadCursor(nullptr, IDC_NO)); ::ShowWindow(hWnd_, SW_HIDE); ope_.StartDrag(); return; }
		if (cmd == COM_SHELL_MENU)    {
			UINT f;
			POINT pt = popupPt(w, index, f);
			ope_.PopupShellMenu(pt, f);
			return;
		}
		if (ope_.Command(cmd) == -1) ::ShowWindow(hWnd_, SW_HIDE);
	}

	// Select file by range specification
	void selectFile(int front, int back, bool all = false)
	{
		if (front == -1 || back == -1 || doc_.InDrives()) return;
		if (back < front) std::swap(front, back);
		doc_.SelectFile(front, back, Document::FILE, all);

		if (front < scrollListTopIndex_) front = scrollListTopIndex_;
		if (back >= scrollListTopIndex_ + scrollListLineNum_) back = scrollListTopIndex_ + scrollListLineNum_ - 1;

		RECT lr = listRect_;
		lr.top = (front - scrollListTopIndex_ + doc_.GetNavis().Count()) * cyItem_;
		lr.bottom = (back - scrollListTopIndex_ + doc_.GetNavis().Count() + 1) * cyItem_;
		::InvalidateRect(hWnd_, &lr, FALSE);

		// Update selected file count display
		lr.top = (doc_.GetNavis().Count() - 1) * cyItem_;
		lr.bottom = lr.top + cyItem_;
		::InvalidateRect(hWnd_, &lr, FALSE);
		::UpdateWindow(hWnd_);
	}

	// Display file information
	void popupInfo(const Selection&, Document::ListType w, int index)
	{
		vector<wstring> items;
		ope_.InformationStrings(items);
		items.push_back(_T("...more"));
		UINT f;
		POINT pt = popupPt(w, index, f);

		HMENU hMenu = ::CreatePopupMenu();
		for (unsigned int i = 0; i < items.size(); ++i) {
			::AppendMenu(hMenu, MF_STRING, i, items[i].c_str());
		}
		int ret = ::TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_LEFTBUTTON | f, pt.x, pt.y, 0, hWnd_, nullptr);
		::DestroyMenu(hMenu);
		if (ret == (int)items.size() - 1) {
			ope_.PopupFileProperty();
		}
	}

	// Find the position of the popup
	POINT popupPt(Document::ListType w, int index, UINT &f)
	{
		f = TPM_RETURNCMD;
		RECT r;
		::GetWindowRect(hWnd_, &r);

		int l = indexToLine(index, w);
		POINT pt = { 0, l == -1 ? 0 : l * cyItem_ };
		::ClientToScreen(hWnd_, &pt);
		menuTop_() = pt.y;
		if (popupPos_ == 0 || popupPos_ == 3) {
			pt.x = r.right - 6 - ::GetSystemMetrics(SM_CXFIXEDFRAME);
		}
		else {
			f |= TPM_RIGHTALIGN;
			pt.x = r.left + ::GetSystemMetrics(SM_CXFIXEDFRAME);
		}
		return pt;
	}

	// Return line from index and type
	int indexToLine(int index, Document::ListType type)
	{
		return index + (doc_.GetNavis().Count() - scrollListTopIndex_) * (type == Document::FILE);
	}

	// Return index and type from line
	int lineToIndex(int line, Document::ListType &type)
	{
		if (line < doc_.GetNavis().Count()) {
			type = Document::HIER;
			return line;
		}
		type = Document::FILE;
		int i = line - doc_.GetNavis().Count() + scrollListTopIndex_;
		return i < doc_.GetFiles().Count() ? i : -1;
	}

public:

	static BOOL InitApplication(HINSTANCE hInst, const wchar_t* className)
	{
		WNDCLASS wc;
		wc.style         = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc   = (WNDPROC)View::wndProc;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 0;
		wc.hInstance     = hInst;
		wc.hIcon         = nullptr;
		wc.hCursor       = ::LoadCursor(nullptr, IDC_ARROW);  // Mouse cursor (standard arrow)
		wc.hbrBackground = nullptr;
		wc.lpszMenuName  = nullptr;
		wc.lpszClassName = className;
		return ::RegisterClass(&wc);
	}

	// Window size position adjustment
	virtual void Updated()
	{
		setScrollListTopIndex(ht_.index());
		setCursorIndex(-1, Document::FILE);
		scrollListLineNum_ = (listRect_.bottom - doc_.GetNavis().Count() * cyItem_) / cyItem_;

		tt_.Inactivate();
		::InvalidateRect(hWnd_, nullptr, TRUE);
		::UpdateWindow(hWnd_);
	}

};
