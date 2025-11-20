/**
 * View
 *
 * @author Takuto Yanagida
 * @version 2025-11-20
 */

#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include <optional>
#include <cmath>
#include <locale>

#include "gsl/gsl"
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

BOOL init_application(HINSTANCE hinst, const wchar_t* class_name) noexcept;
LRESULT CALLBACK wnd_proc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);

class View : public Observer {

	static constexpr auto SEPA  = 1;
	static constexpr auto DIR   = 2;
	static constexpr auto HIDE  = 4;
	static constexpr auto LINK  = 8;
	static constexpr auto HIER  = 16;
	static constexpr auto SEL   = 32;
	static constexpr auto EMPTY = 64;

	int mouseDownY_ = -1;
	int mouseDownArea_ = -1;
	std::optional<size_t> mouseDownIndex_;
	std::optional<size_t> mouseDownTopIndex_;
	Document::ListType mouseDownListType_ = Document::ListType::FILE;

	size_t scrollListTopIndex_ = 0;
	std::optional<size_t> listCursorIndex_ = 0;
	Document::ListType listCursorSwitch_ = Document::ListType::FILE;

	int cxSide_{};
	int cyItem_{};
	int cxScrollBar_{};
	HFONT hMarkFont_{};
	HFONT hItemFont_{};
	size_t scrollListLineNum_{};
	RECT listRect_{};

	Pref pref_;
	int popupPos_{};
	bool fullScreenCheck_{};

	static int& menuTop_() noexcept {
		static int menuTop_;
		return menuTop_;
	}
	bool curSelIsLong_{};
	bool suppressPopup_{};
	UINT cursorAlign_{};
	const HWND hWnd_{};
	double dpiFactX_{ 1.0 };
	double dpiFactY_{ 1.0 };

	HierTransition ht_;
	Document doc_;
	TypeTable extensions_;
	Selection ope_;
	RenameEdit re_;
	Search search_;
	ToolTip tt_;

public:

	static LRESULT CALLBACK menu_proc(HWND hMenu, UINT msg, WPARAM wp, LPARAM lp) noexcept {
		switch (msg) {
		case WM_WINDOWPOSCHANGING:
			{
				[[gsl::suppress(type.1)]]
				auto pos = reinterpret_cast<LPWINDOWPOS>(lp);
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
		[[gsl::suppress(type.1)]]
		auto proc = reinterpret_cast<WNDPROC>(::GetWindowLongPtr(hMenu, GWLP_USERDATA));
		return ::CallWindowProc(proc, hMenu, msg, wp, lp);
	}

	View(const HWND hWnd) : hWnd_(hWnd), doc_(extensions_, pref_, SEPA, (SEPA | HIER)), ope_(extensions_, pref_), re_(WM_RENAMEEDITCLOSED) {}

	View(const View&) = delete;
	virtual View& operator=(const View&) = delete;
	View(View&&) = delete;
	virtual View& operator=(View&&) = delete;
	virtual ~View() = default;

	void initialize() {
		doc_.set_observer(this);

		ope_.set_window_handle(hWnd_);
		re_.initialize(hWnd_);
		tt_.initialize(hWnd_);

		const int dpi = ::GetDpiForWindow(hWnd_);
		dpiFactX_ = dpi / 96.0;
		dpiFactY_ = dpi / 96.0;

		// Whether to make multi-user (call first)
		if (pref_.item_int(SECTION_WINDOW, KEY_MULTI_USER, VAL_MULTI_USER)) pref_.set_multi_user_mode();
		load_pref_data(true);  // Read and set from Ini file

		::SetTimer(hWnd_, 1, 300, nullptr);
	}

	// Read INI file
	void load_pref_data(const bool firstTime) {
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

		ope_.set_default_opener(defOpener);
		set_hot_key(hotKey, IDHK);

		::DeleteObject(hItemFont_);
		hItemFont_ = ::CreateFont(fontSize, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE, fontName.c_str());
		if (fontName.empty() || !hItemFont_) {
			hItemFont_ = window_utils::get_ui_message_font(hWnd_);
		}
		hMarkFont_ = ::CreateFont(std::lrint(14 * dpiFactX_), 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, SYMBOL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _T("Marlett"));
		re_.set_font(hItemFont_);

		search_.initialize(useMigemo);
		extensions_.restore(pref_);  // Load extension color

		doc_.initialize(firstTime);
	}

	void set_hot_key(const wstring& key, int id) const {
		UINT flag = 0;
		if (key.size() < 5) return;
		if (key.at(0) == _T('1')) flag |= MOD_ALT;
		if (key.at(1) == _T('1')) flag |= MOD_CONTROL;
		if (key.at(2) == _T('1')) flag |= MOD_SHIFT;
		if (key.at(3) == _T('1')) flag |= MOD_WIN;
		if (flag) RegisterHotKey(hWnd_, id, flag, key.at(4));
	}

	void finalize() {
		re_.finalize();

		RECT rw;
		GetWindowRect(hWnd_, &rw);
		pref_.set_current_section(SECTION_WINDOW);
		pref_.set_item_int(KEY_WIDTH, static_cast<int>((static_cast<__int64>(rw.right) - rw.left) / dpiFactX_));
		pref_.set_item_int(KEY_HEIGHT, static_cast<int>((static_cast<__int64>(rw.bottom) - rw.top) / dpiFactY_));

		doc_.finalize();
		::UnregisterHotKey(hWnd_, IDHK);  // Cancel hot key
		::DeleteObject(hMarkFont_);
		::DeleteObject(hItemFont_);

		::PostQuitMessage(0);
	}

	void wmClose() {
		if (window_utils::ctrl_pressed()) {
			::ShowWindow(hWnd_, SW_HIDE);
			load_pref_data(false);
			::ShowWindow(hWnd_, SW_SHOW);
		} else {
			doc_.finalize();
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
		load_pref_data(false);
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
		scrollListLineNum_ = (cheight - doc_.get_navi_count() * cyItem_) / cyItem_;

		const ItemList& files = doc_.get_files();

		if (scrollListLineNum_ <= files.size() && scrollListTopIndex_ > files.size() - scrollListLineNum_) {
			set_scroll_list_top_index(files.size() - scrollListLineNum_);
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

	void wmEndSession() {
		doc_.finalize();
	}

	void wmRequestUpdate() {
		ope_.done_request();
		doc_.Update();
	}

	void wmRenameEditClosed() {
		auto& renamedPath = re_.get_rename_path();
		auto& newFileName = re_.get_new_file_name();
		const auto ok = ope_.rename(renamedPath, newFileName);
		auto newPath = path::parent(renamedPath);
		newPath.append(_T("\\")).append(newFileName);
		if (ok && doc_.current_path() == renamedPath) doc_.set_current_directory(newPath);
		else doc_.Update();
	}

	// Event handler of WM_*MENULOOP
	void wmMenuLoop(bool enter) noexcept {
		auto hMenu = ::FindWindow(TEXT("#32768"), nullptr);

		if (!::IsWindow(hMenu)) return;
		if (enter) {
			::SetWindowLongPtr(hMenu, GWLP_USERDATA, ::GetWindowLongPtr(hMenu, GWLP_WNDPROC));
			[[gsl::suppress(type.1)]]
			::SetWindowLongPtr(hMenu, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(View::menu_proc));
		} else {
			::SetWindowLongPtr(hMenu, GWLP_WNDPROC, ::GetWindowLongPtr(hMenu, GWLP_USERDATA));
		}
	}

	void wmMouseWheel(int delta) noexcept {
		if (re_.is_active()) return;  // Rejected while renaming
		if (delta > 0) {
			set_scroll_list_top_index(3 <= scrollListTopIndex_ ? scrollListTopIndex_ - 3 : 0);
		}
		else {
			set_scroll_list_top_index(scrollListTopIndex_ + 3);
		}
	}

	enum class IconType { NONE = 0, SQUARE = 1, CIRCLE = 2, SCIRCLE = 3 };

	void wmPaint() {
		RECT rc{};
		PAINTSTRUCT ps{};

		::GetClientRect(hWnd_, &rc);
		auto dc = ::BeginPaint(hWnd_, &ps);

		if (ps.rcPaint.right > rc.right - cxScrollBar_) draw_scroll_bar(dc);
		if (ps.rcPaint.left < rc.right - cxScrollBar_) {
			const long begin = ps.rcPaint.top / cyItem_;
			const long end   = ps.rcPaint.bottom / cyItem_;
			RECT r{};
			r.top    = begin * cyItem_;
			r.bottom = r.top + cyItem_;
			r.left   = 0;
			r.right  = rc.right - cxScrollBar_;

			const ItemList& navis = doc_.get_navis();
			const ItemList& files = doc_.get_files();

			for (size_t i = begin; i <= gsl::narrow<size_t>(end); ++i) {
				if (i < navis.size()) {
					const auto it = navis.at(i);
					if (!it) continue;
					if ((it->data() & SEPA) != 0) {
						draw_separator(dc, r, (it->data() == (SEPA | HIER)));
					} else {
						draw_item(dc, r, it.get(), listCursorSwitch_ == Document::ListType::HIER && i == listCursorIndex_);
					}
				} else if (i - navis.size() + scrollListTopIndex_ < files.size()) {
					const size_t t = i - navis.size() + scrollListTopIndex_;
					draw_item(dc, r, files.at(t).get(), listCursorSwitch_ == Document::ListType::FILE && t == listCursorIndex_);
				} else {
					::FillRect(dc, &r, ::GetSysColorBrush(COLOR_MENU));
				}
				r.top += cyItem_, r.bottom += cyItem_;
			}
		}
		::EndPaint(hWnd_, &ps);
	}

	// Draw a scroll bar
	void draw_scroll_bar(HDC dc) noexcept {
		RECT rc{};
		const ItemList& files = doc_.get_files();

		::GetClientRect(hWnd_, &rc);
		rc.left = rc.right - cxScrollBar_;
		if (files.size() <= scrollListLineNum_) {
			::FillRect(dc, &rc, ::GetSysColorBrush(COLOR_MENU));
			return;
		}
		const double d = static_cast<double>(rc.bottom) / files.size();
		RECT t = rc;
		t.bottom = static_cast<LONG>(d * scrollListTopIndex_);
		::FillRect(dc, &t, ::GetSysColorBrush(COLOR_BTNSHADOW));
		if (scrollListLineNum_ + scrollListTopIndex_ < files.size()) {
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
	void draw_separator(HDC dc, RECT r, bool isHier) {
		SIZE font{};
		const ItemList& files = doc_.get_files();

		::FillRect(dc, &r, ::GetSysColorBrush(COLOR_MENU));
		::SelectObject(dc, hItemFont_);  // Font selection (do here because we measure the size below)
		if (isHier) {
			wstring num;
			if (doc_.selected_count()) {
				if (doc_.selected_count() == files.size()) {
					num.assign(_T("ALL / "));
				} else {
					num.assign(std::to_wstring(doc_.selected_count())).append(_T(" / "));
				}
			}
			num.append(std::to_wstring(files.size()));
			::GetTextExtentPoint32(dc, num.c_str(), gsl::narrow<int>(num.size()), &font);
			RECT nr = r;
			nr.left = nr.right - font.cx - 3;
			window_utils::draw_gray_text(dc, nr, num.c_str());
			r.right = nr.left - 1;

			TCHAR str[2]{};
			str[0] = doc_.get_option().get_sort_type_char();

			r.left += 3;
			window_utils::draw_gray_text(dc, r, &str[0]);
			::GetTextExtentPoint32(dc, &str[0], 1, &font);
			r.left = font.cx + 2;
		}
		if (doc_.in_history()) {
			r.left += 3;
			window_utils::draw_gray_text(dc, r, _T("x"));
			::GetTextExtentPoint32(dc, _T("x"), 1, &font);
			r.left = font.cx + 2;
		}
		::InflateRect(&r, -3, cyItem_ / -2);
		::DrawEdge(dc, &r, EDGE_ETCHED, BF_TOP);
	}

	// Draw an item
	void draw_item(HDC dc, RECT r, const Item* fd, bool cur) noexcept {
		::FillRect(dc, &r, ::GetSysColorBrush(cur ? COLOR_HIGHLIGHT : COLOR_MENU));  // Draw the background
		if (!fd) return;
		if (fd->is_empty()) {
			::SetTextColor(dc, GetSysColor(COLOR_GRAYTEXT));
			::SetBkMode(dc, TRANSPARENT);
			::SelectObject(dc, hItemFont_);
			r.left = cxSide_;
			curSelIsLong_ = false;
			::DrawText(dc, _T("empty"), -1, &r, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
			return;
		}
		IconType type = IconType::NONE;
		int color = fd->color();
		if (fd->is_link()) {  // Is shortcut?
			if (color == -1) color = ::GetSysColor(COLOR_GRAYTEXT), type = IconType::SCIRCLE;
			else type = IconType::CIRCLE;
		} else if (color != -1) {
			type = IconType::SQUARE;
		}
		draw_mark(dc, r, type, color, cur, fd->is_sel(), fd->is_dir());

		// Set text color and draw file name
		if (fd->is_hidden()) color = COLOR_GRAYTEXT;
		else color = (cur ? COLOR_HIGHLIGHTTEXT : COLOR_MENUTEXT);
		::SetTextColor(dc, GetSysColor(color));
		::SetBkMode(dc, TRANSPARENT);
		::SelectObject(dc, hItemFont_);
		r.left = cxSide_;
		if (fd->is_dir()) r.right -= cxSide_;
		SIZE font{};
		if (cur) {
			::GetTextExtentPoint32(dc, fd->name().c_str(), gsl::narrow<int>(fd->name().size()), &font);
			curSelIsLong_ = font.cx > r.right - r.left;  // File name at cursor position is out
		}
		::DrawText(dc, fd->name().c_str(), -1, &r, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | ((curSelIsLong_ && cur) ? cursorAlign_ : 0));
	}

	// Draw a mark
	void draw_mark(HDC dc, RECT r, IconType type, int color, bool cur, bool sel, bool dir) const noexcept {
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
				if (id != fid) window_utils::foreground_window(hWnd_);  // Bring the window to the front
			}
			// Search delay processing
			if (search_.is_reserved()) {
				set_cursor_index(search_.find_first(listCursorIndex_, doc_.get_files()), Document::ListType::FILE);
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
		if (fullScreenCheck_ && window_utils::is_foreground_window_full_screen()) return;
		::ShowWindow(hWnd_, SW_SHOW);
	}

	void wmShowWindow(bool show) {
		if (show) {
			doc_.Update();
			window_utils::move_window_to_corner(hWnd_, popupPos_);
			window_utils::foreground_window(hWnd_);  // Bring the window to the front
		} else {
			ht_.clear_indexes();
			re_.close();
		}
	}

	// Event Handler of WM_*BUTTONDOWN
	void wmButtonDown(int vkey, int x, int y) {
		if (re_.is_active()) return;  // Reject while renaming

		mouseDownY_        = y;
		mouseDownArea_     = get_item_area(x);
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
		if (vkey == VK_LBUTTON || vkey == VK_RBUTTON) action(CMD_START_DRAG, mouseDownListType_, mouseDownIndex_);
		mouseDownY_ = mouseDownArea_ = -1;
		mouseDownIndex_.reset();
		mouseDownTopIndex_.reset();
	}

	void wmMouseMove(WPARAM mkey, int x, int y) {
		if (re_.is_active()) return;  // Reject while renaming

		if (mouseDownY_ != -1 && mouseDownArea_ == 1) {  // Scroller
			const double t = mouseDownTopIndex_.value() - (static_cast<long long>(mouseDownY_) - y) * static_cast<double>(doc_.get_file_count()) / listRect_.bottom;
			set_scroll_list_top_index(std::lrint(max(0, t)));
			return;
		}
		Document::ListType type;
		const std::optional<size_t> cursor = lineToIndex(y / cyItem_, type);
		const int dir    = pointer_moving_dir(x, y);
		const int area   = get_item_area(x);
		static int lastArea = 2;
		static ULONGLONG lastTime;

		if (area == 2) {  // The mouse moved on the item
			if (cursor != listCursorIndex_) {
				set_cursor_index(cursor, type);
			} else {
				if (lastArea == 2) change_item_align(x, y);
				else if (::GetTickCount64() - lastTime < 400) {
					if (lastArea == 0 && dir == 1 && ht_.can_go_back()) {
						doc_.set_current_directory(ht_.go_back());
					} else if (lastArea == 1 && dir == 0 && doc_.is_movable_to_lower(type, listCursorIndex_.value())) {
						ht_.go_forward(scrollListTopIndex_, doc_.current_path());
						doc_.move_to_lower(type, listCursorIndex_.value());
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
				select_file(mouseDownIndex_, listCursorIndex_);
			}
			if (mkey & MK_LBUTTON) action(CMD_POPUP_INFO, type, listCursorIndex_);
			else if (window_utils::ctrl_pressed()) action(CMD_SHELL_MENU, type, listCursorIndex_);
			else popup_menu(type, listCursorIndex_);
			lastArea = -1;
		}
	}

	// Sense the direction the pointer is moving
	int pointer_moving_dir(int x, int y) noexcept {
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
	void change_item_align(int x, int y) noexcept {
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
		if (re_.is_active()) {
			re_.close();
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
			on_separator_click(vkey, cursor.value(), x, type) ||
			(type == Document::ListType::FILE && doc_.is_file_empty()) ||
			(!listCursorIndex_ || !mouseDownIndex_) ||
			type != mouseDownListType_ ||
			get_item_area(x) != 2
		) {
			mouseDownY_    = -1;
			mouseDownArea_ = -1;
			mouseDownIndex_.reset();
			mouseDownTopIndex_.reset();
			return;
		}

		const bool lbtn = (mkey & MK_LBUTTON) != 0;
		const bool rbtn = (mkey & MK_RBUTTON) != 0;

		if ((vkey == VK_RBUTTON && lbtn) || (vkey == VK_LBUTTON && rbtn)) {
			if (type == Document::ListType::FILE && listCursorIndex_ != mouseDownIndex_) {
				select_file(mouseDownIndex_, listCursorIndex_);
			}
			action(CMD_SHELL_MENU, type, listCursorIndex_);
		} else if (vkey == VK_LBUTTON) {
			if (type == Document::ListType::FILE && (listCursorIndex_ != mouseDownIndex_ || window_utils::ctrl_pressed())) {
				select_file(mouseDownIndex_, listCursorIndex_);
			} else {
				action(CMD_OPEN, type, listCursorIndex_);
			}
		} else if (vkey == VK_RBUTTON) {
			if (type == Document::ListType::FILE && listCursorIndex_ != mouseDownIndex_) {
				select_file(mouseDownIndex_, listCursorIndex_);
			}
			if (window_utils::ctrl_pressed()) {
				action(CMD_SHELL_MENU, type, listCursorIndex_);
			} else {
				popup_menu(type, listCursorIndex_);
			}
		} else if (vkey == VK_MBUTTON) {
			if (type == Document::ListType::FILE && listCursorIndex_ != mouseDownIndex_ && doc_.arrange_favorites(mouseDownIndex_, listCursorIndex_)) {
				doc_.Update();
			} else {
				action(CMD_FAVORITE, type, listCursorIndex_);
			}
		}
		mouseDownY_ = mouseDownArea_ = -1;
		mouseDownIndex_.reset();
		mouseDownTopIndex_.reset();
	}

	// Check the position of the pointer
	int get_item_area(int x) const noexcept {
		if (x < cxSide_) return 0;
		if (x >= listRect_.right - cxSide_) return 1;
		return 2;
	}

	// Click on the separator
	bool on_separator_click(int vkey, size_t index, int x, Document::ListType type) {
		if ((doc_.get_item(type, index)->data() & SEPA) == 0) return false;

		if (doc_.in_history()) {  // Click history separator
			if (vkey == VK_LBUTTON) action(CMD_CLEAR_HISTORY, type, index);
			return true;
		}
		if (doc_.get_item(type, index)->data() == (SEPA | HIER)) {  // Click the hierarchy separator
			if (listRect_.right * 2 / 3 < x) {  // If it is more than two thirds
				select_file(0, doc_.get_file_count() - 1);
			} else {
				int sortBy = doc_.get_option().get_sort_type();
				switch (vkey) {
				case VK_LBUTTON:
					if (++sortBy > 3) sortBy = 0;
					doc_.get_option().set_sort_type(sortBy);
					break;
				case VK_RBUTTON: doc_.get_option().set_sort_order(!doc_.get_option().get_sort_order()); break;
				case VK_MBUTTON: doc_.get_option().set_show_hidden(!doc_.get_option().is_show_hidden()); break;
				default: break;
				}
				ht_.set_index(0U);
				doc_.Update();
			}
		}
		return true;
	}

	void wmKeyDown(WPARAM key) {
		const bool ctrl = window_utils::ctrl_pressed();
		if (ctrl || key == VK_APPS || key == VK_DELETE || key == VK_RETURN) {
			if (!listCursorIndex_) return;
			if (ctrl) {
				if (key == VK_APPS) {
					action(CMD_SHELL_MENU, listCursorSwitch_, listCursorIndex_);
				}  else if (_T('A') <= key && key <= _T('Z')) {
					accelerator(gsl::narrow<TCHAR>(key), listCursorSwitch_, listCursorIndex_);
				}
			} else {
				switch (key) {
				case VK_APPS:   popup_menu(listCursorSwitch_, listCursorIndex_); break;
				case VK_DELETE: action(CMD_DELETE, listCursorSwitch_, listCursorIndex_); break;
				case VK_RETURN: action(CMD_OPEN, listCursorSwitch_, listCursorIndex_); break;
				default: break;
				}
			}
		} else {
			if (key == VK_F3) {
				set_cursor_index(search_.find_next(listCursorIndex_, doc_.get_files()), Document::ListType::FILE);
				return;
			}
			key_cursor(key);  // Cursor movement by key operation
			if (_T('A') <= key && key <= _T('Z')) search_.key_search(key);  // Key input search
		}
	}

	// TODO Allow cursor movement to the navigation pane
	// Cursor key input
	void key_cursor(WPARAM key) {
		std::optional<size_t> index = listCursorIndex_;
		const ItemList& files = doc_.get_files();

		if (!index || listCursorSwitch_ == Document::ListType::HIER) {
			set_cursor_index(scrollListTopIndex_, Document::ListType::FILE);
			return;
		}
		size_t i = index.value();
		switch (key) {
		case VK_SPACE:  // It's not a cursor but it looks like it
			select_file(index, index);
			[[fallthrough]];
		case VK_DOWN:
			i++;
			if (i >= files.size()) i = 0;
			if ((doc_.get_item(Document::ListType::FILE, i)->data() & SEPA) != 0) i++;
			set_cursor_index(i, Document::ListType::FILE);
			return;
		case VK_UP:
			if (i == 0) i = files.size() - 1;
			else i--;
			if ((doc_.get_item(Document::ListType::FILE, i)->data() & SEPA) != 0) {
				if (i == 0) i = files.size() - 1;
				else i--;
			}
			set_cursor_index(i, Document::ListType::FILE);
			return;
		default: break;
		}
		if (key == VK_LEFT || key == VK_RIGHT) {
			if (key == VK_LEFT) {
				if (!ht_.can_go_back()) return;
				doc_.set_current_directory(ht_.go_back());
			} else {
				if (!doc_.is_movable_to_lower(Document::ListType::FILE, i)) return;
				ht_.go_forward(scrollListTopIndex_, doc_.current_path());
				doc_.move_to_lower(Document::ListType::FILE, i);
			}
			set_cursor_index(scrollListTopIndex_, Document::ListType::FILE);
		}
	}

	// Specify cursor position
	void set_cursor_index(std::optional<size_t> index, Document::ListType type) {
		if (index.has_value() && type == Document::ListType::FILE) {
			if (index.value() < scrollListTopIndex_) {
				set_scroll_list_top_index(index.value(), false);
			}
			else if (scrollListTopIndex_ + scrollListLineNum_ <= index.value()) {
				set_scroll_list_top_index(index.value() - scrollListLineNum_ + 1, false);
			}
		}
		if (listCursorIndex_ && (listCursorSwitch_ != type || (!index.has_value() || listCursorIndex_ != index.value()))) {
			RECT r = listRect_;
			r.top    = gsl::narrow_cast<long>(index_to_line(listCursorIndex_.value(), listCursorSwitch_) * cyItem_);
			r.bottom = r.top + cyItem_;
			::InvalidateRect(hWnd_, &r, FALSE);
		}
		if (!index.has_value() || (doc_.get_item(type, index.value())->data() & SEPA) != 0) {
			listCursorIndex_.reset();
			tt_.inactivate();  // Hide tool tip
			::UpdateWindow(hWnd_);
			return;
		}
		if (listCursorSwitch_ != type || listCursorIndex_ != index) {
			listCursorIndex_ = index.value();
			listCursorSwitch_ = type;
			RECT r = listRect_;
			r.top    = gsl::narrow<long>(index_to_line(index.value(), type) * cyItem_);
			r.bottom = r.top + cyItem_;
			::InvalidateRect(hWnd_, &r, FALSE);
		}
		if (listCursorSwitch_ != type) mouseDownIndex_.reset();
		::UpdateWindow(hWnd_);  // Update here as curSelIsLong_ is referred below
		tt_.inactivate();  // Hide tool tip
		if (curSelIsLong_) {  // Show tool tip
			tt_.activate(doc_.get_item(type, index.value())->name(), listRect_);
		} else if (doc_.in_bookmark() || doc_.in_history()) {
			tt_.activate(doc_.get_item(type, index.value())->path(), listRect_);
		}
	}

	// Specify the start index of the scroll list
	void set_scroll_list_top_index(size_t i, bool update = true) noexcept {
		const ItemList& files = doc_.get_files();

		const size_t old = scrollListTopIndex_;
		if (files.size() <= scrollListLineNum_) {
			scrollListTopIndex_ = 0;
		} else if (files.size() < scrollListLineNum_ + i) {
			scrollListTopIndex_ = files.size() - scrollListLineNum_;
		} else {
			scrollListTopIndex_ = i;
		}
		RECT r;
		GetClientRect(hWnd_, &r);
		r.right -= cxScrollBar_;
		r.top    = gsl::narrow<long>(cyItem_ * doc_.get_navi_count());
		::ScrollWindow(hWnd_, 0, gsl::narrow_cast<long>((old - scrollListTopIndex_) * cyItem_), &r, &r);
		GetClientRect(hWnd_, &r);
		r.left = r.right - cxScrollBar_;
		::InvalidateRect(hWnd_, &r, FALSE);
		if (update) ::UpdateWindow(hWnd_);
	}

	// Display pop-up menu and execute command
	void popup_menu(Document::ListType w, std::optional<size_t> index) {
		if (!index) return;

		doc_.set_operator(index, w, ope_);
		if (ope_.size() == 0 || ope_[0].empty()) return;  // Reject if objs is empty

		auto ext = file_system::is_directory(ope_[0]) ? PATH_EXT_DIR : path::ext(ope_[0]);
		const int type = extensions_.get_id(ext) + 1;

		UINT f;
		const POINT pt = get_popup_pt(w, index.value(), f);
		PopupMenu pm(hWnd_, &pref_);
		wstring cmd;
		std::vector<wstring> items;

		if (pm.popup(type, pt, f, cmd, items)) {
			action(ope_, cmd, w, index);
		}
	}

	// Execution of command by accelerator
	void accelerator(TCHAR accelerator, Document::ListType w, std::optional<size_t> index) {
		doc_.set_operator(index, w, ope_);
		if (ope_.size() == 0 || ope_[0].empty()) return;  // Reject if objs is empty
		auto ext = file_system::is_directory(ope_[0]) ? PATH_EXT_DIR : path::ext(ope_[0]);
		const int type = extensions_.get_id(ext) + 1;
		wstring cmd;
		PopupMenu pm(hWnd_, &pref_);
		if (pm.get_accel_command(type, accelerator, cmd)) action(ope_, cmd, w, index);
	}

	// Command execution
	void action(const wstring& cmd, Document::ListType w, std::optional<size_t> index) {
		doc_.set_operator(index, w, ope_);
		action(ope_, cmd, w, index);
	}

	void action(const Selection &objs, const wstring& cmd, Document::ListType w, std::optional<size_t> index) {
		const bool hasObj = objs.size() != 0 && !objs[0].empty();
		wstring oldCurrent;

		if (hasObj) {
			oldCurrent.assign(file_system::current_directory_path());
			doc_.set_current_directory(path::parent(objs[0]).c_str());
			doc_.set_history(objs[0]);
		}
		if (cmd.front() == _T('<')) {
			if (index) {
				system_command(cmd, objs, w, index.value());
			}
		} else {
			if (!hasObj) return;
			::ShowWindow(hWnd_, SW_HIDE);  // Hide in advance
			ope_.open_by(cmd);
		}
		if (hasObj) doc_.set_current_directory(oldCurrent.c_str());
	}

	// System command execution
	void system_command(const wstring& cmd, const Selection& objs, Document::ListType w, size_t index) {
		if (cmd == CMD_SELECT_ALL)    { select_file(0, doc_.get_file_count() - 1, true); return; }
		if (cmd == CMD_RENAME)        {
			re_.open(objs[0], gsl::narrow<long>(index_to_line(index, w) * cyItem_), listRect_.right, cyItem_);
			return;
		}
		if (cmd == CMD_POPUP_INFO)    { popup_info(objs, w, index); return; }
		if (cmd == CMD_CLEAR_HISTORY) { doc_.clear_history(); return; }
		if (cmd == CMD_FAVORITE)      { doc_.add_or_remove_favorite(objs[0], w, index); return; }  // Update here, so do nothing after return
		if (cmd == CMD_START_DRAG)    { ::SetCursor(::LoadCursor(nullptr, IDC_NO)); ::ShowWindow(hWnd_, SW_HIDE); ope_.start_drag(); return; }
		if (cmd == CMD_SHELL_MENU)    {
			UINT f;
			const POINT pt = get_popup_pt(w, index, f);
			ope_.popup_shell_menu(pt, f);
			return;
		}
		if (ope_.command(cmd) == -1) ::ShowWindow(hWnd_, SW_HIDE);
	}

	// Select file by range specification
	void select_file(std::optional<size_t> front_opt, std::optional<size_t> back_opt, bool all = false) {
		if (!front_opt || !back_opt || doc_.in_drives()) return;
		size_t front = front_opt.value();
		size_t back  = back_opt.value();
		if (back < front) std::swap(front, back);
		doc_.select_file(front, back, Document::ListType::FILE, all);

		if (front < scrollListTopIndex_) front = scrollListTopIndex_;
		if (back >= scrollListTopIndex_ + scrollListLineNum_) back = scrollListTopIndex_ + scrollListLineNum_ - 1;

		RECT lr = listRect_;
		lr.top    = gsl::narrow<long>(cyItem_ * (front - scrollListTopIndex_ + doc_.get_navi_count()));
		lr.bottom = gsl::narrow<long>(cyItem_ * (back  - scrollListTopIndex_ + doc_.get_navi_count() + 1));
		::InvalidateRect(hWnd_, &lr, FALSE);

		// Update selected file count display
		lr.top    = gsl::narrow<long>(cyItem_ * (doc_.get_navi_count() - 1));
		lr.bottom = lr.top + cyItem_;
		::InvalidateRect(hWnd_, &lr, FALSE);
		::UpdateWindow(hWnd_);
	}

	// Display file information
	void popup_info(const Selection&, Document::ListType w, size_t index) {
		vector<wstring> items;
		ope_.create_information_strings(items);
		items.push_back(_T("...more"));
		UINT f{};

		HMENU hmenu = ::CreatePopupMenu();
		if (hmenu == nullptr) return;

		for (size_t i = 0; i < items.size(); ++i) {
			::AppendMenu(hmenu, MF_STRING, i, items.at(i).c_str());
		}
		const POINT pt = get_popup_pt(w, index, f);
		const int ret  = ::TrackPopupMenu(hmenu, TPM_RETURNCMD | TPM_LEFTBUTTON | f, pt.x, pt.y, 0, hWnd_, nullptr);
		::DestroyMenu(hmenu);
		if (gsl::narrow<size_t>(ret) == items.size() - 1) {
			ope_.popup_file_property();
		}
	}

	// Find the position of the popup
	POINT get_popup_pt(Document::ListType w, size_t index, UINT &f) noexcept {
		f = TPM_RETURNCMD;
		RECT r{};
		::GetWindowRect(hWnd_, &r);

		const size_t l = index_to_line(index, w);
		POINT pt = { 0, gsl::narrow<long>(l * cyItem_) };
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
	size_t index_to_line(size_t index, Document::ListType type) noexcept {
		return index + (doc_.get_navi_count() - scrollListTopIndex_) * static_cast<size_t>(type == Document::ListType::FILE);
	}

	// Return index and type from line
	std::optional<size_t> lineToIndex(size_t line, Document::ListType &type) noexcept {
		if (line < doc_.get_navi_count()) {
			type = Document::ListType::HIER;
			return line;
		}
		type = Document::ListType::FILE;
		const size_t i = line - doc_.get_navi_count() + scrollListTopIndex_;
		return (i < doc_.get_file_count()) ? std::optional<size_t>{i} : std::nullopt;
	}

public:

	// Window size position adjustment
	void updated() override {
		set_scroll_list_top_index(ht_.index());
		set_cursor_index(std::nullopt, Document::ListType::FILE);
		scrollListLineNum_ = (listRect_.bottom - doc_.get_navi_count() * cyItem_) / cyItem_;

		tt_.inactivate();
		::InvalidateRect(hWnd_, nullptr, TRUE);
		::UpdateWindow(hWnd_);
	}

};
