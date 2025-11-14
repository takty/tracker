/**
 * Shell File Operations
 *
 * @author Takuto Yanagida
 * @version 2025-11-14
 */

#pragma once

#include <string>

#include <shlobj.h>

#include "gsl/gsl"
#include "path.hpp"
#include "shell.hpp"

namespace operation {

	class ShellItem {

		IShellItem* si_ = nullptr;

	public:

		ShellItem(const std::wstring& path) noexcept {
			LPVOID dest = nullptr;
			auto p           = path::ensure_no_unc_prefix(path);
			const auto res   = ::SHCreateItemFromParsingName(p.c_str(), nullptr, IID_IShellItem, &dest);
			if (res == S_OK) {
				si_ = static_cast<IShellItem*>(dest);
			}
		}

		ShellItem(const ShellItem&) = delete;
		ShellItem& operator=(const ShellItem&) = delete;
		ShellItem(ShellItem&&) = delete;
		ShellItem& operator=(ShellItem&&) = delete;

		~ShellItem() noexcept {
			if (si_) {
				try {
					si_->Release();
				} catch (...) {
				}
			}
		}

		IShellItem* ptr() const noexcept {
			return si_;
		}

	};

	class ShellItemIdArray {

		shell::ItemIdChildList iicl_;
		IShellItemArray* sia_ = nullptr;

	public:

		ShellItemIdArray(const std::vector<std::wstring>& paths) : iicl_(paths) {
			auto parent_shf = iicl_.parent_shell_folder();
			auto& cs        = iicl_.child_list();

			if (parent_shf && !cs.empty()) {
				LPVOID ptr = cs.data();
				const auto res = ::SHCreateShellItemArray(nullptr, parent_shf, gsl::narrow<UINT>(cs.size()), static_cast<LPCITEMIDLIST*>(ptr), &sia_);
				if (res != S_OK) {
					sia_ = nullptr;
				}
			}
		}

		ShellItemIdArray(const ShellItemIdArray&) = delete;
		ShellItemIdArray& operator=(const ShellItemIdArray&) = delete;
		ShellItemIdArray(ShellItemIdArray&&) = delete;
		ShellItemIdArray& operator=(ShellItemIdArray&&) = delete;
		~ShellItemIdArray() = default;

		void release() {
			iicl_.release();
			if (sia_) {
				sia_->Release();
			}
		}

		IShellItemArray* shell_item_array() const noexcept {
			return sia_;
		}

	};

	bool perform_(IFileOperation* file_op_) {
		if (!file_op_) return false;
		const auto res = file_op_->PerformOperations();
		return SUCCEEDED(res);
	}

	template<typename F> bool do_multiple_files_op_(const std::vector<std::wstring>& paths, F fn) {
		if (paths.empty()) return false;

		auto need_update = false;
		std::wstring cur_parent;
		std::vector<std::wstring> cur_paths;

		for (const auto& p : paths) {
			auto parent = path::parent(p);
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

	IFileOperation* create_file_op_() {
		LPVOID dest = nullptr;
		const auto res = ::CoCreateInstance(CLSID_FileOperation, nullptr, CLSCTX_ALL, IID_IFileOperation, &dest);

		if (SUCCEEDED(res) && dest) {
			IFileOperation* fop = static_cast<IFileOperation*>(dest);
			if (fop) {
				const auto r = fop->SetOperationFlags(FOF_ALLOWUNDO | FOFX_ADDUNDORECORD | FOFX_NOMINIMIZEBOX);
				if (SUCCEEDED(r)) return fop;
				fop->Release();
			}
		}
		return nullptr;
	}

	void release_(IFileOperation* file_op_) {
		if (file_op_) {
			file_op_->Release();
		}
	}

	// Rename files
	bool rename(const std::wstring& path, const std::wstring& new_fname) {
		bool ret = false;
		IFileOperation* file_op = create_file_op_();
		if (file_op) {
			ShellItem path_si(path);
			if (path_si.ptr()) {
				const auto res = file_op->RenameItem(path_si.ptr(), new_fname.c_str(), nullptr);
				if (SUCCEEDED(res)) {
					ret = perform_(file_op);
				}
			}
			release_(file_op);
		}
		return ret;
	}

	// Copy one file
	bool copy_one_file(const std::wstring& path, const std::wstring& dest_dir, const std::wstring& new_fname) {
		bool ret = false;
		IFileOperation* file_op = create_file_op_();
		if (file_op) {
			ShellItem path_si(path);
			ShellItem dest_si(dest_dir);
			if (path_si.ptr() && dest_si.ptr()) {
				const auto res = file_op->CopyItem(path_si.ptr(), dest_si.ptr(), new_fname.c_str(), nullptr);
				if (SUCCEEDED(res)) {
					ret = perform_(file_op);
				}
			}
			release_(file_op);
		}
		return ret;
	}

	// Copy files
	bool copy_files(const std::vector<std::wstring>& paths, const std::wstring& dest_dir) {
		bool ret = false;
		IFileOperation* file_op = create_file_op_();
		if (file_op) {
			ShellItem dest_si(dest_dir);
			if (dest_si.ptr()) {
				ret = do_multiple_files_op_(paths, [&](const std::vector<std::wstring>& ps) {
					ShellItemIdArray sia(ps);
					if (sia.shell_item_array()) {
						const auto res = file_op->CopyItems(sia.shell_item_array(), dest_si.ptr());
						if (SUCCEEDED(res)) {
							sia.release();
							return perform_(file_op);
						}
					}
					sia.release();
					return false;
				});
			}
			release_(file_op);
		}
		return ret;
	}

	// Move files
	bool move_files(const std::vector<std::wstring>& paths, const std::wstring& dest_dir) {
		bool ret = false;
		IFileOperation* file_op = create_file_op_();
		if (file_op) {
			ShellItem dest_si(dest_dir);
			if (dest_si.ptr()) {
				ret = do_multiple_files_op_(paths, [&](const std::vector<std::wstring>& ps) {
					ShellItemIdArray sia(ps);
					if (sia.shell_item_array()) {
						const auto res = file_op->MoveItems(sia.shell_item_array(), dest_si.ptr());
						if (SUCCEEDED(res)) {
							sia.release();
							return perform_(file_op);
						}
					}
					sia.release();
					return false;
				});
			}
			release_(file_op);
		}
		return ret;
	}

	// Delete files
	bool delete_files(const std::vector<std::wstring>& paths) {
		bool ret = false;
		IFileOperation* file_op = create_file_op_();
		if (file_op) {
			ret = do_multiple_files_op_(paths, [&](const std::vector<std::wstring>& ps) {
				ShellItemIdArray sia(ps);
				if (sia.shell_item_array()) {
					const auto res = file_op->DeleteItems(sia.shell_item_array());
					if (SUCCEEDED(res)) {
						sia.release();
						return perform_(file_op);
					}
				}
				sia.release();
				return false;
			});
			release_(file_op);
		}
		return ret;
	}

};
