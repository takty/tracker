/**
 * Shortcut File Operations
 *
 * @author Takuto Yanagida
 * @version 2025-11-10
 */

#pragma once

#include <string>
#include <vector>

#include <Shlobj.h>

#include "path.hpp"
#include "file_system.hpp"

namespace link {

	IShellLink* get_shell_link_interface() noexcept {
		LPVOID ptr = nullptr;
		const auto hr = ::CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLink, &ptr);
		if (FAILED(hr) || ptr == nullptr) {
			return nullptr;
		}
		return static_cast<IShellLink*>(ptr);
	}

	IPersistFile* get_persist_file_interface(IShellLink* psl) {
		if (psl == nullptr) {
			return nullptr;
		}
		LPVOID ptr = nullptr;
		const auto hr = psl->QueryInterface(IID_IPersistFile, &ptr);
		if (FAILED(hr) || ptr == nullptr) {
			return nullptr;
		}
		return static_cast<IPersistFile*>(ptr);
	}

	// Check whether the path is a shortcut file
	bool is_link(const std::wstring& path) noexcept {
		return !file_system::is_directory(path) && path::ext(path) == L"lnk";
	}

	// Create shortcut file
	bool create(const std::wstring& shortcut_path, const std::wstring& target) {
		IShellLink* psl = get_shell_link_interface();
		if (psl == nullptr) return false;

		psl->SetPath(target.c_str());
		if (!file_system::is_directory(target)) {
			psl->SetWorkingDirectory(path::parent(target).c_str());
		}
		IPersistFile* ppf = get_persist_file_interface(psl);
		if (ppf) {
			auto sp = shortcut_path.c_str();
			ppf->Save(sp, TRUE);
			ppf->Release();
			psl->Release();
			return true;
		} else {
			psl->Release();
			return false;
		}
	}

	// Resolve shortcut and get the target path
	std::wstring resolve(const std::wstring& path) {
		std::wstring target;
		IShellLink* psl = get_shell_link_interface();
		if (psl == nullptr) return target;

		IPersistFile* ppf = get_persist_file_interface(psl);
		if (ppf) {
			auto sp = path.c_str();
			auto hr = ppf->Load(sp, STGM_READ);  // load target path
			if (SUCCEEDED(hr)) {
				std::vector<wchar_t> buf(MAX_PATH);
				hr = psl->GetPath(buf.data(), MAX_PATH, nullptr, SLGP_RAWPATH);
				if (SUCCEEDED(hr)) {
					target.append(buf.data());
				}
			}
			ppf->Release();
		}
		psl->Release();
		return target;
	}

};
