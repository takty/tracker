/**
 *
 * Shell Object Operations
 *
 * @author Takuto Yanagida
 * @version 2020-03-22
 *
 */


#pragma once

#include <vector>
#include <string>

#include <windows.h>
#include <shlobj.h>

#include "Path.hpp"


class Shell {

public:

	class ItemIdChildList {

		LPSHELLFOLDER parent_shf_;
		std::vector<PITEMID_CHILD> ids_;

	public:

		ItemIdChildList(const std::vector<std::wstring>& paths) {
			parent_shf_ = Shell::get_parent_shell_folder(paths.front());
			if (parent_shf_ == nullptr) return;

			for (const auto& e : paths) {
				auto fname = Path::name(e);
				if (Path::is_root(e)) fname += L'\\';
				PITEMID_CHILD id;
				auto res = parent_shf_->ParseDisplayName(nullptr, nullptr, (LPWSTR)fname.c_str(), nullptr, &id, nullptr);
				if (res == S_OK) ids_.push_back(id);
			}
		}

		const LPSHELLFOLDER parent_shell_folder() {
			return parent_shf_;
		}

		const std::vector<PITEMID_CHILD>& child_list() {
			return ids_;
		}

		~ItemIdChildList() {
			for (auto e : ids_) ::CoTaskMemFree(e);
			parent_shf_->Release();
		}

	};

	static LPSHELLFOLDER get_shell_folder(const std::wstring& path) {
		PIDLIST_ABSOLUTE parent_id;
		auto p = Path::ensure_no_unc_prefix(path);
		auto res = ::SHParseDisplayName(p.c_str(), nullptr, &parent_id, 0, nullptr);  // This function can handle a super long path without UNC token

		LPSHELLFOLDER parent_shf = nullptr;
		if (res == S_OK) {
			res = ::SHBindToObject(nullptr, parent_id, nullptr, IID_IShellFolder, (void**)&parent_shf);
			::CoTaskMemFree(parent_id);
		}
		return parent_shf;
	}

	static LPSHELLFOLDER get_parent_shell_folder(const std::wstring& path) {
		HRESULT res;

		PIDLIST_ABSOLUTE parent_id;
		if (Path::is_root(path)) {
			res = ::SHGetSpecialFolderLocation(nullptr, CSIDL_DRIVES, &parent_id);
		} else {
			auto p = Path::ensure_no_unc_prefix(Path::parent(path));
			res = ::SHParseDisplayName(p.c_str(), nullptr, &parent_id, 0, nullptr);  // This function can handle a super long path without UNC token
		}
		LPSHELLFOLDER parent_shf = nullptr;
		if (res == S_OK) {
			res = ::SHBindToObject(nullptr, parent_id, nullptr, IID_IShellFolder, (void**)&parent_shf);
			::CoTaskMemFree(parent_id);
		}
		return parent_shf;
	}

	static LPVOID get_ole_ui_object(const std::vector<std::wstring>& paths, REFIID riid) {
		ItemIdChildList sidcl(paths);
		auto parent_shf = sidcl.parent_shell_folder();
		const auto& cs = sidcl.child_list();

		if (parent_shf == nullptr) return nullptr;
		LPVOID ret_obj = nullptr;
		auto res = parent_shf->GetUIObjectOf(nullptr, cs.size(), (LPCITEMIDLIST*)cs.data(), riid, nullptr, &ret_obj);
		if (res == S_OK) return ret_obj;
		return nullptr;
	}

};
