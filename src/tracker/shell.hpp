/**
 * Shell Object Operations
 *
 * @author Takuto Yanagida
 * @version 2025-11-13
 */

#pragma once

#include <vector>
#include <string>

#include <windows.h>
#include <shlobj.h>

#include "gsl/gsl"
#include "path.hpp"

namespace shell {

	LPSHELLFOLDER get_shell_folder(const std::wstring& path) noexcept {
		PIDLIST_ABSOLUTE parent_id;

		auto p = path::ensure_no_unc_prefix(path);
		const auto res = ::SHParseDisplayName(p.data(), nullptr, &parent_id, 0, nullptr);  // This function can handle a super long path without UNC token

		LPVOID parent_shf = nullptr;
		if (res == S_OK) {
			::SHBindToObject(nullptr, parent_id, nullptr, IID_IShellFolder, &parent_shf);
			::CoTaskMemFree(parent_id);
		}
		return static_cast<LPSHELLFOLDER>(parent_shf);
	}

	LPSHELLFOLDER get_parent_shell_folder(const std::wstring& path) noexcept {
		HRESULT res{};

		PIDLIST_ABSOLUTE parent_id{};
		if (path::is_root(path)) {
			res = ::SHGetSpecialFolderLocation(nullptr, CSIDL_DRIVES, &parent_id);
		} else {
			auto p = path::ensure_no_unc_prefix(path::parent(path));
			res = ::SHParseDisplayName(p.data(), nullptr, &parent_id, 0, nullptr);  // This function can handle a super long path without UNC token
		}
		LPVOID parent_shf = nullptr;
		if (res == S_OK) {
			::SHBindToObject(nullptr, parent_id, nullptr, IID_IShellFolder, &parent_shf);
			::CoTaskMemFree(parent_id);
		}
		return static_cast<LPSHELLFOLDER>(parent_shf);
	}

	class ItemIdChildList {

		LPSHELLFOLDER parent_shf_;
		std::vector<PITEMID_CHILD> ids_;

	public:

		ItemIdChildList(const std::vector<std::wstring>& paths) {
			parent_shf_ = get_parent_shell_folder(paths.front());
			if (parent_shf_ == nullptr) {
				return;
			}
			for (const auto& e : paths) {
				auto fname = path::name(e);
				if (path::is_root(e)) {
					fname += L'\\';
				}
				PITEMID_CHILD id;
				const auto res = parent_shf_->ParseDisplayName(nullptr, nullptr, fname.data(), nullptr, &id, nullptr);
				if (res == S_OK) {
					ids_.push_back(id);
				}
			}
		}

		ItemIdChildList(const ItemIdChildList&) = delete;
		ItemIdChildList& operator=(const ItemIdChildList&) = delete;
		ItemIdChildList(ItemIdChildList&&) = delete;
		ItemIdChildList& operator=(ItemIdChildList&&) = delete;
		~ItemIdChildList() = default;

		const void release() {
			for (auto e : ids_) {
				::CoTaskMemFree(e);
			}
			parent_shf_->Release();
		}

		const LPSHELLFOLDER parent_shell_folder() const noexcept {
			return parent_shf_;
		}

		const std::vector<PITEMID_CHILD>& child_list() const noexcept {
			return ids_;
		}

		std::vector<PITEMID_CHILD>& child_list() noexcept {
			return ids_;
		}

	};

	LPVOID get_ole_ui_object(const std::vector<std::wstring>& paths, REFIID riid) {
		LPVOID ret = nullptr;

		ItemIdChildList iicl(paths);
		const auto parent_shf = iicl.parent_shell_folder();

		if (parent_shf) {
			auto &cs    = iicl.child_list();
			LPVOID ptr  = cs.data();
			LPVOID dest = nullptr;
			const auto res = parent_shf->GetUIObjectOf(nullptr, gsl::narrow<UINT>(cs.size()), static_cast<LPCITEMIDLIST*>(ptr), riid, nullptr, &dest);
			if (res == S_OK) {
				ret = dest;
			}
		}
		iicl.release();
		return ret;
	}

	HRESULT invoke(const LPCONTEXTMENU cm, const char* cmd) {
		if (!cm) return 0;
		CMINVOKECOMMANDINFO ici{};
		ici.cbSize = sizeof(ici);
		ici.fMask = 0;
		ici.hwnd = nullptr;
		ici.lpVerb = cmd;
		ici.lpParameters = nullptr;
		ici.lpDirectory = nullptr;
		ici.nShow = SW_SHOWNORMAL;
		return cm->InvokeCommand(&ici);
	}

	HRESULT execute(const std::vector<std::wstring>& paths, const char* cmd) {
		auto cm = static_cast<LPCONTEXTMENU>(shell::get_ole_ui_object(paths, IID_IContextMenu));
		if (!cm) return 0;

		const auto res = invoke(cm, cmd);
		cm->Release();
		return res;
	}

	// Open files by context menu command
	void open(const std::vector<std::wstring>& paths) {
		execute(paths, "open");
	}

	// Copy files by context menu command
	void copy(const std::vector<std::wstring>& paths) {
		execute(paths, "copy");
	}

	// Cut files by context menu command
	void cut(const std::vector<std::wstring>& paths) {
		execute(paths, "cut");
	}

	// Paste files in the path_0 directory by context menu command
	void paste_in(const std::vector<std::wstring>& path_0) {
		execute(path_0, "paste");
	}

	// Paste files as links in the path_0 directory by context menu command
	void paste_as_link_in(const std::vector<std::wstring>& path_0) {
		execute(path_0, "pastelink");
	}

	// Delete files by context menu command
	void delete_files(const std::vector<std::wstring>& paths) {
		execute(paths, "delete");
	}

	// Show the property of files
	void show_property(const std::vector<std::wstring>& objs) {
		// Instead of calling execute function with command "properties"...
		auto cm = static_cast<LPDATAOBJECT>(shell::get_ole_ui_object(objs, IID_IDataObject));
		if (!cm) return;
		::SHMultiFileProperties(cm, 0);
		cm->Release();
	}

};
