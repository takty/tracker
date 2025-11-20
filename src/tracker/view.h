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

constexpr auto IDHK = 1;

BOOL init_application(HINSTANCE inst, const wchar_t* class_name) noexcept;
LRESULT CALLBACK wnd_proc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);

class View : public Observer {

	static constexpr auto SEPA  = 1;
	static constexpr auto DIR   = 2;
	static constexpr auto HIDE  = 4;
	static constexpr auto LINK  = 8;
	static constexpr auto HIER  = 16;
	static constexpr auto SEL   = 32;
	static constexpr auto EMPTY = 64;

	int mouse_down_y_ = -1;
	int mouse_down_area_ = -1;
	std::optional<size_t> mouse_down_idx_;
	std::optional<size_t> mouse_down_top_idx_;
	Document::ListType mouse_down_list_type_ = Document::ListType::FILE;

	size_t scroll_list_top_idx_ = 0;
	std::optional<size_t> list_cursor_idx_ = 0;
	Document::ListType list_cursor_switch_ = Document::ListType::FILE;

	int cx_side_{};
	int cy_item_{};
	int cx_scroll_bar_{};
	HFONT font_mark_{};
	HFONT font_item_{};
	size_t scroll_list_line_num_{};
	RECT list_rect_{};

	Pref pref_;
	int popup_pos_{};
	bool full_screen_check_{};

	static int& menu_top_() noexcept {
		static int menu_top_;
		return menu_top_;
	}
	bool is_cur_sel_long_{};
	bool suppress_popup_{};
	UINT cursor_align_{};
	const HWND wnd_{};
	double dpi_fact_x_{ 1.0 };
	double dpi_fact_y_{ 1.0 };

	HierTransition ht_;
	Document doc_;
	TypeTable extensions_;
	Selection ope_;
	RenameEdit re_;
	Search search_;
	ToolTip tt_;

public:

	static LRESULT CALLBACK menu_proc(HWND menu, UINT msg, WPARAM wp, LPARAM lp) noexcept {
		switch (msg) {
		case WM_WINDOWPOSCHANGING:
			{
				[[gsl::suppress(type.1)]]
				auto pos = reinterpret_cast<LPWINDOWPOS>(lp);
				if ((pos->flags & SWP_NOSIZE) && pos->y < menu_top_()) {
					RECT r;
					::GetWindowRect(menu, &r);
					const int h = ::GetSystemMetrics(SM_CYSCREEN);
					pos->y = h - (r.bottom - r.top);
				}
				[[fallthrough]];
			}
		default: break;
		}
		[[gsl::suppress(type.1)]]
		auto proc = reinterpret_cast<WNDPROC>(::GetWindowLongPtr(menu, GWLP_USERDATA));
		return ::CallWindowProc(proc, menu, msg, wp, lp);
	}

	View(const HWND wnd) : wnd_(wnd), doc_(extensions_, pref_, SEPA, (SEPA | HIER)), ope_(extensions_, pref_), re_(WM_RENAMEEDITCLOSED) {}
	View(const View&) = delete;
	virtual View& operator=(const View&) = delete;
	View(View&&) = delete;
	virtual View& operator=(View&&) = delete;
	virtual ~View() = default;

	void initialize() {
		doc_.set_observer(this);

		ope_.set_window_handle(wnd_);
		re_.initialize(wnd_);
		tt_.initialize(wnd_);

		const int dpi = ::GetDpiForWindow(wnd_);
		dpi_fact_x_ = dpi / 96.0;
		dpi_fact_y_ = dpi / 96.0;

		// Whether to make multi-user (call first)
		if (pref_.item_int(SECTION_WINDOW, KEY_MULTI_USER, VAL_MULTI_USER)) pref_.set_multi_user_mode();
		load_pref_data(true);  // Read and set from Ini file

		::SetTimer(wnd_, 1, 300, nullptr);
	}

	// Read INI file
	void load_pref_data(const bool is_first_time) {
		pref_.set_current_section(SECTION_WINDOW);

		cx_side_       = std::lrint(pref_.item_int(KEY_SIDE_AREA_WIDTH, VAL_SIDE_AREA_WIDTH) * dpi_fact_x_);
		cy_item_       = std::lrint(pref_.item_int(KEY_LINE_HEIGHT,     VAL_LINE_HEIGHT)     * dpi_fact_y_);
		cx_scroll_bar_ = std::lrint(6 * dpi_fact_x_);

		popup_pos_         = pref_.item_int(KEY_POPUP_POSITION, VAL_POPUP_POSITION);
		full_screen_check_ = pref_.item_int(KEY_FULL_SCREEN_CHECK, VAL_FULL_SCREEN_CHECK) != 0;

		const int width  = std::lrint(pref_.item_int(KEY_WIDTH,  VAL_WIDTH)  * dpi_fact_x_);
		const int height = std::lrint(pref_.item_int(KEY_HEIGHT, VAL_HEIGHT) * dpi_fact_y_);

		const std::wstring def_opener = pref_.item(KEY_NO_LINKED, VAL_NO_LINKED);
		const std::wstring hot_key    = pref_.item(KEY_POPUP_HOT_KEY, VAL_POPUP_HOT_KEY);

		const std::wstring font_name = pref_.item(KEY_FONT_NAME, VAL_FONT_NAME);
		const int          font_size = std::lrint(pref_.item_int(KEY_FONT_SIZE, VAL_FONT_SIZE) * dpi_fact_x_);

		const bool use_migemo = pref_.item_int(KEY_USE_MIGEMO, VAL_USE_MIGEMO) != 0;

		::MoveWindow(wnd_, 0, 0, width, height, FALSE);
		::ShowWindow(wnd_, SW_SHOW);  // Once display, and calculate the size etc.
		::ShowWindow(wnd_, SW_HIDE);  // Hide immediately

		ope_.set_default_opener(def_opener);
		set_hot_key(hot_key, IDHK);

		::DeleteObject(font_item_);
		font_item_ = ::CreateFont(font_size, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE, font_name.c_str());
		if (font_name.empty() || !font_item_) {
			font_item_ = window_utils::get_ui_message_font(wnd_);
		}
		font_mark_ = ::CreateFont(std::lrint(14 * dpi_fact_x_), 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, SYMBOL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _T("Marlett"));
		re_.set_font(font_item_);

		search_.initialize(use_migemo);
		extensions_.restore(pref_);  // Load extension color

		doc_.initialize(is_first_time);
	}

	void set_hot_key(const std::wstring& key, int id) const {
		UINT flag = 0;
		if (key.size() < 5) return;
		if (key.at(0) == _T('1')) flag |= MOD_ALT;
		if (key.at(1) == _T('1')) flag |= MOD_CONTROL;
		if (key.at(2) == _T('1')) flag |= MOD_SHIFT;
		if (key.at(3) == _T('1')) flag |= MOD_WIN;
		if (flag) RegisterHotKey(wnd_, id, flag, key.at(4));
	}

	void finalize() {
		re_.finalize();

		RECT rw{};
		::GetWindowRect(wnd_, &rw);
		pref_.set_current_section(SECTION_WINDOW);
		pref_.set_item_int(KEY_WIDTH,  static_cast<int>((static_cast<__int64>(rw.right)  - rw.left) / dpi_fact_x_));
		pref_.set_item_int(KEY_HEIGHT, static_cast<int>((static_cast<__int64>(rw.bottom) - rw.top)  / dpi_fact_y_));

		doc_.finalize();
		::UnregisterHotKey(wnd_, IDHK);  // Cancel hot key
		::DeleteObject(font_mark_);
		::DeleteObject(font_item_);

		::PostQuitMessage(0);
	}

	void wm_close() {
		if (window_utils::ctrl_pressed()) {
			::ShowWindow(wnd_, SW_HIDE);
			load_pref_data(false);
			::ShowWindow(wnd_, SW_SHOW);
		} else {
			doc_.finalize();
			::DestroyWindow(wnd_);
		}
	}

	void wm_dpi_changed(int dpi_x, int dpi_y) {
		dpi_fact_x_ = dpi_x / 96.0;
		dpi_fact_y_ = dpi_y / 96.0;

		const auto visible = ::IsWindowVisible(wnd_);
		if (visible) {
			::ShowWindow(wnd_, SW_HIDE);
		}
		load_pref_data(false);
		if (visible) {
			::ShowWindow(wnd_, SW_SHOW);
		}
	}

	void wm_window_pos_changing(WINDOWPOS *wpos) const noexcept {
		if (wpos == nullptr) {
			return;
		}
		const int edge  = ::GetSystemMetrics(SM_CYSIZEFRAME);
		const int up_nc = ::GetSystemMetrics(SM_CYSMCAPTION) + edge;

		wpos->cy = (wpos->cy + cy_item_ / 2 - edge - up_nc) / cy_item_ * cy_item_ + edge + up_nc;
		if (wpos->cy - edge - up_nc < cy_item_ * 8) wpos->cy = cy_item_ * 8 + edge + up_nc;
		if (wpos->cx < 96) wpos->cx = 96;
	}

	void wm_size(int cwidth, int cheight) noexcept {
		::SetRect(&list_rect_, 0, 0, cwidth - cx_scroll_bar_, cheight);
		scroll_list_line_num_ = (cheight - doc_.get_navi_count() * cy_item_) / cy_item_;

		const ItemList& files = doc_.get_files();

		if (scroll_list_line_num_ <= files.size() && scroll_list_top_idx_ > files.size() - scroll_list_line_num_) {
			set_scroll_list_top_index(files.size() - scroll_list_line_num_);
		}
	}

	void wm_hot_key(WPARAM id) noexcept {
		if (wnd_ == nullptr) {
			return;
		}
		if (id == IDHK) {
			if (::IsWindowVisible(wnd_)) {
				suppress_popup_ = true;
			}
			::ShowWindow(wnd_, ::IsWindowVisible(wnd_) ? SW_HIDE : SW_SHOW);
		}
	}

	void wm_end_session() {
		doc_.finalize();
	}

	void wm_request_update() {
		ope_.done_request();
		doc_.Update();
	}

	void wm_rename_edit_closed() {
		auto& renamedPath = re_.get_rename_path();
		auto& newFileName = re_.get_new_file_name();
		const auto ok     = ope_.rename(renamedPath, newFileName);
		auto newPath      = path::parent(renamedPath);
		newPath.append(_T("\\")).append(newFileName);

		if (ok && doc_.current_path() == renamedPath) {
			doc_.set_current_directory(newPath);
		} else {
			doc_.Update();
		}
	}

	// Event handler of WM_*MENULOOP
	void wm_menu_loop(bool enter) noexcept {
		auto menu = ::FindWindow(TEXT("#32768"), nullptr);

		if (!::IsWindow(menu)) return;
		if (enter) {
			::SetWindowLongPtr(menu, GWLP_USERDATA, ::GetWindowLongPtr(menu, GWLP_WNDPROC));
			[[gsl::suppress(type.1)]]
			::SetWindowLongPtr(menu, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(View::menu_proc));
		} else {
			::SetWindowLongPtr(menu, GWLP_WNDPROC, ::GetWindowLongPtr(menu, GWLP_USERDATA));
		}
	}

	void wm_mouse_wheel(int delta) noexcept {
		if (re_.is_active()) return;  // Rejected while renaming
		if (delta > 0) {
			set_scroll_list_top_index(3 <= scroll_list_top_idx_ ? scroll_list_top_idx_ - 3 : 0);
		}
		else {
			set_scroll_list_top_index(scroll_list_top_idx_ + 3);
		}
	}

	enum class IconType { NONE = 0, SQUARE = 1, CIRCLE = 2, SCIRCLE = 3 };

	void wm_paint() {
		RECT rc{};
		PAINTSTRUCT ps{};

		::GetClientRect(wnd_, &rc);
		auto dc = ::BeginPaint(wnd_, &ps);

		if (ps.rcPaint.right > rc.right - cx_scroll_bar_) draw_scroll_bar(dc);
		if (ps.rcPaint.left < rc.right - cx_scroll_bar_) {
			const long bgn = ps.rcPaint.top    / cy_item_;
			const long end = ps.rcPaint.bottom / cy_item_;
			RECT r{};
			r.top    = bgn * cy_item_;
			r.bottom = r.top + cy_item_;
			r.left   = 0;
			r.right  = rc.right - cx_scroll_bar_;

			const ItemList& navis = doc_.get_navis();
			const ItemList& files = doc_.get_files();

			for (size_t i = bgn; i <= gsl::narrow<size_t>(end); ++i) {
				if (i < navis.size()) {
					const auto it = navis.at(i);
					if (!it) continue;
					if ((it->data() & SEPA) != 0) {
						draw_separator(dc, r, (it->data() == (SEPA | HIER)));
					} else {
						draw_item(dc, r, it.get(), list_cursor_switch_ == Document::ListType::HIER && i == list_cursor_idx_);
					}
				} else if (i - navis.size() + scroll_list_top_idx_ < files.size()) {
					const size_t t = i - navis.size() + scroll_list_top_idx_;
					draw_item(dc, r, files.at(t).get(), list_cursor_switch_ == Document::ListType::FILE && t == list_cursor_idx_);
				} else {
					::FillRect(dc, &r, ::GetSysColorBrush(COLOR_MENU));
				}
				r.top += cy_item_, r.bottom += cy_item_;
			}
		}
		::EndPaint(wnd_, &ps);
	}

	// Draw a scroll bar
	void draw_scroll_bar(HDC dc) noexcept {
		RECT rc{};
		const ItemList& files = doc_.get_files();

		::GetClientRect(wnd_, &rc);
		rc.left = rc.right - cx_scroll_bar_;
		if (files.size() <= scroll_list_line_num_) {
			::FillRect(dc, &rc, ::GetSysColorBrush(COLOR_MENU));
			return;
		}
		const double d = static_cast<double>(rc.bottom) / files.size();
		RECT t = rc;
		t.bottom = static_cast<LONG>(d * scroll_list_top_idx_);
		::FillRect(dc, &t, ::GetSysColorBrush(COLOR_BTNSHADOW));
		if (scroll_list_line_num_ + scroll_list_top_idx_ < files.size()) {
			t.top     = t.bottom;
			t.bottom += static_cast<LONG>(d * scroll_list_line_num_);
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
		::SelectObject(dc, font_item_);  // Font selection (do here because we measure the size below)
		if (isHier) {
			std::wstring num;
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
		::InflateRect(&r, -3, cy_item_ / -2);
		::DrawEdge(dc, &r, EDGE_ETCHED, BF_TOP);
	}

	// Draw an item
	void draw_item(HDC dc, RECT r, const Item* fd, bool cur) noexcept {
		::FillRect(dc, &r, ::GetSysColorBrush(cur ? COLOR_HIGHLIGHT : COLOR_MENU));  // Draw the background
		if (!fd) return;
		if (fd->is_empty()) {
			::SetTextColor(dc, GetSysColor(COLOR_GRAYTEXT));
			::SetBkMode(dc, TRANSPARENT);
			::SelectObject(dc, font_item_);
			r.left = cx_side_;
			is_cur_sel_long_ = false;
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
		::SelectObject(dc, font_item_);
		r.left = cx_side_;
		if (fd->is_dir()) r.right -= cx_side_;
		SIZE font{};
		if (cur) {
			::GetTextExtentPoint32(dc, fd->name().c_str(), gsl::narrow<int>(fd->name().size()), &font);
			is_cur_sel_long_ = font.cx > r.right - r.left;  // File name at cursor position is out
		}
		::DrawText(dc, fd->name().c_str(), -1, &r, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | ((is_cur_sel_long_ && cur) ? cursor_align_ : 0));
	}

	// Draw a mark
	void draw_mark(HDC dc, RECT r, IconType type, int color, bool cur, bool sel, bool dir) const noexcept {
		TCHAR c[2]{};
		RECT rl = r, rr = r;

		rl.right = cx_side_;
		rr.left  = r.right - cx_side_;

		::SetBkMode(dc, TRANSPARENT);
		::SelectObject(dc, font_mark_);  // Select font for symbols

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

	void wm_timer() {
		if (::IsWindowVisible(wnd_)) {
			if (wnd_ != ::GetForegroundWindow()) {  // If the window is displayed but somehow it is not the front
				DWORD id, fid;
				::GetWindowThreadProcessId(wnd_, &id);
				::GetWindowThreadProcessId(::GetForegroundWindow(), &fid);
				if (id != fid) window_utils::foreground_window(wnd_);  // Bring the window to the front
			}
			// Search delay processing
			if (search_.is_reserved()) {
				set_cursor_index(search_.find_first(list_cursor_idx_, doc_.get_files()), Document::ListType::FILE);
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
		if (full_screen_check_ && window_utils::is_foreground_window_full_screen()) return;
		::ShowWindow(wnd_, SW_SHOW);
	}

	void wm_show_window(bool show) {
		if (show) {
			doc_.Update();
			window_utils::move_window_to_corner(wnd_, popup_pos_);
			window_utils::foreground_window(wnd_);  // Bring the window to the front
		} else {
			ht_.clear_indexes();
			re_.close();
		}
	}

	// Event Handler of WM_*BUTTONDOWN
	void wm_button_down(int vkey, int x, int y) {
		if (re_.is_active()) return;  // Reject while renaming

		mouse_down_y_       = y;
		mouse_down_area_    = get_item_area(x);
		mouse_down_idx_     = line_to_index(y / cy_item_, mouse_down_list_type_);
		mouse_down_top_idx_ = scroll_list_top_idx_;

		if (mouse_down_area_ == 1) {  // Scroller
			::SetCapture(wnd_);
			return;
		}
		if (!mouse_down_idx_) return;

		// Check button long press
		POINT pt{}, gp{};
		const auto time_out = ::GetTickCount64() + 500;
		::GetCursorPos(&gp);
		while (time_out > ::GetTickCount64()) {
			::GetCursorPos(&pt);
			if (abs(pt.x - gp.x) > 2 || abs(pt.y - gp.y) > 2) return;
			if (!(::GetAsyncKeyState(vkey) < 0)) return;
		}
		if (vkey == VK_LBUTTON || vkey == VK_RBUTTON) action(CMD_START_DRAG, mouse_down_list_type_, mouse_down_idx_);
		mouse_down_y_ = mouse_down_area_ = -1;
		mouse_down_idx_.reset();
		mouse_down_top_idx_.reset();
	}

	void wm_mouse_move(WPARAM mkey, int x, int y) {
		if (re_.is_active()) return;  // Reject while renaming

		if (mouse_down_y_ != -1 && mouse_down_area_ == 1) {  // Scroller
			const double t = mouse_down_top_idx_.value() - (static_cast<long long>(mouse_down_y_) - y) * static_cast<double>(doc_.get_file_count()) / list_rect_.bottom;
			set_scroll_list_top_index(std::lrint(max(0, t)));
			return;
		}
		Document::ListType type;
		const std::optional<size_t> cursor = line_to_index(y / cy_item_, type);
		const int dir    = pointer_moving_dir(x, y);
		const int area   = get_item_area(x);
		static int last_area = 2;
		static ULONGLONG last_time;

		if (area == 2) {  // The mouse moved on the item
			if (cursor != list_cursor_idx_) {
				set_cursor_index(cursor, type);
			} else {
				if (last_area == 2) change_item_align(x, y);
				else if (::GetTickCount64() - last_time < 400) {
					if (last_area == 0 && dir == 1 && ht_.can_go_back()) {
						doc_.set_current_directory(ht_.go_back());
					} else if (last_area == 1 && dir == 0 && doc_.is_movable_to_lower(type, list_cursor_idx_.value())) {
						ht_.go_forward(scroll_list_top_idx_, doc_.current_path());
						doc_.move_to_lower(type, list_cursor_idx_.value());
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
				if (area == 0 && dir == 0) last_area = 0;  // Can return even if the cursor is not active
				else if (area == 1 && dir == 1 && list_cursor_idx_) last_area = 1;
			}
		} else if (!(mkey & MK_MBUTTON)) {  // L or R button
			if (dir == 2 || dir == 3 || !list_cursor_idx_) return;
			if (type == Document::ListType::FILE && mouse_down_idx_ != list_cursor_idx_ && type == mouse_down_list_type_) {
				select_file(mouse_down_idx_, list_cursor_idx_);
			}
			if (mkey & MK_LBUTTON) action(CMD_POPUP_INFO, type, list_cursor_idx_);
			else if (window_utils::ctrl_pressed()) action(CMD_SHELL_MENU, type, list_cursor_idx_);
			else popup_menu(type, list_cursor_idx_);
			last_area = -1;
		}
	}

	// Sense the direction the pointer is moving
	int pointer_moving_dir(int x, int y) noexcept {
		static int last_x, last_y;
		int dir{};

		const int w = abs(x - last_x);
		const int h = abs(y - last_y);

		if (w > h)      dir = (x - last_x < 0) ? 0 : 1;
		else if (w < h) dir = (y - last_y < 0) ? 2 : 3;
		else            dir = -1;

		last_x = x;
		last_y = y;
		return dir;
	}

	// Right justify file name by cursor position
	void change_item_align(int x, int y) noexcept {
		if (!is_cur_sel_long_) return;
		RECT r = list_rect_;
		UINT align = 0;
		if (r.right / 4 * 3 < x) align = DT_RIGHT;
		if (align != cursor_align_) {  // Redraw
			cursor_align_ = align;
			r.top = y / cy_item_ * cy_item_;
			r.bottom = r.top + cy_item_;
			::InvalidateRect(wnd_, &r, FALSE);
			::UpdateWindow(wnd_);
		}
	}

	// Event Handler of WM_*BUTTONUP
	void wm_button_up(int vkey, int x, int y, WPARAM mkey) {
		if (re_.is_active()) {
			re_.close();
			return;  // Reject while renaming
		}
		if (mouse_down_area_ == 1) {  // Scroller
			::ReleaseCapture();
			mouse_down_y_ = mouse_down_area_ = -1;
			mouse_down_idx_.reset();
			mouse_down_top_idx_.reset();
			return;
		}
		Document::ListType type;
		const std::optional<size_t> cursor = line_to_index(y / cy_item_, type);
		if (
			!cursor.has_value() ||
			on_separator_click(vkey, cursor.value(), x, type) ||
			(type == Document::ListType::FILE && doc_.is_file_empty()) ||
			(!list_cursor_idx_ || !mouse_down_idx_) ||
			type != mouse_down_list_type_ ||
			get_item_area(x) != 2
		) {
			mouse_down_y_    = -1;
			mouse_down_area_ = -1;
			mouse_down_idx_.reset();
			mouse_down_top_idx_.reset();
			return;
		}

		const bool lbtn = (mkey & MK_LBUTTON) != 0;
		const bool rbtn = (mkey & MK_RBUTTON) != 0;

		if ((vkey == VK_RBUTTON && lbtn) || (vkey == VK_LBUTTON && rbtn)) {
			if (type == Document::ListType::FILE && list_cursor_idx_ != mouse_down_idx_) {
				select_file(mouse_down_idx_, list_cursor_idx_);
			}
			action(CMD_SHELL_MENU, type, list_cursor_idx_);
		} else if (vkey == VK_LBUTTON) {
			if (type == Document::ListType::FILE && (list_cursor_idx_ != mouse_down_idx_ || window_utils::ctrl_pressed())) {
				select_file(mouse_down_idx_, list_cursor_idx_);
			} else {
				action(CMD_OPEN, type, list_cursor_idx_);
			}
		} else if (vkey == VK_RBUTTON) {
			if (type == Document::ListType::FILE && list_cursor_idx_ != mouse_down_idx_) {
				select_file(mouse_down_idx_, list_cursor_idx_);
			}
			if (window_utils::ctrl_pressed()) {
				action(CMD_SHELL_MENU, type, list_cursor_idx_);
			} else {
				popup_menu(type, list_cursor_idx_);
			}
		} else if (vkey == VK_MBUTTON) {
			if (type == Document::ListType::FILE && list_cursor_idx_ != mouse_down_idx_ && doc_.arrange_favorites(mouse_down_idx_, list_cursor_idx_)) {
				doc_.Update();
			} else {
				action(CMD_FAVORITE, type, list_cursor_idx_);
			}
		}
		mouse_down_y_ = mouse_down_area_ = -1;
		mouse_down_idx_.reset();
		mouse_down_top_idx_.reset();
	}

	// Check the position of the pointer
	int get_item_area(int x) const noexcept {
		if (x < cx_side_) return 0;
		if (x >= list_rect_.right - cx_side_) return 1;
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
			if (list_rect_.right * 2 / 3 < x) {  // If it is more than two thirds
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

	void wm_key_down(WPARAM key) {
		const bool ctrl = window_utils::ctrl_pressed();
		if (ctrl || key == VK_APPS || key == VK_DELETE || key == VK_RETURN) {
			if (!list_cursor_idx_) return;
			if (ctrl) {
				if (key == VK_APPS) {
					action(CMD_SHELL_MENU, list_cursor_switch_, list_cursor_idx_);
				}  else if (_T('A') <= key && key <= _T('Z')) {
					accelerator(gsl::narrow<TCHAR>(key), list_cursor_switch_, list_cursor_idx_);
				}
			} else {
				switch (key) {
				case VK_APPS:   popup_menu(list_cursor_switch_, list_cursor_idx_); break;
				case VK_DELETE: action(CMD_DELETE, list_cursor_switch_, list_cursor_idx_); break;
				case VK_RETURN: action(CMD_OPEN, list_cursor_switch_, list_cursor_idx_); break;
				default: break;
				}
			}
		} else {
			if (key == VK_F3) {
				set_cursor_index(search_.find_next(list_cursor_idx_, doc_.get_files()), Document::ListType::FILE);
				return;
			}
			key_cursor(key);  // Cursor movement by key operation
			if (_T('A') <= key && key <= _T('Z')) {
				search_.key_search(gsl::narrow<wchar_t>(key));  // Key input search
			}
		}
	}

	// TODO Allow cursor movement to the navigation pane
	// Cursor key input
	void key_cursor(WPARAM key) {
		std::optional<size_t> index = list_cursor_idx_;
		const ItemList& files = doc_.get_files();

		if (!index || list_cursor_switch_ == Document::ListType::HIER) {
			set_cursor_index(scroll_list_top_idx_, Document::ListType::FILE);
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
				ht_.go_forward(scroll_list_top_idx_, doc_.current_path());
				doc_.move_to_lower(Document::ListType::FILE, i);
			}
			set_cursor_index(scroll_list_top_idx_, Document::ListType::FILE);
		}
	}

	// Specify cursor position
	void set_cursor_index(std::optional<size_t> index, Document::ListType type) {
		if (index.has_value() && type == Document::ListType::FILE) {
			if (index.value() < scroll_list_top_idx_) {
				set_scroll_list_top_index(index.value(), false);
			}
			else if (scroll_list_top_idx_ + scroll_list_line_num_ <= index.value()) {
				set_scroll_list_top_index(index.value() - scroll_list_line_num_ + 1, false);
			}
		}
		if (list_cursor_idx_ && (list_cursor_switch_ != type || (!index.has_value() || list_cursor_idx_ != index.value()))) {
			RECT r = list_rect_;
			r.top    = gsl::narrow_cast<long>(index_to_line(list_cursor_idx_.value(), list_cursor_switch_) * cy_item_);
			r.bottom = r.top + cy_item_;
			::InvalidateRect(wnd_, &r, FALSE);
		}
		if (!index.has_value() || (doc_.get_item(type, index.value())->data() & SEPA) != 0) {
			list_cursor_idx_.reset();
			tt_.inactivate();  // Hide tool tip
			::UpdateWindow(wnd_);
			return;
		}
		if (list_cursor_switch_ != type || list_cursor_idx_ != index) {
			list_cursor_idx_ = index.value();
			list_cursor_switch_ = type;
			RECT r = list_rect_;
			r.top    = gsl::narrow<long>(index_to_line(index.value(), type) * cy_item_);
			r.bottom = r.top + cy_item_;
			::InvalidateRect(wnd_, &r, FALSE);
		}
		if (list_cursor_switch_ != type) mouse_down_idx_.reset();
		::UpdateWindow(wnd_);  // Update here as curSelIsLong_ is referred below
		tt_.inactivate();  // Hide tool tip
		if (is_cur_sel_long_) {  // Show tool tip
			tt_.activate(doc_.get_item(type, index.value())->name(), list_rect_);
		} else if (doc_.in_bookmark() || doc_.in_history()) {
			tt_.activate(doc_.get_item(type, index.value())->path(), list_rect_);
		}
	}

	// Specify the start index of the scroll list
	void set_scroll_list_top_index(size_t i, bool update = true) noexcept {
		const ItemList& files = doc_.get_files();

		const size_t old = scroll_list_top_idx_;
		if (files.size() <= scroll_list_line_num_) {
			scroll_list_top_idx_ = 0;
		} else if (files.size() < scroll_list_line_num_ + i) {
			scroll_list_top_idx_ = files.size() - scroll_list_line_num_;
		} else {
			scroll_list_top_idx_ = i;
		}
		RECT r;
		GetClientRect(wnd_, &r);
		r.right -= cx_scroll_bar_;
		r.top    = gsl::narrow<long>(cy_item_ * doc_.get_navi_count());
		::ScrollWindow(wnd_, 0, gsl::narrow_cast<long>((old - scroll_list_top_idx_) * cy_item_), &r, &r);
		GetClientRect(wnd_, &r);
		r.left = r.right - cx_scroll_bar_;
		::InvalidateRect(wnd_, &r, FALSE);
		if (update) ::UpdateWindow(wnd_);
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
		PopupMenu pm(wnd_, &pref_);
		std::wstring cmd;
		std::vector<std::wstring> items;

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
		std::wstring cmd;
		PopupMenu pm(wnd_, &pref_);
		if (pm.get_accel_command(type, accelerator, cmd)) action(ope_, cmd, w, index);
	}

	// Command execution
	void action(const std::wstring& cmd, Document::ListType w, std::optional<size_t> index) {
		doc_.set_operator(index, w, ope_);
		action(ope_, cmd, w, index);
	}

	void action(const Selection &objs, const std::wstring& cmd, Document::ListType w, std::optional<size_t> index) {
		const bool has_obj = objs.size() != 0 && !objs[0].empty();
		std::wstring old_current;

		if (has_obj) {
			old_current.assign(file_system::current_directory_path());
			::SetCurrentDirectory(path::parent(objs[0]).c_str());
			doc_.set_history(objs[0]);
		}
		if (cmd.front() == _T('<')) {
			if (index) {
				system_command(cmd, objs, w, index.value());
			}
		} else {
			if (!has_obj) return;
			::ShowWindow(wnd_, SW_HIDE);  // Hide in advance
			ope_.open_by(cmd);
		}
		if (has_obj) ::SetCurrentDirectory(old_current.c_str());
	}

	// System command execution
	void system_command(const std::wstring& cmd, const Selection& objs, Document::ListType w, size_t index) {
		if (cmd == CMD_SELECT_ALL)    { select_file(0, doc_.get_file_count() - 1, true); return; }
		if (cmd == CMD_RENAME)        {
			re_.open(objs[0], gsl::narrow<long>(index_to_line(index, w) * cy_item_), list_rect_.right, cy_item_);
			return;
		}
		if (cmd == CMD_POPUP_INFO)    { popup_info(objs, w, index); return; }
		if (cmd == CMD_CLEAR_HISTORY) { doc_.clear_history(); return; }
		if (cmd == CMD_FAVORITE)      { doc_.add_or_remove_favorite(objs[0], w, index); return; }  // Update here, so do nothing after return
		if (cmd == CMD_START_DRAG)    { ::SetCursor(::LoadCursor(nullptr, IDC_NO)); ::ShowWindow(wnd_, SW_HIDE); ope_.start_drag(); return; }
		if (cmd == CMD_SHELL_MENU)    {
			UINT f;
			const POINT pt = get_popup_pt(w, index, f);
			ope_.popup_shell_menu(pt, f);
			return;
		}
		if (ope_.command(cmd) == -1) ::ShowWindow(wnd_, SW_HIDE);
	}

	// Select file by range specification
	void select_file(std::optional<size_t> front_opt, std::optional<size_t> back_opt, bool all = false) {
		if (!front_opt || !back_opt || doc_.in_drives()) return;
		size_t front = front_opt.value();
		size_t back  = back_opt.value();
		if (back < front) std::swap(front, back);
		doc_.select_file(front, back, Document::ListType::FILE, all);

		if (front < scroll_list_top_idx_) front = scroll_list_top_idx_;
		if (back >= scroll_list_top_idx_ + scroll_list_line_num_) back = scroll_list_top_idx_ + scroll_list_line_num_ - 1;

		RECT lr = list_rect_;
		lr.top    = gsl::narrow<long>(cy_item_ * (front - scroll_list_top_idx_ + doc_.get_navi_count()));
		lr.bottom = gsl::narrow<long>(cy_item_ * (back  - scroll_list_top_idx_ + doc_.get_navi_count() + 1));
		::InvalidateRect(wnd_, &lr, FALSE);

		// Update selected file count display
		lr.top    = gsl::narrow<long>(cy_item_ * (doc_.get_navi_count() - 1));
		lr.bottom = lr.top + cy_item_;
		::InvalidateRect(wnd_, &lr, FALSE);
		::UpdateWindow(wnd_);
	}

	// Display file information
	void popup_info(const Selection&, Document::ListType w, size_t index) {
		std::vector<std::wstring> items;
		ope_.create_information_strings(items);
		items.push_back(_T("...more"));
		UINT f{};

		HMENU hmenu = ::CreatePopupMenu();
		if (hmenu == nullptr) return;

		for (size_t i = 0; i < items.size(); ++i) {
			::AppendMenu(hmenu, MF_STRING, i, items.at(i).c_str());
		}
		const POINT pt = get_popup_pt(w, index, f);
		const int ret  = ::TrackPopupMenu(hmenu, TPM_RETURNCMD | TPM_LEFTBUTTON | f, pt.x, pt.y, 0, wnd_, nullptr);
		::DestroyMenu(hmenu);
		if (gsl::narrow<size_t>(ret) == items.size() - 1) {
			ope_.popup_file_property();
		}
	}

	// Find the position of the popup
	POINT get_popup_pt(Document::ListType w, size_t index, UINT &f) noexcept {
		f = TPM_RETURNCMD;
		RECT r{};
		::GetWindowRect(wnd_, &r);

		const size_t l = index_to_line(index, w);
		POINT pt = { 0, gsl::narrow<long>(l * cy_item_) };
		::ClientToScreen(wnd_, &pt);
		menu_top_() = pt.y;
		if (popup_pos_ == 0 || popup_pos_ == 3) {
			pt.x = r.right - 6 - ::GetSystemMetrics(SM_CXFIXEDFRAME);
		} else {
			f |= TPM_RIGHTALIGN;
			pt.x = r.left + ::GetSystemMetrics(SM_CXFIXEDFRAME);
		}
		return pt;
	}

	// Return line from index and type
	size_t index_to_line(size_t index, Document::ListType type) noexcept {
		return index + (doc_.get_navi_count() - scroll_list_top_idx_) * static_cast<size_t>(type == Document::ListType::FILE);
	}

	// Return index and type from line
	std::optional<size_t> line_to_index(size_t line, Document::ListType &type) noexcept {
		if (line < doc_.get_navi_count()) {
			type = Document::ListType::HIER;
			return line;
		}
		type = Document::ListType::FILE;
		const size_t i = line - doc_.get_navi_count() + scroll_list_top_idx_;
		return (i < doc_.get_file_count()) ? std::optional<size_t>{i} : std::nullopt;
	}

public:

	// Window size position adjustment
	void updated() override {
		set_scroll_list_top_index(ht_.index());
		set_cursor_index(std::nullopt, Document::ListType::FILE);
		scroll_list_line_num_ = (list_rect_.bottom - doc_.get_navi_count() * cy_item_) / cy_item_;

		tt_.inactivate();
		::InvalidateRect(wnd_, nullptr, TRUE);
		::UpdateWindow(wnd_);
	}

};
