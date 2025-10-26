/**
 * View
 *
 * @author Takuto Yanagida
 * @version 2025-10-26
 */

#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include <optional>
#include <cmath>
#include <locale>

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

constexpr auto IDHK = 1;

BOOL InitApplication(HINSTANCE hInst, const wchar_t* className);
LRESULT CALLBACK wndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);

class View : public Observer {

	static constexpr auto SEPA  = 1;
	static constexpr auto DIR   = 2;
	static constexpr auto HIDE  = 4;
	static constexpr auto LINK  = 8;
	static constexpr auto HIER  = 16;
	static constexpr auto SEL   = 32;
	static constexpr auto EMPTY = 64;

	static HFONT GetUiMessageFont() noexcept {
		NONCLIENTMETRICSW ncm{};
		ncm.cbSize = sizeof(ncm);
		if (!SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0)) {
			return nullptr;
		}
		return CreateFontIndirectW(&ncm.lfMessageFont);
	}

	int mouseDownY_        = -1;
	int mouseDownArea_     = -1;
	std::optional<size_t> mouseDownIndex_;
	std::optional<size_t> mouseDownTopIndex_;
	Document::ListType mouseDownListType_ = Document::ListType::FILE;

	size_t scrollListTopIndex_ = 0;
	std::optional<size_t> listCursorIndex_    = 0;
	Document::ListType listCursorSwitch_ = Document::ListType::FILE;

	int cxSide_ = 0, cyItem_ = 0, cxScrollBar_ = 0;
	HFONT hMarkFont_ = nullptr, hItemFont_ = nullptr;
	size_t scrollListLineNum_ = 0;
	RECT listRect_;

	Pref pref_;
	int popupPos_;
	bool fullScreenCheck_;

	static int& menuTop_() noexcept {
		static int menuTop_;
		return menuTop_;
	}
	bool curSelIsLong_ = false, suppressPopup_ = false;
	UINT cursorAlign_ = 0;
	const HWND hWnd_;
	double dpiFactX_, dpiFactY_;

	HierTransition ht_;
	Document doc_;
	TypeTable extensions_;
	Selection ope_;
	RenameEdit re_;
	Search search_;
	ToolTip tt_;

public:

	static LRESULT CALLBACK menuProc(HWND hMenu, UINT msg, WPARAM wp, LPARAM lp) noexcept {
		switch (msg) {
		case WM_WINDOWPOSCHANGING:
			{
				auto pos = reinterpret_cast<WINDOWPOS*>(lp);
				if ((pos->flags & SWP_NOSIZE) && pos->y < menuTop_()) {
					RECT r;
					::GetWindowRect(hMenu, &r);
					const int h = ::GetSystemMetrics(SM_CYSCREEN);
					pos->y = h - (r.bottom - r.top);
				}
				[[fallthrough]];
			}
		default: break;
		}
		auto proc = reinterpret_cast<WNDPROC>(::GetWindowLongPtr(hMenu, GWLP_USERDATA));
		return ::CallWindowProc(proc, hMenu, msg, wp, lp);
	}

	View(const HWND hWnd) : hWnd_(hWnd), doc_(extensions_, pref_, SEPA, (SEPA | HIER)), ope_(extensions_, pref_), re_(WM_RENAMEEDITCLOSED) {
		doc_.SetView(this);

		::SetWindowLongPtr(hWnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));  // Set the window a property
		ope_.SetWindowHandle(hWnd_);
		re_.Initialize(hWnd_);
		tt_.Initialize(hWnd_);

		const auto dpi = WindowUtils::GetDPI(hWnd_);
		dpiFactX_ = dpi.x / 96.0;
		dpiFactY_ = dpi.y / 96.0;

		// Whether to make multi-user (call first)
		if (pref_.item_int(SECTION_WINDOW, KEY_MULTI_USER, VAL_MULTI_USER)) pref_.set_multi_user_mode();
		loadPropData(true);  // Read and set from Ini file

		::SetTimer(hWnd_, 1, 300, nullptr);
	}

	// Read INI file
	void loadPropData(const bool firstTime) {
		pref_.set_current_section(SECTION_WINDOW);

		cxSide_      = std::lrint(pref_.item_int(KEY_SIDE_AREA_WIDTH, VAL_SIDE_AREA_WIDTH) * dpiFactX_);
		cyItem_      = std::lrint(pref_.item_int(KEY_LINE_HEIGHT,     VAL_LINE_HEIGHT)     * dpiFactY_);
		cxScrollBar_ = std::lrint(6 * dpiFactX_);

		popupPos_        = pref_.item_int(KEY_POPUP_POSITION, VAL_POPUP_POSITION);
		fullScreenCheck_ = pref_.item_int(KEY_FULL_SCREEN_CHECK, VAL_FULL_SCREEN_CHECK) != 0;

		const int width  = std::lrint(pref_.item_int(KEY_WIDTH,  VAL_WIDTH)  * dpiFactX_);
		const int height = std::lrint(pref_.item_int(KEY_HEIGHT, VAL_HEIGHT) * dpiFactY_);

		const wstring defOpener = pref_.item(KEY_NO_LINKED, VAL_NO_LINKED);
		const wstring hotKey    = pref_.item(KEY_POPUP_HOT_KEY, VAL_POPUP_HOT_KEY);

		const wstring fontName = pref_.item(KEY_FONT_NAME, VAL_FONT_NAME);
		const int     fontSize = std::lrint(pref_.item_int(KEY_FONT_SIZE, VAL_FONT_SIZE) * dpiFactX_);

		const bool useMigemo = pref_.item_int(KEY_USE_MIGEMO, VAL_USE_MIGEMO) != 0;

		::MoveWindow(hWnd_, 0, 0, width, height, FALSE);
		::ShowWindow(hWnd_, SW_SHOW);  // Once display, and calculate the size etc.
		::ShowWindow(hWnd_, SW_HIDE);  // Hide immediately

		ope_.SetDefaultOpener(defOpener);
		SetHotkey(hotKey, IDHK);

		::DeleteObject(hItemFont_);
		hItemFont_ = ::CreateFont(fontSize, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE, fontName.c_str());
		if (fontName.empty() || !hItemFont_) {
			hItemFont_ = View::GetUiMessageFont();
		}
		hMarkFont_ = ::CreateFont(std::lrint(14 * dpiFactX_), 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, SYMBOL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _T("Marlett"));
		re_.SetFont(hItemFont_);

		search_.Initialize(useMigemo);
		extensions_.restore(pref_);  // Load extension color

		doc_.Initialize(firstTime);
	}

	void SetHotkey(const wstring& key, int id) const {
		UINT flag = 0;
		if (key.size() < 5) return;
		if (key.at(0) == _T('1')) flag |= MOD_ALT;
		if (key.at(1) == _T('1')) flag |= MOD_CONTROL;
		if (key.at(2) == _T('1')) flag |= MOD_SHIFT;
		if (key.at(3) == _T('1')) flag |= MOD_WIN;
		if (flag) RegisterHotKey(hWnd_, id, flag, key.at(4));
	}

	View(const View&) = delete;
	virtual View& operator=(const View&) = delete;
	View(View&&) = delete;
	virtual View& operator=(View&&) = delete;

	virtual ~View() {
		re_.Finalize();

		RECT rw;
		GetWindowRect(hWnd_, &rw);
		pref_.set_current_section(SECTION_WINDOW);
		pref_.set_item_int(KEY_WIDTH,  static_cast<int>((static_cast<__int64>(rw.right)  - rw.left) / dpiFactX_));
		pref_.set_item_int(KEY_HEIGHT, static_cast<int>((static_cast<__int64>(rw.bottom) - rw.top)  / dpiFactY_));

		doc_.Finalize();
		::UnregisterHotKey(hWnd_, IDHK);  // Cancel hot key
		::DeleteObject(hMarkFont_);
		::DeleteObject(hItemFont_);

		::PostQuitMessage(0);
	}

	void wmClose() {
		if (WindowUtils::CtrlPressed()) {
			::ShowWindow(hWnd_, SW_HIDE);
			loadPropData(false);
			::ShowWindow(hWnd_, SW_SHOW);
		} else {
			doc_.Finalize();
			::DestroyWindow(hWnd_);
		}
	}

	void wmDpiChanged(int dpiX, int dpiY) {
		dpiFactX_ = dpiX / 96.0;
		dpiFactY_ = dpiY / 96.0;

		const auto visible = ::IsWindowVisible(hWnd_);
		if (visible) {
			::ShowWindow(hWnd_, SW_HIDE);
		}
		loadPropData(false);
		if (visible) {
			::ShowWindow(hWnd_, SW_SHOW);
		}
	}

	void wmWindowPosChanging(WINDOWPOS *wpos) const noexcept {
		if (wpos == nullptr) {
			return;
		}
		const int edge = ::GetSystemMetrics(SM_CYSIZEFRAME);
		const int upNC = ::GetSystemMetrics(SM_CYSMCAPTION) + edge;

		wpos->cy = (wpos->cy + cyItem_ / 2 - edge - upNC) / cyItem_ * cyItem_ + edge + upNC;
		if (wpos->cy - edge - upNC < cyItem_ * 8) wpos->cy = cyItem_ * 8 + edge + upNC;
		if (wpos->cx < 96) wpos->cx = 96;
	}

	void wmSize(int cwidth, int cheight) noexcept {
		::SetRect(&listRect_, 0, 0, cwidth - cxScrollBar_, cheight);
		scrollListLineNum_ = (cheight - doc_.GetNavis().Count() * cyItem_) / cyItem_;

		const ItemList& files = doc_.GetFiles();

		if (scrollListLineNum_ <= files.Count() && scrollListTopIndex_ > files.Count() - scrollListLineNum_) {
			setScrollListTopIndex(files.Count() - scrollListLineNum_);
		}
	}

	void wmHotKey(WPARAM id) noexcept {
		if (hWnd_ == nullptr) {
			return;
		}
		if (id == IDHK) {
			if (::IsWindowVisible(hWnd_)) suppressPopup_ = true;
			::ShowWindow(hWnd_, ::IsWindowVisible(hWnd_) ? SW_HIDE : SW_SHOW);
		}
	}

	void wmEndSession() noexcept {
		doc_.Finalize();
	}

	void wmRequestUpdate() {
		ope_.DoneRequest();
		doc_.Update();
	}

	void wmRenameEditClosed() {
		auto& renamedPath = re_.GetRenamePath();
		auto& newFileName = re_.GetNewFileName();
		const auto ok = ope_.Rename(renamedPath, newFileName);
		auto newPath = Path::parent(renamedPath);
		newPath.append(_T("\\")).append(newFileName);
		if (ok && doc_.CurrentPath() == renamedPath) doc_.SetCurrentDirectory(newPath);
		else doc_.Update();
	}

	// Event handler of WM_*MENULOOP
	void wmMenuLoop(bool enter) noexcept {
		auto hMenu = ::FindWindow(TEXT("#32768"), nullptr);

		if (!::IsWindow(hMenu)) return;
		if (enter) {
			const auto proc = static_cast<LONG_PTR>(::GetWindowLongPtr(hMenu, GWLP_WNDPROC));
			::SetWindowLongPtr(hMenu, GWLP_USERDATA, proc);
			::SetWindowLongPtr(hMenu, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(View::menuProc));
		} else {
			const auto proc = static_cast<LONG_PTR>(::GetWindowLongPtr(hMenu, GWLP_USERDATA));
			::SetWindowLongPtr(hMenu, GWLP_WNDPROC, proc);
		}
	}

	void wmMouseWheel(int delta) noexcept {
		if (re_.IsActive()) return;  // Rejected while renaming
		if (delta > 0) {
			setScrollListTopIndex(3 <= scrollListTopIndex_ ? scrollListTopIndex_ - 3 : 0);
		}
		else {
			setScrollListTopIndex(scrollListTopIndex_ + 3);
		}
	}

	enum class IconType { NONE = 0, SQUARE = 1, CIRCLE = 2, SCIRCLE = 3 };

	void wmPaint() {
		RECT rc{};
		PAINTSTRUCT ps{};

		::GetClientRect(hWnd_, &rc);
		auto dc = ::BeginPaint(hWnd_, &ps);

		if (ps.rcPaint.right > rc.right - cxScrollBar_) drawScrollBar(dc);
		if (ps.rcPaint.left < rc.right - cxScrollBar_) {
			const size_t begin = ps.rcPaint.top / cyItem_;
			const size_t end   = ps.rcPaint.bottom / cyItem_;
			RECT r{};
			r.top    = static_cast<LONG>(begin * cyItem_);
			r.bottom = r.top + cyItem_;
			r.left   = 0;
			r.right  = rc.right - cxScrollBar_;

			const ItemList &navis = doc_.GetNavis(), &files = doc_.GetFiles();

			for (size_t i = begin; i <= end; ++i) {
				if (i < navis.Count()) {
					const Item *fd = navis[i];
					if (!fd) continue;
					if ((fd->data() & SEPA) != 0) {
						drawSeparator(dc, r, (fd->data() == (SEPA | HIER)));
					} else {
						DrawItem(dc, r, fd, listCursorSwitch_ == Document::ListType::HIER && i == listCursorIndex_);
					}
				} else if (i - navis.Count() + scrollListTopIndex_ < files.Count()) {
					const size_t t = i - navis.Count() + scrollListTopIndex_;
					DrawItem(dc, r, files[t], listCursorSwitch_ == Document::ListType::FILE && t == listCursorIndex_);
				} else {
					::FillRect(dc, &r, ::GetSysColorBrush(COLOR_MENU));
				}
				r.top += cyItem_, r.bottom += cyItem_;
			}
		}
		::EndPaint(hWnd_, &ps);
	}

	// Draw a scroll bar
	void drawScrollBar(HDC dc) noexcept {
		RECT rc{};
		const ItemList& files = doc_.GetFiles();

		::GetClientRect(hWnd_, &rc);
		rc.left = rc.right - cxScrollBar_;
		if (files.Count() <= scrollListLineNum_) {
			::FillRect(dc, &rc, ::GetSysColorBrush(COLOR_MENU));
			return;
		}
		const double d = static_cast<double>(rc.bottom) / files.Count();
		RECT t = rc;
		t.bottom = static_cast<LONG>(d * scrollListTopIndex_);
		::FillRect(dc, &t, ::GetSysColorBrush(COLOR_BTNSHADOW));
		if (scrollListLineNum_ + scrollListTopIndex_ < files.Count()) {
			t.top     = t.bottom;
			t.bottom += static_cast<LONG>(d * scrollListLineNum_);
			::FillRect(dc, &t, ::GetSysColorBrush(COLOR_MENU));
			t.top    = t.bottom;
			t.bottom = rc.bottom;
			::FillRect(dc, &t, ::GetSysColorBrush(COLOR_BTNSHADOW));
		} else {
			t.top    = t.bottom;
			t.bottom = rc.bottom;
			::FillRect(dc, &t, ::GetSysColorBrush(COLOR_MENU));
		}
	}

	// Draw a separator
	void drawSeparator(HDC dc, RECT r, bool isHier) {
		SIZE font{};
		const ItemList& files = doc_.GetFiles();

		::FillRect(dc, &r, ::GetSysColorBrush(COLOR_MENU));
		::SelectObject(dc, hItemFont_);  // Font selection (do here because we measure the size below)
		if (isHier) {
			wstring num;
			if (doc_.SelectedCount()) {
				if (doc_.SelectedCount() == files.Count()) {
					num.assign(_T("ALL / "));
				} else {
					num.assign(std::to_wstring(doc_.SelectedCount())).append(_T(" / "));
				}
			}
			num.append(std::to_wstring(files.Count()));
			::GetTextExtentPoint32(dc, num.c_str(), static_cast<int>(num.size()), &font);
			RECT nr = r;
			nr.left = nr.right - font.cx - 3;
			WindowUtils::DrawGrayText(dc, nr, num.c_str());
			r.right = nr.left - 1;

			TCHAR str[2]{};
			str[0] = doc_.GetOpt().GetSortTypeChar();

			r.left += 3;
			WindowUtils::DrawGrayText(dc, r, &str[0]);
			::GetTextExtentPoint32(dc, &str[0], 1, &font);
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
	void DrawItem(HDC dc, RECT r, const Item *fd, bool cur) noexcept {
		::FillRect(dc, &r, ::GetSysColorBrush(cur ? COLOR_HIGHLIGHT : COLOR_MENU));  // Draw the background
		if (!fd) return;
		if (fd->IsEmpty()) {
			::SetTextColor(dc, GetSysColor(COLOR_GRAYTEXT));
			::SetBkMode(dc, TRANSPARENT);
			::SelectObject(dc, hItemFont_);
			r.left = cxSide_;
			curSelIsLong_ = false;
			::DrawText(dc, _T("empty"), -1, &r, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
			return;
		}
		IconType type = IconType::NONE;
		int color = fd->Color();
		if (fd->IsLink()) {  // Is shortcut?
			if (color == -1) color = ::GetSysColor(COLOR_GRAYTEXT), type = IconType::SCIRCLE;
			else type = IconType::CIRCLE;
		} else if (color != -1) {
			type = IconType::SQUARE;
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
		SIZE font{};
		if (cur) {
			::GetTextExtentPoint32(dc, fd->Name().c_str(), static_cast<int>(fd->Name().size()), &font);
			curSelIsLong_ = font.cx > r.right - r.left;  // File name at cursor position is out
		}
		::DrawText(dc, fd->Name().c_str(), -1, &r, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | ((curSelIsLong_ && cur) ? cursorAlign_ : 0));
	}

	// Draw a mark
	void drawMark(HDC dc, RECT r, IconType type, int color, bool cur, bool sel, bool dir) const noexcept {
		TCHAR c[2]{};
		RECT rl = r, rr = r;

		rl.right = cxSide_, rr.left = r.right - cxSide_;
		::SetBkMode(dc, TRANSPARENT);
		::SelectObject(dc, hMarkFont_);  // Select font for symbols
		if (type == IconType::SQUARE)       c[0] = _T('g');
		else if (type == IconType::CIRCLE)  c[0] = _T('n');
		else if (type == IconType::SCIRCLE) c[0] = _T('i');
		if (c[0]) {
			::SetTextColor(dc, color);
			::DrawText(dc, &c[0], 1, &rl, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
		}
		// Draw selection marks and folder marks
		::SetTextColor(dc, ::GetSysColor(cur ? COLOR_HIGHLIGHTTEXT : COLOR_MENUTEXT));
		if (sel) ::DrawText(dc, _T("a"), 1, &rl, 0x0025);
		if (dir) ::DrawText(dc, _T("4"), 1, &rr, 0x0025);
	}

	void wmTimer() {
		if (::IsWindowVisible(hWnd_)) {
			if (hWnd_ != ::GetForegroundWindow()) {  // If the window is displayed but somehow it is not the front
				DWORD id, fid;
				::GetWindowThreadProcessId(hWnd_, &id);
				::GetWindowThreadProcessId(::GetForegroundWindow(), &fid);
				if (id != fid) WindowUtils::ForegroundWindow(hWnd_);  // Bring the window to the front
			}
			// Search delay processing
			if (search_.IsReserved()) {
				setCursorIndex(search_.FindFirst(listCursorIndex_, doc_.GetFiles()), Document::ListType::FILE);
			}
			return;
		}
		// Pop-up function with mouse_
		const int r = ::GetSystemMetrics(SM_CXSCREEN) - 1;
		const int b = ::GetSystemMetrics(SM_CYSCREEN) - 1;
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

	void wmShowWindow(bool show) {
		if (show) {
			doc_.Update();
			WindowUtils::MoveWindowToCorner(hWnd_, popupPos_);
			WindowUtils::ForegroundWindow(hWnd_);  // Bring the window to the front
		} else {
			ht_.clearIndexes();
			re_.Close();
		}
	}

	// Event Handler of WM_*BUTTONDOWN
	void wmButtonDown(int vkey, int x, int y) {
		if (re_.IsActive()) return;  // Reject while renaming

		mouseDownY_        = y;
		mouseDownArea_     = getItemArea(x);
		mouseDownIndex_    = lineToIndex(y / cyItem_, mouseDownListType_);
		mouseDownTopIndex_ = scrollListTopIndex_;

		if (mouseDownArea_ == 1) {  // Scroller
			SetCapture(hWnd_);
			return;
		}
		if (!mouseDownIndex_) return;

		// Check button long press
		POINT pt{}, gp{};
		const auto timeOut = ::GetTickCount64() + 500;
		::GetCursorPos(&gp);
		while (timeOut > ::GetTickCount64()) {
			::GetCursorPos(&pt);
			if (abs(pt.x - gp.x) > 2 || abs(pt.y - gp.y) > 2) return;
			if (!(::GetAsyncKeyState(vkey) < 0)) return;
		}
		if (vkey == VK_LBUTTON || vkey == VK_RBUTTON) action(COM_START_DRAG, mouseDownListType_, mouseDownIndex_);
		mouseDownY_ = mouseDownArea_ = -1;
		mouseDownIndex_.reset();
		mouseDownTopIndex_.reset();
	}

	void wmMouseMove(WPARAM mkey, int x, int y) {
		if (re_.IsActive()) return;  // Reject while renaming

		if (mouseDownY_ != -1 && mouseDownArea_ == 1) {  // Scroller
			const double t = mouseDownTopIndex_.value() - (static_cast<long long>(mouseDownY_) - y) * static_cast<double>(doc_.GetFiles().Count()) / listRect_.bottom;
			setScrollListTopIndex(std::lrint(max(0, t)));
			return;
		}
		Document::ListType type;
		const std::optional<size_t> cursor = lineToIndex(y / cyItem_, type);
		const int dir    = pointerMovingDir(x, y);
		const int area   = getItemArea(x);
		static int lastArea = 2;
		static ULONGLONG lastTime;

		if (area == 2) {  // The mouse moved on the item
			if (cursor != listCursorIndex_) {
				setCursorIndex(cursor, type);
			} else {
				if (lastArea == 2) itemAlignChange(x, y);
				else if (::GetTickCount64() - lastTime < 400) {
					if (lastArea == 0 && dir == 1 && ht_.canGoBack()) {
						doc_.SetCurrentDirectory(ht_.goBack());
					} else if (lastArea == 1 && dir == 0 && doc_.MovableToLower(type, listCursorIndex_.value())) {
						ht_.goForward(scrollListTopIndex_, doc_.CurrentPath());
						doc_.MoveToLower(type, listCursorIndex_.value());
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
				else if (area == 1 && dir == 1 && listCursorIndex_) lastArea = 1;
			}
		} else if (!(mkey & MK_MBUTTON)) {  // L or R button
			if (dir == 2 || dir == 3 || !listCursorIndex_) return;
			if (type == Document::ListType::FILE && mouseDownIndex_ != listCursorIndex_ && type == mouseDownListType_) {
				selectFile(mouseDownIndex_, listCursorIndex_);
			}
			if (mkey & MK_LBUTTON) action(COM_POPUP_INFO, type, listCursorIndex_);
			else if (WindowUtils::CtrlPressed()) action(COM_SHELL_MENU, type, listCursorIndex_);
			else popupMenu(type, listCursorIndex_);
			lastArea = -1;
		}
	}

	// Sense the direction the pointer is moving
	int pointerMovingDir(int x, int y) noexcept {
		static int lastX, lastY;
		int dir{};

		const int w = abs(x - lastX);
		const int h = abs(y - lastY);

		if (w > h)      dir = (x - lastX < 0) ? 0 : 1;
		else if (w < h) dir = (y - lastY < 0) ? 2 : 3;
		else            dir = -1;

		lastX = x;
		lastY = y;
		return dir;
	}

	// Right justify file name by cursor position
	void itemAlignChange(int x, int y) noexcept {
		if (!curSelIsLong_) return;
		RECT r = listRect_;
		UINT align = 0;
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
	void wmButtonUp(int vkey, int x, int y, WPARAM mkey) {
		if (re_.IsActive()) {
			re_.Close();
			return;  // Reject while renaming
		}
		if (mouseDownArea_ == 1) {  // Scroller
			ReleaseCapture();
			mouseDownY_ = mouseDownArea_ = -1;
			mouseDownIndex_.reset();
			mouseDownTopIndex_.reset();
			return;
		}
		Document::ListType type;
		const std::optional<size_t> cursor = lineToIndex(y / cyItem_, type);
		if (
			!cursor.has_value() ||
			separatorClick(vkey, cursor.value(), x, type) ||
			(type == Document::ListType::FILE && doc_.GetFiles()[0]->IsEmpty()) ||
			(!listCursorIndex_ || !mouseDownIndex_) ||
			type != mouseDownListType_ ||
			getItemArea(x) != 2
		) {
			mouseDownY_ = mouseDownArea_ = -1;
			mouseDownIndex_.reset();
			mouseDownTopIndex_.reset();
			return;
		}

		const bool lbtn = (mkey & MK_LBUTTON) != 0;
		const bool rbtn = (mkey & MK_RBUTTON) != 0;

		if ((vkey == VK_RBUTTON && lbtn) || (vkey == VK_LBUTTON && rbtn)) {
			if (type == Document::ListType::FILE && listCursorIndex_ != mouseDownIndex_) selectFile(mouseDownIndex_, listCursorIndex_);
			action(COM_SHELL_MENU, type, listCursorIndex_);
		} else if (vkey == VK_LBUTTON) {
			if (type == Document::ListType::FILE && (listCursorIndex_ != mouseDownIndex_ || WindowUtils::CtrlPressed())) {
				selectFile(mouseDownIndex_, listCursorIndex_);
			} else {
				action(COM_OPEN, type, listCursorIndex_);
			}
		} else if (vkey == VK_RBUTTON) {
			if (type == Document::ListType::FILE && listCursorIndex_ != mouseDownIndex_) selectFile(mouseDownIndex_, listCursorIndex_);
			if (WindowUtils::CtrlPressed()) action(COM_SHELL_MENU, type, listCursorIndex_);
			else popupMenu(type, listCursorIndex_);
		} else if (vkey == VK_MBUTTON) {
			if (type == Document::ListType::FILE && listCursorIndex_ != mouseDownIndex_ && doc_.ArrangeFavorites(mouseDownIndex_, listCursorIndex_)) {
				doc_.Update();
			} else {
				action(COM_FAVORITE, type, listCursorIndex_);
			}
		}
		mouseDownY_ = mouseDownArea_ = -1;
		mouseDownIndex_.reset();
		mouseDownTopIndex_.reset();
	}

	// Check the position of the pointer
	int getItemArea(int x) const noexcept {
		if (x < cxSide_) return 0;
		if (x >= listRect_.right - cxSide_) return 1;
		return 2;
	}

	// Click on the separator
	bool separatorClick(int vkey, size_t index, int x, Document::ListType type) {
		if ((doc_.GetItem(type, index)->data() & SEPA) == 0) return false;

		if (doc_.InHistory()) {  // Click history separator
			if (vkey == VK_LBUTTON) action(COM_CLEAR_HISTORY, type, index);
			return true;
		}
		if (doc_.GetItem(type, index)->data() == (SEPA | HIER)) {  // Click the hierarchy separator
			if (listRect_.right * 2 / 3 < x) {  // If it is more than two thirds
				selectFile(0, doc_.GetFiles().Count() - 1);
			} else {
				int sortBy = doc_.GetOpt().GetSortType();
				switch (vkey) {
				case VK_LBUTTON:
					if (++sortBy > 3) sortBy = 0;
					doc_.GetOpt().SetSortType(sortBy);
					break;
				case VK_RBUTTON: doc_.GetOpt().SetSortOrder(!doc_.GetOpt().GetSortOrder()); break;
				case VK_MBUTTON: doc_.GetOpt().SetShowHidden(!doc_.GetOpt().IsShowHidden()); break;
				default: break;
				}
				ht_.setIndex(0U);
				doc_.Update();
			}
		}
		return true;
	}

	void wmKeyDown(WPARAM key) {
		const bool ctrl = WindowUtils::CtrlPressed();
		if (ctrl || key == VK_APPS || key == VK_DELETE || key == VK_RETURN) {
			if (!listCursorIndex_) return;
			if (ctrl) {
				if (key == VK_APPS) {
					action(COM_SHELL_MENU, listCursorSwitch_, listCursorIndex_);
				}  else if (_T('A') <= key && key <= _T('Z')) {
					accelerator(static_cast<TCHAR>(key), listCursorSwitch_, listCursorIndex_);
				}
			} else {
				switch (key) {
				case VK_APPS:   popupMenu(listCursorSwitch_, listCursorIndex_); break;
				case VK_DELETE: action(COM_DELETE, listCursorSwitch_, listCursorIndex_); break;
				case VK_RETURN: action(COM_OPEN, listCursorSwitch_, listCursorIndex_); break;
				default: break;
				}
			}
		} else {
			if (key == VK_F3) {
				setCursorIndex(search_.FindNext(listCursorIndex_, doc_.GetFiles()), Document::ListType::FILE);
				return;
			}
			keyCursor(key);  // Cursor movement by key operation
			if (_T('A') <= key && key <= _T('Z')) search_.KeySearch(key);  // Key input search
		}
	}

	// TODO Allow cursor movement to the navigation pane
	// Cursor key input
	void keyCursor(WPARAM key) {
		std::optional<size_t> index = listCursorIndex_;
		const ItemList& files = doc_.GetFiles();

		if (!index || listCursorSwitch_ == Document::ListType::HIER) {
			setCursorIndex(scrollListTopIndex_, Document::ListType::FILE);
			return;
		}
		size_t i = index.value();
		switch (key) {
		case VK_SPACE:  // It's not a cursor but it looks like it
			selectFile(index, index);
			[[fallthrough]];
		case VK_DOWN:
			i++;
			if (i >= files.Count()) i = 0;
			if ((doc_.GetItem(Document::ListType::FILE, i)->data() & SEPA) != 0) i++;
			setCursorIndex(i, Document::ListType::FILE);
			return;
		case VK_UP:
			if (i == 0) i = files.Count() - 1;
			else i--;
			if ((doc_.GetItem(Document::ListType::FILE, i)->data() & SEPA) != 0) {
				if (i == 0) i = files.Count() - 1;
				else i--;
			}
			setCursorIndex(i, Document::ListType::FILE);
			return;
		default: break;
		}
		if (key == VK_LEFT || key == VK_RIGHT) {
			if (key == VK_LEFT) {
				if (!ht_.canGoBack()) return;
				doc_.SetCurrentDirectory(ht_.goBack());
			} else {
				if (!doc_.MovableToLower(Document::ListType::FILE, i)) return;
				ht_.goForward(scrollListTopIndex_, doc_.CurrentPath());
				doc_.MoveToLower(Document::ListType::FILE, i);
			}
			setCursorIndex(scrollListTopIndex_, Document::ListType::FILE);
		}
	}

	// Specify cursor position
	void setCursorIndex(std::optional<size_t> index, Document::ListType type) {
		if (index.has_value() && type == Document::ListType::FILE) {
			if (index.value() < scrollListTopIndex_) {
				setScrollListTopIndex(index.value(), false);
			}
			else if (scrollListTopIndex_ + scrollListLineNum_ <= index.value()) {
				setScrollListTopIndex(index.value() - scrollListLineNum_ + 1, false);
			}
		}
		if (listCursorIndex_ && (listCursorSwitch_ != type || (!index.has_value() || listCursorIndex_ != index.value()))) {
			RECT r = listRect_;
			r.top    = static_cast<long>(indexToLine(listCursorIndex_.value(), listCursorSwitch_) * cyItem_);
			r.bottom = r.top + cyItem_;
			::InvalidateRect(hWnd_, &r, FALSE);
		}
		if (!index.has_value() || (doc_.GetItem(type, index.value())->data() & SEPA) != 0) {
			listCursorIndex_.reset();
			tt_.Inactivate();  // Hide tool tip
			::UpdateWindow(hWnd_);
			return;
		}
		if (listCursorSwitch_ != type || listCursorIndex_ != index) {
			listCursorIndex_ = index.value();
			listCursorSwitch_ = type;
			RECT r = listRect_;
			r.top    = static_cast<long>(indexToLine(index.value(), type) * cyItem_);
			r.bottom = r.top + cyItem_;
			::InvalidateRect(hWnd_, &r, FALSE);
		}
		if (listCursorSwitch_ != type) mouseDownIndex_.reset();
		::UpdateWindow(hWnd_);  // Update here as curSelIsLong_ is referred below
		tt_.Inactivate();  // Hide tool tip
		if (curSelIsLong_) {  // Show tool tip
			tt_.Activate(doc_.GetItem(type, index.value())->Name(), listRect_);
		} else if (doc_.InBookmark() || doc_.InHistory()) {
			tt_.Activate(doc_.GetItem(type, index.value())->Path(), listRect_);
		}
	}

	// Specify the start index of the scroll list
	void setScrollListTopIndex(size_t i, bool update = true) noexcept {
		const ItemList& files = doc_.GetFiles();

		const size_t old = scrollListTopIndex_;
		if (files.Count() <= scrollListLineNum_) scrollListTopIndex_ = 0;
		else if (files.Count() < scrollListLineNum_ + i) scrollListTopIndex_ = files.Count() - scrollListLineNum_;
		else scrollListTopIndex_ = i;

		RECT r;
		GetClientRect(hWnd_, &r);
		r.right -= cxScrollBar_;
		r.top    = static_cast<long>(cyItem_ * doc_.GetNavis().Count());
		::ScrollWindow(hWnd_, 0, static_cast<long>((old - scrollListTopIndex_) * cyItem_), &r, &r);
		GetClientRect(hWnd_, &r);
		r.left = r.right - cxScrollBar_;
		::InvalidateRect(hWnd_, &r, FALSE);
		if (update) ::UpdateWindow(hWnd_);
	}

	// Display pop-up menu and execute command
	void popupMenu(Document::ListType w, std::optional<size_t> index) {
		if (!index) return;

		doc_.SetOperator(index, w, ope_);
		if (ope_.Count() == 0 || ope_[0].empty()) return;  // Reject if objs is empty

		auto ext = FileSystem::is_directory(ope_[0]) ? PATH_EXT_DIR : Path::ext(ope_[0]);
		const int type = extensions_.get_id(ext) + 1;

		UINT f;
		const POINT pt = popupPt(w, index.value(), f);
		PopupMenu pm(hWnd_, &pref_);
		wstring cmd;
		std::vector<wstring> items;

		if (pm.popup(type, pt, f, cmd, items)) {
			action(ope_, cmd, w, index);
		}
	}

	// Execution of command by accelerator
	void accelerator(TCHAR accelerator, Document::ListType w, std::optional<size_t> index) {
		doc_.SetOperator(index, w, ope_);
		if (ope_.Count() == 0 || ope_[0].empty()) return;  // Reject if objs is empty
		auto ext = FileSystem::is_directory(ope_[0]) ? PATH_EXT_DIR : Path::ext(ope_[0]);
		const int type = extensions_.get_id(ext) + 1;
		wstring cmd;
		PopupMenu pm(hWnd_, &pref_);
		if (pm.getAccelCommand(type, accelerator, cmd)) action(ope_, cmd, w, index);
	}

	// Command execution
	void action(const wstring& cmd, Document::ListType w, std::optional<size_t> index) {
		doc_.SetOperator(index, w, ope_);
		action(ope_, cmd, w, index);
	}

	void action(const Selection &objs, const wstring& cmd, Document::ListType w, std::optional<size_t> index) {
		const bool hasObj = objs.Count() != 0 && !objs[0].empty();
		wstring oldCurrent;

		if (hasObj) {
			oldCurrent.assign(FileSystem::current_directory_path());
			::SetCurrentDirectory(Path::parent(objs[0]).c_str());
			doc_.SetHistory(objs[0]);
		}
		if (cmd.front() == _T('<')) {
			if (index) {
				systemCommand(cmd, objs, w, index.value());
			}
		} else {
			if (!hasObj) return;
			::ShowWindow(hWnd_, SW_HIDE);  // Hide in advance
			ope_.OpenBy(cmd);
		}
		if (hasObj) ::SetCurrentDirectory(oldCurrent.c_str());
	}

	// System command execution
	void systemCommand(const wstring& cmd, const Selection& objs, Document::ListType w, size_t index) {
		if (cmd == COM_SELECT_ALL)    { selectFile(0, doc_.GetFiles().Count() - 1, true); return; }
		if (cmd == COM_RENAME)        {
			re_.Open(objs[0], static_cast<long>(indexToLine(index, w) * cyItem_), listRect_.right, cyItem_);
			return;
		}
		if (cmd == COM_POPUP_INFO)    { popupInfo(objs, w, index); return; }
		if (cmd == COM_CLEAR_HISTORY) { doc_.ClearHistory(); return; }
		if (cmd == COM_FAVORITE)      { doc_.AddOrRemoveFavorite(objs[0], w, index); return; }  // Update here, so do nothing after return
		if (cmd == COM_START_DRAG)    { ::SetCursor(::LoadCursor(nullptr, IDC_NO)); ::ShowWindow(hWnd_, SW_HIDE); ope_.StartDrag(); return; }
		if (cmd == COM_SHELL_MENU)    {
			UINT f;
			const POINT pt = popupPt(w, index, f);
			ope_.PopupShellMenu(pt, f);
			return;
		}
		if (ope_.Command(cmd) == -1) ::ShowWindow(hWnd_, SW_HIDE);
	}

	// Select file by range specification
	void selectFile(std::optional<size_t> front_opt, std::optional<size_t> back_opt, bool all = false) {
		if (!front_opt || !back_opt || doc_.InDrives()) return;
		size_t front = front_opt.value();
		size_t back = back_opt.value();
		if (back < front) std::swap(front, back);
		doc_.SelectFile(front, back, Document::ListType::FILE, all);

		if (front < scrollListTopIndex_) front = scrollListTopIndex_;
		if (back >= scrollListTopIndex_ + scrollListLineNum_) back = scrollListTopIndex_ + scrollListLineNum_ - 1;

		RECT lr = listRect_;
		lr.top    = static_cast<long>(cyItem_ * (front - scrollListTopIndex_ + doc_.GetNavis().Count()));
		lr.bottom = static_cast<long>(cyItem_ * (back  - scrollListTopIndex_ + doc_.GetNavis().Count() + 1));
		::InvalidateRect(hWnd_, &lr, FALSE);

		// Update selected file count display
		lr.top    = static_cast<long>(cyItem_ * (doc_.GetNavis().Count() - 1));
		lr.bottom = lr.top + cyItem_;
		::InvalidateRect(hWnd_, &lr, FALSE);
		::UpdateWindow(hWnd_);
	}

	// Display file information
	void popupInfo(const Selection&, Document::ListType w, size_t index) {
		vector<wstring> items;
		ope_.InformationStrings(items);
		items.push_back(_T("...more"));
		UINT f;
		const POINT pt = popupPt(w, index, f);

		HMENU hMenu = ::CreatePopupMenu();
		if (hMenu == nullptr) return;

		for (size_t i = 0; i < items.size(); ++i) {
			::AppendMenu(hMenu, MF_STRING, i, items.at(i).c_str());
		}
		const int ret = ::TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_LEFTBUTTON | f, pt.x, pt.y, 0, hWnd_, nullptr);
		::DestroyMenu(hMenu);
		if (static_cast<size_t>(ret) == items.size() - 1) {
			ope_.PopupFileProperty();
		}
	}

	// Find the position of the popup
	POINT popupPt(Document::ListType w, size_t index, UINT &f) noexcept {
		f = TPM_RETURNCMD;
		RECT r;
		::GetWindowRect(hWnd_, &r);

		const size_t l = indexToLine(index, w);
		POINT pt = { 0, static_cast<long>(l * cyItem_) };
		::ClientToScreen(hWnd_, &pt);
		menuTop_() = pt.y;
		if (popupPos_ == 0 || popupPos_ == 3) {
			pt.x = r.right - 6 - ::GetSystemMetrics(SM_CXFIXEDFRAME);
		} else {
			f |= TPM_RIGHTALIGN;
			pt.x = r.left + ::GetSystemMetrics(SM_CXFIXEDFRAME);
		}
		return pt;
	}

	// Return line from index and type
	size_t indexToLine(size_t index, Document::ListType type) noexcept {
		return index + (doc_.GetNavis().Count() - scrollListTopIndex_) * static_cast<size_t>(type == Document::ListType::FILE);
	}

	// Return index and type from line
	std::optional<size_t> lineToIndex(size_t line, Document::ListType &type) noexcept {
		if (line < doc_.GetNavis().Count()) {
			type = Document::ListType::HIER;
			return line;
		}
		type = Document::ListType::FILE;
		const size_t i = line - doc_.GetNavis().Count() + scrollListTopIndex_;
		return (i < doc_.GetFiles().Count()) ? std::optional<size_t>{i} : std::nullopt;
	}

public:

	// Window size position adjustment
	void Updated() override {
		setScrollListTopIndex(ht_.index());
		setCursorIndex(std::nullopt, Document::ListType::FILE);
		scrollListLineNum_ = (listRect_.bottom - doc_.GetNavis().Count() * cyItem_) / cyItem_;

		tt_.Inactivate();
		::InvalidateRect(hWnd_, nullptr, TRUE);
		::UpdateWindow(hWnd_);
	}

};
