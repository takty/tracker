/**
 *
 * Clipboard Operations
 *
 * @author Takuto Yanagida
 * @version 2021-05-08
 *
 */


#pragma once

#include <string>
#include <vector>

#include <windows.h>
#include <shlobj.h>

#include "Path.hpp"
#include "Link.hpp"
#include "Shell.hpp"


class Clipboard {

	const HWND hWnd_;

public:

	Clipboard(HWND hWnd) noexcept : hWnd_(hWnd) {}

	Clipboard(const Clipboard& inst) = delete;
	Clipboard(Clipboard&& inst) = delete;
	Clipboard& operator=(const Clipboard& inst) = delete;
	Clipboard& operator=(Clipboard&& inst) = delete;

	~Clipboard() {}

	// Copy file paths to the clipboard
	bool copy_path(const std::vector<std::wstring>& paths) const {
		std::wstring str;
		for (const auto& e : paths) {
			if (1 < e.size() && e.at(e.size() - 1) == L':') {  // Drive
				str.append(e).append(L"\\\r\n");
			} else {
				str.append(e).append(L"\r\n");
			}
		}
		if (paths.size() == 1) str.resize(str.size() - 2);  // remove last \r\n

		auto h = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, (str.size() + 1) * sizeof(wchar_t));
		if (h == nullptr) return false;

		auto cd = static_cast<wchar_t*>(::GlobalLock(h));
		if (cd == nullptr) return false;
		::wcsncpy_s(cd, str.size() + 1, str.c_str(), str.size());
		::GlobalUnlock(h);

		bool ret = false;
		if (::OpenClipboard(nullptr)) {
			if (::EmptyClipboard()) {
				if (::SetClipboardData(CF_UNICODETEXT, h)) ret = true;
				::CloseClipboard();
			}
		}
		return ret;
	}

	// Paste as links in the directory
	bool paste_as_link_in(const std::wstring& dir) const {
		std::vector<wchar_t> buf(MAX_PATH);
		bool ret = false;

		if (!::OpenClipboard(hWnd_)) return false;

		auto hDrop = (HDROP) ::GetClipboardData(CF_HDROP);
		if (hDrop) {
			const int count = ::DragQueryFile(hDrop, 0xFFFFFFFF, nullptr, 0);
			for (int i = 0; i < count; ++i) {
				const size_t len = ::DragQueryFile(hDrop, i, nullptr, 0);  // without end NULL
				if (buf.size() < len + 1) {
					buf.resize(len + 1);  // add end NULL
				}
				::DragQueryFile(hDrop, i, buf.data(), buf.size());
				std::wstring target{ buf.data() };
				auto name = Path::name(target);
				if (Link::is_link(target)) {
					target = Link::resolve(target);
				} else {
					name.append(L".lnk");
				}
				auto path{ dir };
				path.append(L"\\").append(name);
				auto shortcut = FileSystem::unique_name(path);
				if (Link::create(shortcut, target)) ret = true;
			}
		}
		::CloseClipboard();
		return ret;
	}

};
