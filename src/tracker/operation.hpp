/**
 *
 * Shell File Operations
 *
 * @author Takuto Yanagida
 * @version 2020-03-22
 *
 */


#pragma once

#include <string>

#include <shlobj.h>

#include "path.hpp"
#include "file_system.hpp"
#include "shell.hpp"


class Operation {

	class ShellItem {
		IShellItem* si_ = nullptr;

	public:
		ShellItem(const std::wstring path) {
			IShellItem* dest = nullptr;
			auto p = Path::ensure_no_unc_prefix(path);
			auto res = ::SHCreateItemFromParsingName(p.c_str(), nullptr, IID_IShellItem, (void**)&dest);
			if (res == S_OK) si_ = dest;
		}
		IShellItem* ptr() const {
			return si_;
		}
		~ShellItem() {
			if (si_) si_->Release();
		}
	};

	class ShellItemIdArray {
		Shell::ItemIdChildList iicl_;
		IShellItemArray* sia_ = nullptr;

	public:
		ShellItemIdArray(const std::vector<std::wstring>& paths) : iicl_(paths) {
			auto parent_shf = iicl_.parent_shell_folder();
			const auto& cs = iicl_.child_list();
			if (parent_shf != nullptr && !cs.empty()) {
				auto res = ::SHCreateShellItemArray(nullptr, parent_shf, cs.size(), (LPCITEMIDLIST*)cs.data(), &sia_);
				if (res != S_OK) sia_ = nullptr;
			}
		}

		IShellItemArray* shell_item_array() const {
			return sia_;
		}

		~ShellItemIdArray() {
			sia_->Release();
		}
	};

	IFileOperation* file_op_ = nullptr;

	bool perform() const {
		auto res = file_op_->PerformOperations();
		if (SUCCEEDED(res)) return true;
		return false;
	}

	template<typename F> bool do_multiple_files_op(const std::vector<std::wstring>& paths, F fn) const {
		if (paths.empty()) return false;

		auto need_update = false;
		std::wstring cur_parent;
		std::vector<std::wstring> cur_paths;

		for (const auto& p : paths) {
			auto parent = Path::parent(p);
			if (parent != cur_parent) {
				if (!cur_paths.empty()) {
					if (fn(cur_paths)) need_update = true;
				}
				cur_paths.clear();
				cur_parent = parent;
			}
			cur_paths.push_back(p);
		}
		if (!cur_paths.empty()) {
			if (fn(cur_paths)) need_update = true;
		}
		return need_update;
	}

	bool shell_execute(const std::wstring& obj, const wchar_t* opt = nullptr) const {
		SHELLEXECUTEINFO sei;
		sei.cbSize       = sizeof(SHELLEXECUTEINFO);
		sei.fMask        = SEE_MASK_FLAG_NO_UI | SEE_MASK_NOASYNC | SEE_MASK_FLAG_LOG_USAGE;  // To suppress that a caution dialog is shown
		sei.hwnd         = nullptr;
		sei.lpVerb       = nullptr;
		sei.lpFile       = obj.c_str();
		sei.lpParameters = opt;  // A nullptr and an empty string make difference
		sei.lpDirectory  = nullptr;
		sei.nShow        = SW_SHOW;
		return ::ShellExecuteEx(&sei) == TRUE;
	}

public:

	Operation() {
		IFileOperation* obj = nullptr;
		auto res = ::CoCreateInstance(CLSID_FileOperation, nullptr, CLSCTX_ALL, IID_IFileOperation, (void**)&obj);
		if (SUCCEEDED(res)) {
			res = obj->SetOperationFlags(FOF_ALLOWUNDO | FOFX_ADDUNDORECORD | FOFX_NOMINIMIZEBOX);
			if (SUCCEEDED(res)) file_op_ = obj;
		}
	}

	~Operation() {
		if (file_op_) file_op_->Release();
	}

	// Open file
	bool open(const std::wstring& obj) const {
		if (FileSystem::is_directory(obj) && FileSystem::is_existing_same_name_execution_file(obj)) {
			std::wstring qobj(L"\"");
			qobj.append(obj).append(L"\\\"");
			return shell_execute(qobj);
		}
		return shell_execute(obj, L"");
	}

	// Open files with a specific application
	bool open(const std::vector<std::wstring>& objs, const std::wstring& line) const {
		auto ret = FileSystem::extract_command_line_string(line, objs);
		return shell_execute(ret.first, ret.second.c_str());
	}

	// Rename files
	bool rename(const std::wstring& path, const std::wstring& new_fname) const {
		if (!file_op_) return false;
		ShellItem path_si(path);
		if (!path_si.ptr()) return false;

		auto res = file_op_->RenameItem(path_si.ptr(), new_fname.c_str(), nullptr);
		if (SUCCEEDED(res)) return perform();
		return false;
	}

	// Copy one file
	bool copy_one_file(const std::wstring& path, const std::wstring& dest_dir, const std::wstring& new_fname) const {
		if (!file_op_) return false;
		ShellItem path_si(path);
		if (!path_si.ptr()) return false;
		ShellItem dest_si(dest_dir);
		if (!dest_si.ptr()) return false;

		auto res = file_op_->CopyItem(path_si.ptr(), dest_si.ptr(), new_fname.c_str(), nullptr);
		if (SUCCEEDED(res)) return perform();
		return false;
	}

	// Copy files
	bool copy_files(const std::vector<std::wstring>& paths, const std::wstring& dest_dir) const {
		if (!file_op_) return false;
		ShellItem dest_si(dest_dir);
		if (!dest_si.ptr()) return false;

		return do_multiple_files_op(paths, [&](const std::vector<std::wstring>& ps) {
			ShellItemIdArray sia(ps);
			if (!sia.shell_item_array()) return false;

			auto res = file_op_->CopyItems(sia.shell_item_array(), dest_si.ptr());
			if (SUCCEEDED(res)) return perform();
			return false;
		});
	}

	// Move files
	bool move_files(const std::vector<std::wstring>& paths, const std::wstring& dest_dir) const {
		if (!file_op_) return false;
		ShellItem dest_si(dest_dir);
		if (!dest_si.ptr()) return false;

		return do_multiple_files_op(paths, [&](const std::vector<std::wstring>& ps) {
			ShellItemIdArray sia(ps);
			if (!sia.shell_item_array()) return false;

			auto res = file_op_->MoveItems(sia.shell_item_array(), dest_si.ptr());
			if (SUCCEEDED(res)) return perform();
			return false;
		});
	}

	// Delete files
	bool delete_files(const std::vector<std::wstring>& paths) const {
		if (!file_op_) return false;

		return do_multiple_files_op(paths, [&](const std::vector<std::wstring>& ps) {
			ShellItemIdArray sia(ps);
			if (!sia.shell_item_array()) return false;

			auto res = file_op_->DeleteItems(sia.shell_item_array());
			if (SUCCEEDED(res)) return perform();
			return false;
		});
	}

};
