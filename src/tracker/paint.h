/**
 *
 * Paint
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

#include "pref.hpp"
#include "document.h"
#include "window_utils.h"


class Paint {

	enum { SEPA = 1, DIR = 2, HIDE = 4, LINK = 8, HIER = 16, SEL = 32, EMPTY = 64 };

	enum class IconType { NONE = 0, SQUARE = 1, CIRCLE = 2, SCIRCLE = 3 };

	HWND hwnd_;
	const Document& doc_;

	int side_w_{};
	int item_h_{};
	int sbar_w_{};
	HFONT mark_font_{ nullptr };
	HFONT item_font_{ nullptr };

	size_t scroll_size_{};
	size_t scroll_top_{};

	std::optional<size_t> cur_idx_{ std::nullopt };
	Document::ListType cur_type_{ Document::ListType::FILE };

	UINT cur_align_{};
	bool is_cur_long_{ false };

	int index_to_line(Document::ListType lt, const size_t index) noexcept {
		if (lt == Document::ListType::FILE) {
			const size_t navi_size = doc_.get_navi_list().size();
			return index + navi_size - scroll_top_;
		}
		return index;
	}

public:

	Paint(const HWND hwnd, const Document& doc) noexcept : doc_(doc) {
		hwnd_ = hwnd;
	}

	Paint(const Paint& inst)            = delete;
	Paint(Paint&& inst)                 = delete;
	Paint& operator=(const Paint& inst) = delete;
	Paint& operator=(Paint&& inst)      = delete;

	virtual ~Paint() noexcept {
		::DeleteObject(mark_font_);
		::DeleteObject(item_font_);
	}

	void load_pref(const Pref& pref) noexcept {
		const auto dpi = WindowUtils::get_dpi(hwnd_);
		const double dpi_fact_x = dpi.x / 96.0;
		const double dpi_fact_y = dpi.y / 96.0;

		side_w_ = static_cast<int>(pref.get(SECTION_WINDOW, KEY_SIDE_AREA_WIDTH, VAL_SIDE_AREA_WIDTH) * dpi_fact_x);
		item_h_ = static_cast<int>(pref.get(SECTION_WINDOW, KEY_LINE_HEIGHT, VAL_LINE_HEIGHT) * dpi_fact_y);
		sbar_w_ = static_cast<int>(6 * dpi_fact_x);

		std::wstring font_name = pref.get(SECTION_WINDOW, KEY_FONT_NAME, VAL_FONT_NAME);
		const int font_size = static_cast<int>(pref.get(SECTION_WINDOW, KEY_FONT_SIZE, VAL_FONT_SIZE) * dpi_fact_x);

		::DeleteObject(item_font_);
		item_font_ = ::CreateFont(font_size, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE, font_name.c_str());
		if (font_name.empty() || !item_font_) {
			item_font_ = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
		}
		mark_font_ = ::CreateFont(static_cast<int>(14 * dpi_fact_x), 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, SYMBOL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Marlett");
	}

	HFONT get_item_font() {
		return item_font_;
	}


	// ------------------------------------------------------------------------


	void set_scroll_size(size_t size) {
		scroll_size_ = size;
	}

	void set_scroll_top(size_t idx) {
		const size_t last = scroll_top_;
		scroll_top_ = idx;

		RECT r;
		::GetClientRect(hwnd_, &r);
		r.right -= sbar_w_;
		r.top    = item_h_ * doc_.get_navi_list().size();
		::ScrollWindow(hwnd_, 0, (last - scroll_top_) * item_h_, &r, &r);
	}

	void set_cursor(Document::ListType lt, std::optional<size_t> optIdx) {
		cur_type_ = lt;
		cur_idx_  = optIdx;
	}


	// ------------------------------------------------------------------------


	bool is_cursor_name_long() {
		return is_cur_long_;
	}

	bool update_item_align(UINT align, Document::ListType lt, size_t idx) noexcept {
		if (!is_cur_long_) return false;

		if (align != cur_align_) {  // Redraw
			cur_align_ = align;
			invalidate_item(lt, idx);
			return true;
		}
		return false;
	}


	// ------------------------------------------------------------------------


	void invalidate_item(Document::ListType lt, size_t idx) {
		RECT r{};
		::GetClientRect(hwnd_, &r);
		r.right -= sbar_w_;
		r.top    = index_to_line(lt, idx) * item_h_;
		r.bottom = r.top + item_h_;
		::InvalidateRect(hwnd_, &r, FALSE);
	}

	void invalidate_items(Document::ListType lt, size_t from, size_t to) {
		RECT r{};
		::GetClientRect(hwnd_, &r);
		r.right -= sbar_w_;
		r.top    = index_to_line(lt, from) * item_h_;
		r.bottom = index_to_line(lt, to)   * item_h_ + item_h_;
		::InvalidateRect(hwnd_, &r, FALSE);
	}

	void invalidate_scroll_bar() {
		RECT r{};
		::GetClientRect(hwnd_, &r);
		r.left = r.right - sbar_w_;
		::InvalidateRect(hwnd_, &r, FALSE);
	}


	// ------------------------------------------------------------------------


	void wm_paint() noexcept {
		RECT rc;
		PAINTSTRUCT ps;

		::GetClientRect(hwnd_, &rc);
		auto dc = ::BeginPaint(hwnd_, &ps);

		if (ps.rcPaint.right > rc.right - sbar_w_) draw_scroll_bar(dc);
		if (ps.rcPaint.left < rc.right - sbar_w_) {
			const size_t bgn = ps.rcPaint.top / item_h_;
			const size_t end = ps.rcPaint.bottom / item_h_;
			RECT r{};
			r.top = bgn * item_h_, r.bottom = r.top + item_h_, r.left = 0, r.right = rc.right - sbar_w_;

			const ItemList& navis = doc_.get_navi_list(), & files = doc_.get_file_list();

			for (size_t i = bgn; i <= end; ++i) {
				if (i < navis.size()) {
					const Item* fd = navis.at(i);
					if (fd) {
						if ((fd->data() & SEPA) != 0) {
							draw_separator(dc, r, (fd->data() == (SEPA | HIER)));
						}
						else {
							draw_item(dc, r, fd, cur_type_ == Document::ListType::NAVI && cur_idx_ && i == cur_idx_.value());
						}
					}
				}
				else if (i - navis.size() + scroll_top_ < files.size()) {
					const size_t t = i - navis.size() + scroll_top_;
					draw_item(dc, r, files.at(t), cur_type_ == Document::ListType::FILE && cur_idx_ && t == cur_idx_.value());
				}
				else {
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
		const size_t size = doc_.get_file_list().size();

		::GetClientRect(hwnd_, &rc);
		rc.left = rc.right - sbar_w_;
		if (size <= scroll_size_) {
			::FillRect(dc, &rc, (HBRUSH)(COLOR_MENU + 1));
			return;
		}
		const double d = 1.0 * rc.bottom / size;
		RECT t = rc;
		t.bottom = (LONG)(d * scroll_top_);
		::FillRect(dc, &t, (HBRUSH)(COLOR_BTNSHADOW + 1));
		if (scroll_size_ + scroll_top_ < size) {
			t.top = t.bottom;
			t.bottom += (LONG)(d * scroll_size_);
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
	void draw_separator(HDC dc, RECT r, bool isHier) noexcept {
		const std::wstring sortBy{ L"nedsNEDS" };
		const size_t size = doc_.get_file_list().size();
		SIZE font;

		::FillRect(dc, &r, (HBRUSH)(COLOR_MENU + 1));
		::SelectObject(dc, item_font_);  // Font selection (do here because we measure the size below)
		if (isHier) {
			std::wstring num;
			if (doc_.selected_size()) {
				if (doc_.selected_size() == size) {
					num.assign(L"ALL / ");
				}
				else {
					num.assign(std::to_wstring(doc_.selected_size())).append(L" / ");
				}
			}
			num.append(std::to_wstring(size));
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
			WindowUtils::draw_gray_text(dc, r, L"x");
			::GetTextExtentPoint32(dc, L"x", 1, &font);
			r.left = font.cx + 2;
		}
		::InflateRect(&r, -3, item_h_ / -2);
		::DrawEdge(dc, &r, EDGE_ETCHED, BF_TOP);
	}

	// Draw an item
	void draw_item(HDC dc, RECT r, const Item* fd, bool cur) noexcept {
		if (!fd) return;
		::FillRect(dc, &r, (HBRUSH)((cur ? COLOR_HIGHLIGHT : COLOR_MENU) + 1));  // Draw the background
		if (fd->is_empty()) {
			::SetTextColor(dc, GetSysColor(COLOR_GRAYTEXT));
			::SetBkMode(dc, TRANSPARENT);
			::SelectObject(dc, item_font_);
			r.left = side_w_;
			is_cur_long_ = false;
			::DrawText(dc, L"empty", -1, &r, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
			return;
		}
		IconType type = IconType::NONE;
		int color = fd->color();
		if (fd->is_link()) {  // Is shortcut?
			if (color == -1) color = ::GetSysColor(COLOR_GRAYTEXT), type = IconType::SCIRCLE;
			else type = IconType::CIRCLE;
		}
		else if (color != -1) {
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
			is_cur_long_ = font.cx > r.right - r.left;  // File name at cursor position is out
		}
		::DrawText(dc, fd->name().c_str(), -1, &r, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | (cur_align_ * (is_cur_long_ ? 1 : 0) * cur));
	}

	// Draw a mark
	void draw_mark(HDC dc, RECT r, IconType type, int color, bool cur, bool sel, bool dir) noexcept {
		const wchar_t* c = nullptr;
		RECT rl = r, rr = r;

		rl.right = side_w_, rr.left = r.right - side_w_;
		::SetBkMode(dc, TRANSPARENT);
		::SelectObject(dc, mark_font_);  // Select font for symbols
		if (type == IconType::SQUARE)       c = L"g";
		else if (type == IconType::CIRCLE)  c = L"n";
		else if (type == IconType::SCIRCLE) c = L"i";
		if (c) {
			::SetTextColor(dc, color);
			::DrawText(dc, c, 1, &rl, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
		}
		// Draw selection marks and folder marks
		::SetTextColor(dc, ::GetSysColor(cur ? COLOR_HIGHLIGHTTEXT : COLOR_MENUTEXT));
		if (sel) ::DrawText(dc, L"a", 1, &rl, 0x0025);
		if (dir) ::DrawText(dc, L"4", 1, &rr, 0x0025);
	}

};
