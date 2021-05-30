/**
 *
 * View
 *
 * @author Takuto Yanagida
 * @version 2021-05-30
 *
 */


#pragma once

#include <vector>
#include <string>
#include <optional>
#include <gsl/gsl_util>
#include <windows.h>

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

constexpr auto IDHK = 1;
extern const wchar_t WINDOW_NAME[];


BOOL InitApplication(HINSTANCE hInst, const wchar_t* className) noexcept;
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);


class View : public Observer {

	enum { SEPA = 1, DIR = 2, HIDE = 4, LINK = 8, HIER = 16, SEL = 32, EMPTY = 64 };

	int md_y_        = -1;
	int md_area_     = -1;
	std::optional<size_t> md_idx_{ std::nullopt };
	int md_top_idx_ = -1;
	Document::ListType md_list_type_ = Document::ListType::FILE;

	size_t scroll_list_top_idx_ = 0;
	std::optional<size_t> cur_list_Idx_{ std::nullopt };
	Document::ListType cur_list_type_ = Document::ListType::FILE;

	int side_w_ = 0, item_h_ = 0, sbar_w_ = 0;
	HFONT mark_font_ = nullptr, item_font_ = nullptr;
	size_t scroll_list_line_num_ = 0U;
	RECT list_rect_;

	Pref pref_{L"settings.ini"};
	Pref bookmark_data_{L"bookmark.dat"};
	Pref history_data_{L"history.dat"};
	int popup_pos_;
	bool full_screen_check_;

	static int& menu_top_() noexcept {
		static int top_ = 0;
		return top_;
	}
	bool cur_sel_is_long_ = false, suppress_popup_ = false;
	UINT cursor_align_ = 0;
	HWND hwnd_;
	double dpi_fact_x_, dpi_fact_y_;

	HierTransition ht_;
	Document doc_;
	TypeTable extentions_;
	Selection ope_;
	RenameEdit re_;
	Search search_;
	ToolTip tt_;

public:

	static LRESULT CALLBACK menu_proc(HWND hmenu, UINT msg, WPARAM wp, LPARAM lp) noexcept {
		if (msg == WM_WINDOWPOSCHANGING) {
			auto pos = (WINDOWPOS*)lp;
			if ((pos->flags & SWP_NOSIZE) && pos->y < menu_top_()) {
				RECT r;
				::GetWindowRect(hmenu, &r);
				const int h = ::GetSystemMetrics(SM_CYSCREEN);
				pos->y = h - (r.bottom - r.top);
			}
		}
		return ::CallWindowProc((WNDPROC)::GetWindowLong(hmenu, GWL_USERDATA), hmenu, msg, wp, lp);
	}

	View(const HWND hwnd) noexcept : doc_(extentions_, pref_, bookmark_data_, history_data_, SEPA, (SEPA | HIER)), ope_(extentions_, pref_), re_(WM_RENAMEEDITCLOSED) {
		doc_.set_view(this);

		hwnd_ = hwnd;
		::SetWindowLong(hwnd, GWL_USERDATA, (LONG)this);  // Set the window a property
		ope_.set_window_handle(hwnd);
		re_.initialize(hwnd);
		tt_.initialize(hwnd);

		const auto dpi = WindowUtils::get_dpi(hwnd_);
		dpi_fact_x_ = dpi.x / 96.0;
		dpi_fact_y_ = dpi.y / 96.0;

		// Whether to make multiple user (call first)
		if (pref_.get(SECTION_WINDOW, KEY_MULTIPLE_USER, VAL_MULTIPLE_USER)) {
			pref_.enable_multiple_user_mode();
			bookmark_data_.enable_multiple_user_mode();
			history_data_.enable_multiple_user_mode();
		}
		load_prop_data(true);  // Read and set from Ini file

		::SetTimer(hwnd_, 1, 300, nullptr);
	}

	// Read INI file
	void load_prop_data(const bool is_first) noexcept {
		side_w_ = static_cast<int>(pref_.get(SECTION_WINDOW, KEY_SIDE_AREA_WIDTH, VAL_SIDE_AREA_WIDTH) * dpi_fact_x_);
		item_h_ = static_cast<int>(pref_.get(SECTION_WINDOW, KEY_LINE_HEIGHT,     VAL_LINE_HEIGHT)     * dpi_fact_y_);
		sbar_w_ = static_cast<int>(6 * dpi_fact_x_);

		popup_pos_         = pref_.get(SECTION_WINDOW, KEY_POPUP_POSITION, VAL_POPUP_POSITION);
		full_screen_check_ = pref_.get(SECTION_WINDOW, KEY_FULL_SCREEN_CHECK, VAL_FULL_SCREEN_CHECK) != 0;

		std::wstring opener = pref_.get(SECTION_WINDOW, KEY_NO_LINKED, VAL_NO_LINKED);
		ope_.set_default_opener(opener);

		std::wstring hotkey = pref_.get(SECTION_WINDOW, KEY_POPUP_HOT_KEY, VAL_POPUP_HOT_KEY);
		set_hotkey(hotkey, IDHK);

		const int width  = static_cast<int>(pref_.get(SECTION_WINDOW,  KEY_WIDTH,  VAL_WIDTH) * dpi_fact_x_);
		const int height = static_cast<int>(pref_.get(SECTION_WINDOW, KEY_HEIGHT, VAL_HEIGHT) * dpi_fact_y_);

		::MoveWindow(hwnd_, 0, 0, width, height, FALSE);
		::ShowWindow(hwnd_, SW_SHOW);  // Once display, and calculate the size etc.
		::ShowWindow(hwnd_, SW_HIDE);  // Hide immediately

		std::wstring font_name = pref_.get(SECTION_WINDOW, KEY_FONT_NAME, VAL_FONT_NAME);
		const int font_size = static_cast<int>(pref_.get(SECTION_WINDOW, KEY_FONT_SIZE, VAL_FONT_SIZE) * dpi_fact_x_);

		::DeleteObject(item_font_);
		item_font_ = ::CreateFont(font_size, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE, font_name.c_str());
		if (font_name.empty() || !item_font_) {
			item_font_ = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
		}
		mark_font_ = ::CreateFont(static_cast<int>(14 * dpi_fact_x_), 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, SYMBOL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _T("Marlett"));
		re_.set_font(item_font_);

		const bool use_migemo = pref_.get(SECTION_WINDOW, KEY_USE_MIGEMO, VAL_USE_MIGEMO) != 0;
		search_.initialize(use_migemo);

		extentions_.restore(pref_);  // Load extension color
		doc_.initialize(is_first);
	}

	void set_hotkey(const std::wstring& key, int id) noexcept {
		UINT flag = 0;
		if (key.size() < 5) return;
		if (key.at(0) == _T('1')) flag |= MOD_ALT;
		if (key.at(1) == _T('1')) flag |= MOD_CONTROL;
		if (key.at(2) == _T('1')) flag |= MOD_SHIFT;
		if (key.at(3) == _T('1')) flag |= MOD_WIN;
		if (flag) ::RegisterHotKey(hwnd_, id, flag, key.at(4));
	}

	View(const View& inst)            = delete;
	View(View&& inst)                 = delete;
	View& operator=(const View& inst) = delete;
	View& operator=(View&& inst)      = delete;

	virtual ~View() noexcept {
		re_.finalize();

		RECT rw;
		GetWindowRect(hwnd_, &rw);
		pref_.set(SECTION_WINDOW, KEY_WIDTH, static_cast<int>(((__int64)rw.right  - rw.left) / dpi_fact_x_));
		pref_.set(SECTION_WINDOW, KEY_HEIGHT, static_cast<int>(((__int64)rw.bottom - rw.top)  / dpi_fact_y_));
		pref_.store();

		doc_.finalize();
		::UnregisterHotKey(hwnd_, IDHK);  // Cancel hot key
		::DeleteObject(mark_font_);
		::DeleteObject(item_font_);

		::PostQuitMessage(0);
	}

	void wm_close() noexcept {
		if (WindowUtils::is_ctrl_pressed()) {
			::ShowWindow(hwnd_, SW_HIDE);
			load_prop_data(false);
			::ShowWindow(hwnd_, SW_SHOW);
		} else {
			::DestroyWindow(hwnd_);
		}
	}

	void wm_dpi_changed(int dpiX, int dpiY) noexcept {
		dpi_fact_x_ = dpiX / 96.0;
		dpi_fact_y_ = dpiY / 96.0;

		const auto visible = ::IsWindowVisible(hwnd_);
		if (visible) {
			::ShowWindow(hwnd_, SW_HIDE);
		}
		load_prop_data(false);
		if (visible) {
			::ShowWindow(hwnd_, SW_SHOW);
		}
	}

	void wm_window_pos_changing(WINDOWPOS *wpos) noexcept {
		const int edge = ::GetSystemMetrics(SM_CYSIZEFRAME);
		const int upNC = ::GetSystemMetrics(SM_CYSMCAPTION) + edge;

		wpos->cy = (wpos->cy + item_h_ / 2 - edge - upNC) / item_h_ * item_h_ + edge + upNC;
		if (wpos->cy - edge - upNC < item_h_ * 8) wpos->cy = item_h_ * 8 + edge + upNC;
		if (wpos->cx < 96) wpos->cx = 96;
	}

	void wm_size(int cwidth, int cheight) noexcept {
		::SetRect(&list_rect_, 0, 0, cwidth - sbar_w_, cheight);
		scroll_list_line_num_ = (cheight - doc_.get_navi_list().size() * item_h_) / item_h_;

		const ItemList& files = doc_.get_file_list();

		if (scroll_list_line_num_ <= files.size() && scroll_list_top_idx_ > files.size() - scroll_list_line_num_) {
			set_scroll_list_top_index(files.size() - scroll_list_line_num_);
		}
	}

	void wm_hot_key(int id) noexcept {
		if (id == IDHK) {
			if (::IsWindowVisible(hwnd_)) suppress_popup_ = true;
			::ShowWindow(hwnd_, ::IsWindowVisible(hwnd_) ? SW_HIDE : SW_SHOW);
		}
	}

	void wm_end_session() noexcept {
		doc_.finalize();
	}

	void wm_request_update() noexcept {
		ope_.done_request();
		doc_.update();
	}

	void wm_rename_edit_closed() noexcept {
		auto &renamedPath = re_.get_renamed_path();
		auto &newFileName = re_.get_new_file_name();
		const auto ok = ope_.rename(renamedPath, newFileName);
		auto newPath = Path::parent(renamedPath);
		newPath.append(_T("\\")).append(newFileName);
		if (ok && doc_.current_path() == renamedPath) doc_.set_current_directory(newPath);
		else doc_.update();
	}

	// Event handler of WM_*MENULOOP
	void wm_menu_loop(bool enter) noexcept {
		auto hMenu = ::FindWindow(TEXT("#32768"), nullptr);

		if (!::IsWindow(hMenu)) return;
		if (enter) {
			::SetWindowLong(hMenu, GWL_USERDATA, (LONG)::GetWindowLong(hMenu, GWL_WNDPROC));
			::SetWindowLong(hMenu, GWL_WNDPROC, (LONG)View::menu_proc);
		} else {
			::SetWindowLong(hMenu, GWL_WNDPROC, (LONG)::GetWindowLong(hMenu, GWL_USERDATA));
		}
	}

	void wm_mouse_wheel(int delta) noexcept {
		if (re_.is_active()) return;  // Rejected while renaming
		set_scroll_list_top_index(scroll_list_top_idx_ - ((delta > 0) ? 3 : -3));
	}

	enum class IconType { NONE = 0, SQUARE = 1, CIRCLE = 2, SCIRCLE = 3 };

	void wm_paint() noexcept {
		RECT rc;
		PAINTSTRUCT ps;

		::GetClientRect(hwnd_, &rc);
		auto dc = ::BeginPaint(hwnd_, &ps);

		if (ps.rcPaint.right > rc.right - sbar_w_) draw_scroll_bar(dc);
		if (ps.rcPaint.left < rc.right - sbar_w_) {
			const size_t begin = ps.rcPaint.top    / item_h_;
			const size_t end   = ps.rcPaint.bottom / item_h_;
			RECT r{};
			r.top = begin * item_h_, r.bottom = r.top + item_h_, r.left = 0, r.right = rc.right - sbar_w_;

			const ItemList &navis = doc_.get_navi_list(), &files = doc_.get_file_list();

			for (size_t i = begin; i <= end; ++i) {
				if (i < navis.size()) {
					const Item *fd = navis.at(i);
					if (fd) {
						if ((fd->data() & SEPA) != 0) {
							draw_separator(dc, r, (fd->data() == (SEPA | HIER)));
						}
						else {
							draw_item(dc, r, fd, cur_list_type_ == Document::ListType::NAVI && cur_list_Idx_ && i == cur_list_Idx_.value());
						}
					}
				} else if (i - navis.size() + scroll_list_top_idx_ < files.size()) {
					const size_t t = i - navis.size() + scroll_list_top_idx_;
					draw_item(dc, r, files.at(t), cur_list_type_ == Document::ListType::FILE && cur_list_Idx_ && t == cur_list_Idx_.value());
				} else {
					::FillRect(dc, &r, (HBRUSH)(COLOR_MENU + 1));
				}
				r.top += item_h_, r.bottom += item_h_;
			}
		}
		::EndPaint(hwnd_, &ps);
	}

	// Draw a scroll bar
	void draw_scroll_bar(HDC dc) noexcept {
		RECT rc;
		const ItemList& files = doc_.get_file_list();

		::GetClientRect(hwnd_, &rc);
		rc.left = rc.right - sbar_w_;
		if (files.size() <= scroll_list_line_num_) {
			::FillRect(dc, &rc, (HBRUSH)(COLOR_MENU + 1));
			return;
		}
		const double d = 1.0 * rc.bottom / files.size();
		RECT t = rc;
		t.bottom = (LONG)(d * scroll_list_top_idx_);
		::FillRect(dc, &t, (HBRUSH)(COLOR_BTNSHADOW + 1));
		if (scroll_list_line_num_ + scroll_list_top_idx_ < files.size()) {
			t.top = t.bottom;
			t.bottom += (LONG)(d * scroll_list_line_num_);
			::FillRect(dc, &t, (HBRUSH)(COLOR_MENU + 1));
			t.top = t.bottom;
			t.bottom = rc.bottom;
			::FillRect(dc, &t, (HBRUSH)(COLOR_BTNSHADOW + 1));
		} else {
			t.top = t.bottom;
			t.bottom = rc.bottom;
			::FillRect(dc, &t, (HBRUSH)(COLOR_MENU + 1));
		}
	}

	// Draw a separator
	void draw_separator(HDC dc, RECT r, bool isHier) noexcept {
		const std::wstring sortBy{ L"nedsNEDS" };
		const ItemList& files = doc_.get_file_list();
		SIZE font;

		::FillRect(dc, &r, (HBRUSH)(COLOR_MENU + 1));
		::SelectObject(dc, item_font_);  // Font selection (do here because we measure the size below)
		if (isHier) {
			std::wstring num;
			if (doc_.selected_size()) {
				if (doc_.selected_size() == files.size()) {
					num.assign(_T("ALL / "));
				} else {
					num.assign(std::to_wstring(doc_.selected_size())).append(_T(" / "));
				}
			}
			num.append(std::to_wstring(files.size()));
			::GetTextExtentPoint32(dc, num.c_str(), num.size(), &font);
			RECT nr{ r };
			nr.left = nr.right - font.cx - 3;
			WindowUtils::draw_gray_text(dc, nr, num.c_str());
			r.right = nr.left - 1;
			const size_t idx = doc_.get_option().get_sort_type() + doc_.get_option().get_sort_order() * 4;
			const std::wstring str{ sortBy.at(idx) };

			r.left += 3;
			WindowUtils::draw_gray_text(dc, r, str.c_str());
			::GetTextExtentPoint32(dc, str.c_str(), str.size(), &font);
			r.left = font.cx + 2;
		}
		if (doc_.in_history()) {
			r.left += 3;
			WindowUtils::draw_gray_text(dc, r, _T("x"));
			::GetTextExtentPoint32(dc, _T("x"), 1, &font);
			r.left = font.cx + 2;
		}
		::InflateRect(&r, -3, item_h_ / -2);
		::DrawEdge(dc, &r, EDGE_ETCHED, BF_TOP);
	}

	// Draw an item
	void draw_item(HDC dc, RECT r, const Item *fd, bool cur) noexcept {
		if (!fd) return;
		::FillRect(dc, &r, (HBRUSH)((cur ? COLOR_HIGHLIGHT : COLOR_MENU) + 1));  // Draw the background
		if (fd->is_empty()) {
			::SetTextColor(dc, GetSysColor(COLOR_GRAYTEXT));
			::SetBkMode(dc, TRANSPARENT);
			::SelectObject(dc, item_font_);
			r.left = side_w_;
			cur_sel_is_long_ = false;
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
		draw_mark(dc, r, type, color, cur, fd->is_selected(), fd->is_dir());

		// Set text color and draw file name
		if (fd->is_hidden()) color = COLOR_GRAYTEXT;
		else color = (cur ? COLOR_HIGHLIGHTTEXT : COLOR_MENUTEXT);
		::SetTextColor(dc, GetSysColor(color));
		::SetBkMode(dc, TRANSPARENT);
		::SelectObject(dc, item_font_);
		r.left = side_w_;
		if (fd->is_dir()) r.right -= side_w_;
		SIZE font;
		if (cur) {
			::GetTextExtentPoint32(dc, fd->name().c_str(), fd->name().size(), &font);
			cur_sel_is_long_ = font.cx > r.right - r.left;  // File name at cursor position is out
		}
		::DrawText(dc, fd->name().c_str(), -1, &r, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | (cursor_align_ * (cur_sel_is_long_ ? 1 : 0) * cur));
	}

	// Draw a mark
	void draw_mark(HDC dc, RECT r, IconType type, int color, bool cur, bool sel, bool dir) noexcept {
		const wchar_t *c = nullptr;
		RECT rl = r, rr = r;

		rl.right = side_w_, rr.left = r.right - side_w_;
		::SetBkMode(dc, TRANSPARENT);
		::SelectObject(dc, mark_font_);  // Select font for symbols
		if (type == IconType::SQUARE)       c = _T("g");
		else if (type == IconType::CIRCLE)  c = _T("n");
		else if (type == IconType::SCIRCLE) c = _T("i");
		if (c) {
			::SetTextColor(dc, color);
			::DrawText(dc, c, 1, &rl, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
		}
		// Draw selection marks and folder marks
		::SetTextColor(dc, ::GetSysColor(cur ? COLOR_HIGHLIGHTTEXT : COLOR_MENUTEXT));
		if (sel) ::DrawText(dc, _T("a"), 1, &rl, 0x0025);
		if (dir) ::DrawText(dc, _T("4"), 1, &rr, 0x0025);
	}

	void wm_activate_app(bool isActive) noexcept {
		if (!isActive && ::GetCapture() != hwnd_) {
			::ShowWindow(hwnd_, SW_HIDE);
			//pref_.store();
		}
	}

	void wm_timer() noexcept {
		if (::IsWindowVisible(hwnd_)) {
			if (hwnd_ != ::GetForegroundWindow()) {  // If the window is displayed but somehow it is not the front
				DWORD id, fid;
				::GetWindowThreadProcessId(hwnd_, &id);
				::GetWindowThreadProcessId(::GetForegroundWindow(), &fid);
				if (id != fid) WindowUtils::foreground_window(hwnd_);  // Bring the window to the front
			}
			// Search delay processing
			if (search_.is_reserved()) {
				auto from = cur_list_Idx_ ? (cur_list_Idx_.value() + 1) : 0U;
				auto idx = search_.find_first(from, doc_.get_file_list());
				if (idx) set_cursor_index(idx, Document::ListType::FILE);
			}
			return;
		}
		// Pop-up function with mouse_
		const int r = ::GetSystemMetrics(SM_CXSCREEN) - 1;
		const int b = ::GetSystemMetrics(SM_CYSCREEN) - 1;
		POINT pt;
		::GetCursorPos(&pt);
		if (!(
			(popup_pos_ == 0 && pt.x <= 1 && pt.y <= 1) || (popup_pos_ == 1 && pt.x == r && pt.y == 0) ||
			(popup_pos_ == 2 && pt.x == r && pt.y == b) || (popup_pos_ == 3 && pt.x == 0 && pt.y == b)
			)) {
			suppress_popup_ = false;
			return;
		}
		if (suppress_popup_) return;
		// Does not pop up while dragging
		if (::GetAsyncKeyState(VK_LBUTTON) < 0 || ::GetAsyncKeyState(VK_RBUTTON) < 0) return;
		// Does not pop up if the current frontmost window is full screen
		if (full_screen_check_ && WindowUtils::is_foreground_window_full_screen()) return;
		::ShowWindow(hwnd_, SW_SHOW);
	}

	void wm_show_window(BOOL show) noexcept {
		if (show) {
			doc_.update();
			WindowUtils::move_window_to_corner(hwnd_, popup_pos_);
			WindowUtils::foreground_window(hwnd_);  // Bring the window to the front
		} else {
			ht_.clear_indexes();
			re_.hide();
		}
		search_.clear_key();
		::SetWindowText(hwnd_, WINDOW_NAME);
	}

	// Event Handler of WM_*BUTTONDOWN
	void wm_button_down(int vkey, int x, int y) noexcept {
		if (re_.is_active()) return;  // Reject while renaming

		md_y_       = y;
		md_area_    = get_item_area(x);
		md_idx_     = line_to_index(y / item_h_, md_list_type_);
		md_top_idx_ = scroll_list_top_idx_;

		if (md_area_ == 1) {  // Scroller
			::SetCapture(hwnd_);
			return;
		}
		if (!md_idx_) return;

		// Check button long press
		POINT pt, gp;
		const auto timeOut = ::GetTickCount64() + 500;
		::GetCursorPos(&gp);
		while (timeOut > ::GetTickCount64()) {
			::GetCursorPos(&pt);
			if (abs(pt.x - gp.x) > 2 || abs(pt.y - gp.y) > 2) return;
			if (!(::GetAsyncKeyState(vkey) < 0)) return;
		}
		if (vkey == VK_LBUTTON || vkey == VK_RBUTTON) {
			action(COM_START_DRAG, md_list_type_, md_idx_.value());
		}
		md_y_ = md_area_ = md_top_idx_ = -1;
		md_idx_ = std::nullopt;
	}

	void wm_mouse_move(int mkey, int x, int y) noexcept {
		if (re_.is_active()) return;  // Reject while renaming

		if (md_y_ != -1 && md_area_ == 1) {  // Scroller
			const int t = (int)(md_top_idx_ - ((__int64)md_y_ - y) / ((double)list_rect_.bottom / doc_.get_file_list().size()));
			set_scroll_list_top_index(t);
			return;
		}
		Document::ListType lt;
		const auto cursor    = line_to_index(y / item_h_, lt);
		const int dir        = pointer_moving_dir(x, y);
		const int area       = get_item_area(x);
		static int last_area = 2;
		static ULONGLONG last_time{};

		if (area == 2) {  // The mouse moved on the item
			if (!cur_list_Idx_ || cursor != cur_list_Idx_) {
				set_cursor_index(cursor, lt);
			} else {
				if (last_area == 2) {
					change_item_align(x, y);
				}
				else if (::GetTickCount64() - last_time < 400) {
					const size_t index = cur_list_Idx_.value();
					if (last_area == 0 && dir == 1 && ht_.can_go_back()) {
						doc_.set_current_directory(ht_.go_back());
					}
					else if (last_area == 1 && dir == 0 && doc_.is_movable_to_lower(lt, index)) {
						ht_.go_forward(scroll_list_top_idx_, doc_.current_path());
						doc_.move_to_lower(lt, index);
					}
				}
			}
			last_area = 2;
			return;
		}
		// Sidebar processing
		if (mkey == 0) {  // The button is not pressed
			if (last_area == 2) {
				last_time = ::GetTickCount64();
				if (area == 0 && dir == 0) {
					last_area = 0;  // Can return even if the cursor is not active
				}
				else if (area == 1 && dir == 1 && cur_list_Idx_) {
					last_area = 1;
				}
			}
		} else if (!(mkey & MK_MBUTTON)) {  // L or R button
			if (dir == 2 || dir == 3 || !cur_list_Idx_) return;
			if (lt == Document::ListType::FILE && md_idx_ != cur_list_Idx_ && lt == md_list_type_) {
				select_file(md_idx_.value(), cur_list_Idx_.value());
			}
			if (mkey & MK_LBUTTON) {
				action(COM_POPUP_INFO, lt, cur_list_Idx_.value());
			}
			else if (WindowUtils::is_ctrl_pressed()) {
				action(COM_SHELL_MENU, lt, cur_list_Idx_.value());
			}
			else {
				popup_menu(lt, cur_list_Idx_.value());
			}
			last_area = -1;
		}
	}

	// Sense the direction the pointer is moving
	int pointer_moving_dir(int x, int y) noexcept {
		static int lastX = 0, lastY = 0;

		const int dx = x - lastX, dy = y - lastY;
		const int w = abs(dx), h = abs(dy);
		lastX = x, lastY = y;

		if (w > h) return (dx < 0) ? 0 : 1;
		if (w < h) return (dy < 0) ? 2 : 3;
		return -1;
	}

	// Right justify file name by cursor position
	void change_item_align(int x, int y) noexcept {
		if (!cur_sel_is_long_) return;
		RECT r = list_rect_;
		unsigned int align = 0;
		if (r.right / 4 * 3 < x) align = DT_RIGHT;
		if (align != cursor_align_) {  // Redraw
			cursor_align_ = align;
			r.top = y / item_h_ * item_h_;
			r.bottom = r.top + item_h_;
			::InvalidateRect(hwnd_, &r, FALSE);
			::UpdateWindow(hwnd_);
		}
	}

	// Event Handler of WM_*BUTTONUP
	void wm_button_up(int vkey, int x, int y, int mkey) noexcept {
		if (re_.is_active()) {
			re_.hide();
			return;  // Reject while renaming
		}
		if (md_area_ == 1) {  // Scroller
			ReleaseCapture();
			md_y_ = md_area_ = md_top_idx_ = -1;
			md_idx_ = std::nullopt;
			return;
		}
		Document::ListType lt;
		const std::optional<size_t> cursor = line_to_index(y / item_h_, lt);
		if (
			!cursor ||
			is_separator_clicked(vkey, cursor, x, lt) ||
			(lt == Document::ListType::FILE && doc_.get_file_list().at(0)->is_empty()) ||
			(!cur_list_Idx_ || !md_idx_) ||
			lt != md_list_type_ ||
			get_item_area(x) != 2
			) {
			md_y_ = md_area_ = md_top_idx_ = -1;
			md_idx_ = std::nullopt;
			return;
		}
		const bool lbtn = (mkey & MK_LBUTTON) != 0;
		const bool rbtn = (mkey & MK_RBUTTON) != 0;

		if ((vkey == VK_RBUTTON && lbtn) || (vkey == VK_LBUTTON && rbtn)) {
			if (lt == Document::ListType::FILE && cur_list_Idx_ != md_idx_) {
				select_file(md_idx_.value(), cur_list_Idx_.value());
			}
			action(COM_SHELL_MENU, lt, cur_list_Idx_.value());
		} else if (vkey == VK_LBUTTON) {
			if (lt == Document::ListType::FILE && (cur_list_Idx_ != md_idx_ || WindowUtils::is_ctrl_pressed())) {
				select_file(md_idx_.value(), cur_list_Idx_.value());
			} else {
				action(COM_OPEN, lt, cur_list_Idx_.value());
			}
		} else if (vkey == VK_RBUTTON) {
			if (lt == Document::ListType::FILE && cur_list_Idx_ != md_idx_) {
				select_file(md_idx_.value(), cur_list_Idx_.value());
			}
			if (WindowUtils::is_ctrl_pressed()) {
				action(COM_SHELL_MENU, lt, cur_list_Idx_.value());
			}
			else {
				popup_menu(lt, cur_list_Idx_.value());
			}
		} else if (vkey == VK_MBUTTON) {
			if (lt == Document::ListType::FILE && cur_list_Idx_ != md_idx_ && md_idx_ && cur_list_Idx_ && doc_.arrange_bookmarks(md_idx_.value(), cur_list_Idx_.value())) {
				doc_.update();
			} else {
				action(COM_FAVORITE, lt, cur_list_Idx_.value());
			}
		}
		md_y_ = md_area_ = md_top_idx_ = -1;
		md_idx_ = std::nullopt;
	}

	// Check the position of the pointer
	int get_item_area(int x) noexcept {
		if (x < side_w_) return 0;
		if (x >= list_rect_.right - side_w_) return 1;
		return 2;
	}

	// Click on the separator
	bool is_separator_clicked(int vkey, std::optional<size_t> optIdx, int x, Document::ListType lt) noexcept {
		if (!optIdx) return false;
		const size_t index = optIdx.value();
		const auto it = doc_.get_item(lt, index);
		if ((it->data() & SEPA) == 0) return false;

		if (doc_.in_history()) {  // Click history separator
			if (x < list_rect_.right * 1 / 3 && vkey == VK_LBUTTON) {
				action(COM_CLEAR_HISTORY, lt, index);
			}
			return true;
		}
		if (it->data() == (SEPA | HIER)) {  // Click the hierarchy separator
			if (x < list_rect_.right * 1 / 3) {
				int sortBy = doc_.get_option().get_sort_type();
				switch (vkey) {
				case VK_LBUTTON:
					if (++sortBy > 3) sortBy = 0;
					doc_.get_option().set_sort_type(sortBy);
					break;
				case VK_RBUTTON: doc_.get_option().set_sort_order(!doc_.get_option().get_sort_order()); break;
				case VK_MBUTTON: doc_.get_option().set_hidden_shown(!doc_.get_option().is_hidden_shown()); break;
				default:  // do nothing
					break;
				}
				ht_.set_index(0U);
				doc_.update();
			}
			else if (list_rect_.right * 2 / 3 < x && vkey == VK_LBUTTON) {  // If it is more than two thirds
				select_file(0U, doc_.get_file_list().size() - 1);
			}
		}
		return true;
	}

	void wm_key_down(int key) noexcept {
		const bool ctrl = WindowUtils::is_ctrl_pressed();
		if (ctrl) {
			if (!cur_list_Idx_) return;
			if (key == VK_APPS) {
				action(COM_SHELL_MENU, cur_list_type_, cur_list_Idx_.value());
			}
			else if (L'A' <= key && key <= L'Z') {
				accelerator(static_cast<wchar_t>(key), cur_list_type_, cur_list_Idx_.value());
			}
		}
		else if (key == VK_APPS || key == VK_DELETE || key == VK_RETURN) {
			if (!cur_list_Idx_) return;
			switch (key) {
			case VK_APPS:   popup_menu(cur_list_type_, cur_list_Idx_.value()); break;
			case VK_DELETE: action(COM_DELETE, cur_list_type_, cur_list_Idx_.value()); break;
			case VK_RETURN: action(COM_OPEN, cur_list_type_, cur_list_Idx_.value()); break;
			default:  // do nothing
				break;
			}
		} else {
			if (L'A' <= key && key <= L'Z') {
				auto& str = search_.add_key(gsl::narrow_cast<wchar_t>(key));  // Key input search
				::SetWindowText(hwnd_, str.c_str());
			}
			else if (key == VK_BACK) {
				auto& str = search_.remove_key();
				::SetWindowText(hwnd_, str.empty() ? WINDOW_NAME : str.c_str());
			}
			else if (key == VK_ESCAPE) {
				search_.clear_key();
				::SetWindowText(hwnd_, WINDOW_NAME);
			}
			else if (key == VK_F3) {
				auto from = cur_list_Idx_ ? (cur_list_Idx_.value() + 1) : 0U;
				set_cursor_index(search_.find_next(from, doc_.get_file_list()), Document::ListType::FILE);
			}
			else {
				key_cursor(key);  // Cursor movement by key operation
			}
		}
	}

	// Cursor key input
	void key_cursor(int key) noexcept {
		auto listType{ cur_list_type_ };
		auto otherListType{ cur_list_type_ == Document::ListType::FILE ? Document::ListType::NAVI : Document::ListType::FILE };
		const ItemList& list = cur_list_type_ == Document::ListType::FILE ? doc_.get_file_list() : doc_.get_navi_list();
		const ItemList& otherList = cur_list_type_ == Document::ListType::FILE ? doc_.get_navi_list() : doc_.get_file_list();

		if (!cur_list_Idx_) {
			set_cursor_index(scroll_list_top_idx_, Document::ListType::FILE);
			return;
		}
		size_t index{ cur_list_Idx_.value() };
		switch (key) {
		case VK_SPACE:
			if (listType == Document::ListType::FILE) {
				select_file(index, index);
			}
			[[fallthrough]];
		case VK_DOWN:
			index++;
			if (index >= list.size()) {
				listType = otherListType;
				index = 0;
			}
			if ((doc_.get_item(listType, index)->data() & SEPA) != 0) {
				index++;
				if (index >= list.size()) {
					listType = otherListType;
					index = 0;
				}
			}
			set_cursor_index(index, listType);
			return;
		case VK_UP:
			if (index == 0) {
				listType = otherListType;
				index = otherList.size() - 1;
			}
			else {
				index--;
			}
			if ((doc_.get_item(listType, index)->data() & SEPA) != 0) {
				if (index == 0) {
					listType = otherListType;
					index = otherList.size() - 1;
				}
				else {
					index--;
				}
			}
			set_cursor_index(index, listType);
			return;
		default:  // do nothing
			break;
		}
		if (key == VK_LEFT || key == VK_RIGHT) {
			if (key == VK_LEFT) {
				if (!ht_.can_go_back()) return;
				doc_.set_current_directory(ht_.go_back());
			} else {
				if (!doc_.is_movable_to_lower(listType, index)) return;
				ht_.go_forward(scroll_list_top_idx_, doc_.current_path());
				doc_.move_to_lower(listType, index);
			}
			set_cursor_index(scroll_list_top_idx_, Document::ListType::FILE);
		}
	}

	// Specify cursor position
	void set_cursor_index(std::optional<size_t> optIdx, Document::ListType lt) noexcept {
		const size_t index = optIdx.value_or(0U);
		if (optIdx && lt == Document::ListType::FILE) {
			if (index < scroll_list_top_idx_) set_scroll_list_top_index(index, false);
			else if (scroll_list_top_idx_ + scroll_list_line_num_ <= index) set_scroll_list_top_index(index - scroll_list_line_num_ + 1, false);
		}
		if (cur_list_Idx_ && (cur_list_type_ != lt || cur_list_Idx_.value() != index)) {
			RECT r = list_rect_;
			r.top = index_to_line(cur_list_Idx_.value(), cur_list_type_) * item_h_;
			r.bottom = r.top + item_h_;
			::InvalidateRect(hwnd_, &r, FALSE);
		}
		if (!optIdx || (doc_.get_item(lt, index)->data() & SEPA) != 0) {
			cur_list_Idx_ = std::nullopt;
			tt_.inactivate();  // Hide tool tip
			::UpdateWindow(hwnd_);
			return;
		}
		if (cur_list_type_ != lt || !cur_list_Idx_ || cur_list_Idx_.value() != index) {
			cur_list_Idx_ = index;
			cur_list_type_ = lt;
			RECT r = list_rect_;
			r.top = index_to_line(index, lt) * item_h_;
			r.bottom = r.top + item_h_;
			::InvalidateRect(hwnd_, &r, FALSE);
		}
		if (cur_list_type_ != lt) md_idx_ = std::nullopt;
		::UpdateWindow(hwnd_);  // Update here as curSelIsLong_ is referred below
		tt_.inactivate();  // Hide tool tip
		if (cur_sel_is_long_) {  // Show tool tip
			tt_.activate(doc_.get_item(lt, index)->name(), list_rect_);
		} else if (doc_.in_bookmark() || doc_.in_history()) {
			tt_.activate(doc_.get_item(lt, index)->path(), list_rect_);
		}
	}

	// Specify the start index of the scroll list
	void set_scroll_list_top_index(int i, bool update = true) noexcept {
		const ItemList& files = doc_.get_file_list();

		const int old = scroll_list_top_idx_;
		if (i < 0) scroll_list_top_idx_ = 0;
		else if (files.size() <= scroll_list_line_num_) scroll_list_top_idx_ = 0;
		else if (files.size() - i < scroll_list_line_num_) scroll_list_top_idx_ = files.size() - scroll_list_line_num_;
		else scroll_list_top_idx_ = i;

		RECT r;
		GetClientRect(hwnd_, &r);
		r.right -= sbar_w_;
		r.top = item_h_ * doc_.get_navi_list().size();
		::ScrollWindow(hwnd_, 0, (old - scroll_list_top_idx_) * item_h_, &r, &r);
		GetClientRect(hwnd_, &r);
		r.left = r.right - sbar_w_;
		::InvalidateRect(hwnd_, &r, FALSE);
		if (update) ::UpdateWindow(hwnd_);
	}


	// ---------------------------------------------------------------------------


	// Display pop-up menu and execute command
	void popup_menu(Document::ListType lt, const size_t index) noexcept {
		ope_.clear();
		doc_.set_operator(index, lt, ope_);
		if (ope_.size() == 0 || ope_[0].empty()) return;  // Reject if objs is empty

		auto &ext = FileSystem::is_directory(ope_[0]) ? PATH_EXT_DIR : Path::ext(ope_[0]);
		const int type = extentions_.get_id(ext) + 1;
		UINT f;
		const POINT pt = get_popup_position(lt, index, f);
		std::wstring cmd;
		PopupMenu pm(hwnd_, &pref_);

		std::vector<std::wstring> items;
		if (pm.popup(type, pt, f, cmd, items)) {
			execute(ope_, cmd, lt, index);
		}
	}

	// Execution of command by accelerator
	void accelerator(wchar_t accelerator, Document::ListType lt, const size_t index) noexcept {
		ope_.clear();
		doc_.set_operator(index, lt, ope_);
		if (ope_.size() == 0 || ope_[0].empty()) return;  // Reject if objs is empty

		auto &ext = FileSystem::is_directory(ope_[0]) ? PATH_EXT_DIR : Path::ext(ope_[0]);
		const int type = extentions_.get_id(ext) + 1;
		std::wstring cmd;
		PopupMenu pm(hwnd_, &pref_);
		if (pm.get_accel_command(type, accelerator, cmd)) {
			execute(ope_, cmd, lt, index);
		}
	}

	// Command execution
	void action(const std::wstring& cmd, Document::ListType lt, const size_t index) noexcept {
		ope_.clear();
		doc_.set_operator(index, lt, ope_);
		execute(ope_, cmd, lt, index);
	}


	// ---------------------------------------------------------------------------


	void execute(const Selection &objs, const std::wstring& cmd, Document::ListType lt, const size_t index) noexcept {
		if (objs.size() == 0 || objs[0].empty()) {
			if (cmd.at(0) == _T('<')) {
				do_system_command(cmd, objs, lt, index);
			}
		}
		else {
			std::wstring old_cd{ FileSystem::current_directory_path() };
			::SetCurrentDirectory(Path::parent(objs[0]).c_str());

			doc_.set_history(objs[0]);
			if (cmd.at(0) == _T('<')) {
				do_system_command(cmd, objs, lt, index);
			}
			else {
				::ShowWindow(hwnd_, SW_HIDE);  // Hide in advance
				ope_.open_with(cmd);
			}
			::SetCurrentDirectory(old_cd.c_str());
		}
	}

	// System command execution
	void do_system_command(const std::wstring& cmd, const Selection& objs, Document::ListType lt, const size_t index) noexcept {
		if (cmd == COM_SELECT_ALL)    { select_file(0U, doc_.get_file_list().size() - 1, true); return; }
		if (cmd == COM_RENAME)        {
			re_.show(objs[0], index_to_line(index, lt) * item_h_, list_rect_.right, item_h_);
			return;
		}
		if (cmd == COM_POPUP_INFO)    { popup_info(objs, lt, index); return; }
		if (cmd == COM_CLEAR_HISTORY) { doc_.clear_history(); return; }
		if (cmd == COM_FAVORITE)      { doc_.add_or_remove_bookmark(objs[0], lt, index); return; }  // Update here, so do nothing after return
		if (cmd == COM_START_DRAG)    {
			::SetCursor(::LoadCursor(nullptr, IDC_NO));
			::ShowWindow(hwnd_, SW_HIDE); ope_.start_drag();
			return;
		}
		if (cmd == COM_SHELL_MENU)    {
			UINT f;
			const POINT pt = get_popup_position(lt, index, f);
			ope_.popup_shell_menu(pt, f);
			return;
		}
		if (ope_.command(cmd) == -1) ::ShowWindow(hwnd_, SW_HIDE);
	}

	// Select file by range specification
	void select_file(size_t front, size_t back, bool all = false) noexcept {
		if (doc_.in_drives()) return;
		if (back < front) std::swap(front, back);
		doc_.select_file(front, back, Document::ListType::FILE, all);

		if (front < scroll_list_top_idx_) {
			front = scroll_list_top_idx_;
		}
		if (back >= scroll_list_top_idx_ + scroll_list_line_num_) {
			back = scroll_list_top_idx_ + scroll_list_line_num_ - 1;
		}
		RECT lr = list_rect_;
		lr.top    = (front - scroll_list_top_idx_ + doc_.get_navi_list().size())     * item_h_;
		lr.bottom = (back  - scroll_list_top_idx_ + doc_.get_navi_list().size() + 1) * item_h_;
		::InvalidateRect(hwnd_, &lr, FALSE);

		// Update selected file count display
		lr.top    = (doc_.get_navi_list().size() - 1) * item_h_;
		lr.bottom = lr.top + item_h_;
		::InvalidateRect(hwnd_, &lr, FALSE);
		::UpdateWindow(hwnd_);
	}

	// Display file information
	void popup_info(const Selection& objs, Document::ListType lt, const size_t index) noexcept {
		if (objs.size() == 0) {
			return;
		}
		std::vector<std::wstring> items;
		objs.make_info_strings(items);
		items.push_back(_T("...more"));
		UINT f;
		const POINT pt = get_popup_position(lt, index, f);

		HMENU hmenu = ::CreatePopupMenu();
		for (size_t i = 0U; i < items.size(); ++i) {
			::AppendMenu(hmenu, MF_STRING, i, items.at(i).c_str());
		}
		const int ret = ::TrackPopupMenu(hmenu, TPM_RETURNCMD | TPM_LEFTBUTTON | f, pt.x, pt.y, 0, hwnd_, nullptr);
		::DestroyMenu(hmenu);
		if (ret == (int)items.size() - 1) {
			objs.popup_file_property();
		}
	}


	// ---------------------------------------------------------------------------


	// Find the position of the popup
	POINT get_popup_position(Document::ListType lt, const size_t index, UINT &out_align) noexcept {
		out_align = TPM_RETURNCMD;
		RECT r{};
		::GetWindowRect(hwnd_, &r);

		const int l = index_to_line(index, lt);
		POINT pt = { 0, l * item_h_ };
		::ClientToScreen(hwnd_, &pt);
		menu_top_() = pt.y;
		if (popup_pos_ == 0 || popup_pos_ == 3) {
			pt.x = r.right - 6 - ::GetSystemMetrics(SM_CXFIXEDFRAME);
		} else {
			out_align |= TPM_RIGHTALIGN;
			pt.x = r.left + ::GetSystemMetrics(SM_CXFIXEDFRAME);
		}
		return pt;
	}

	// Return line from index and type
	int index_to_line(const size_t index, Document::ListType lt) noexcept {
		if (lt == Document::ListType::FILE) {
			const size_t navi_size = doc_.get_navi_list().size();
			return index + navi_size - scroll_list_top_idx_;
		}
		return index;
	}

	// Return index and type from line
	std::optional<size_t> line_to_index(size_t line, Document::ListType& out_lt) noexcept {
		const size_t navi_size = doc_.get_navi_list().size();
		if (line < navi_size) {
			out_lt = Document::ListType::NAVI;
			return line;
		}
		out_lt = Document::ListType::FILE;
		const size_t idx = line - navi_size + scroll_list_top_idx_;
		if (idx < doc_.get_file_list().size()) {
			return idx;
		}
		return std::nullopt;
	}

public:

	// Window size position adjustment
	void updated() noexcept override {
		search_.clear_key();
		::SetWindowText(hwnd_, WINDOW_NAME);

		set_scroll_list_top_index(ht_.index());
		set_cursor_index(std::nullopt, Document::ListType::FILE);
		scroll_list_line_num_ = (list_rect_.bottom - doc_.get_navi_list().size() * item_h_) / item_h_;

		tt_.inactivate();
		::InvalidateRect(hwnd_, nullptr, TRUE);
		::UpdateWindow(hwnd_);
	}

};
