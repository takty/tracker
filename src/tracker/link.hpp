/**
 * Shortcut File Operations
 *
 * @author Takuto Yanagida
 * @version 2025-10-22
 */

#pragma once

#include <string>
#include <vector>

#include <Shlobj.h>

#include "classes.h"
#include "path.hpp"
#include "file_system.hpp"

class Link {

public:

	// Check whether the path is a shortcut file
	static bool is_link(const std::wstring& path) {
		return !FileSystem::is_directory(path) && Path::ext(path) == L"lnk";
	}

	// Create shortcut file
	static bool create(const std::wstring& shortcut_path, const std::wstring& target) {
		IShellLink* psl{};
		auto hr = ::CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLink, reinterpret_cast<LPVOID*>(&psl));
		if (FAILED(hr) || !psl) return false;

		psl->SetPath(target.c_str());
		if (!FileSystem::is_directory(target)) {
			psl->SetWorkingDirectory(Path::parent(target).c_str());
		}
		IPersistFile* ppf{};
		hr = psl->QueryInterface(IID_IPersistFile, reinterpret_cast<LPVOID*>(&ppf));
		if (SUCCEEDED(hr) && ppf) {
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
	static std::wstring resolve(const std::wstring& path) {
		std::wstring target;
		IShellLink* psl{};

		auto hr = ::CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLink, reinterpret_cast<LPVOID*>(&psl));
		if (FAILED(hr) || !psl) return target;

		IPersistFile* ppf{};
		hr = psl->QueryInterface(IID_IPersistFile, reinterpret_cast<LPVOID*>(&ppf));
		if (SUCCEEDED(hr) && ppf) {
			auto sp = path.c_str();
			hr = ppf->Load(sp, STGM_READ);  // load target path
			if (SUCCEEDED(hr)) {
				std::vector<wchar_t> buf(MAX_PATH);
				hr = psl->GetPath(buf.data(), MAX_PATH, nullptr, SLGP_RAWPATH);
				if (SUCCEEDED(hr)) {
					target.append(buf.data());
				}
				//while (true) {
				//	cch *= 2;
				//	buf.resize(cch);
				//}
				//target.append(buf.data());
			}
			ppf->Release();
		}
		psl->Release();
		return target;
	}

};
