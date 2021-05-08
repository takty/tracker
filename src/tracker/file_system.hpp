/**
 *
 * File System Operations
 *
 * @author Takuto Yanagida
 * @version 2021-05-08
 *
 */


#pragma once

#include <string>
#include <vector>

#include <FileAPI.h>
#include <Shlobj.h>

#include "Path.hpp"


class FileSystem {

	// Calculate file/directory size internally
	static bool calc_file_size_internally(const std::wstring& path, uint64_t& size, const uint64_t& limitTime) {
		if (limitTime > 0 && ::GetTickCount64() > limitTime) return false;
		bool success = true;

		find_first_file(path, [&](const std::wstring& parent, const WIN32_FIND_DATA& wfd) {
			if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				if (!calc_file_size_internally(parent + (TCHAR*) wfd.cFileName, size, limitTime)) {
					success = false;
					return false;  // break
				}
			} else {
				size += (((uint64_t) wfd.nFileSizeHigh) << 32) + wfd.nFileSizeLow;
			}
			return true;  // continue
		});
		return success;
	}

public:

	// Extract command line string (path|opt)
	static std::pair<std::wstring, std::wstring> extract_command_line_string(const std::wstring& line, const std::vector<std::wstring>& objs) {
		const auto sep = line.find_first_of(L'|');
		if (sep == std::wstring::npos) {
			return { Path::absolute_path(line, module_file_path()), Path::space_separeted_quoted_paths_string(objs) };
		}
		auto opt = line.substr(sep + 1);

		// %path% -> "path\file"
		auto pos = opt.find(L"%path%");
		if (pos != std::wstring::npos) {
			opt.replace(pos, 6, Path::quote(objs.front()));
		}
		// %paths% -> "path\file1" "path\file2"...
		pos = opt.find(L"%paths%");
		if (pos != std::wstring::npos) {
			opt.replace(pos, 7, Path::space_separeted_quoted_paths_string(objs));
		}
		// %file% -> "file"
		pos = opt.find(L"%file%");
		if (pos != std::wstring::npos) {
			opt.replace(pos, 6, Path::quote(Path::name(objs.front())));
		}
		return{ Path::absolute_path(line.substr(0, sep), module_file_path()), opt };
	}

	// Make unique new name
	static std::wstring unique_name(const std::wstring& obj, const std::wstring& post = std::wstring()) {
		if (Path::is_root(obj)) return L"";  // Failure
		const auto path = Path::parent(obj);
		auto name = Path::name(obj);

		std::wstring ext;
		if (!is_directory(obj)) {
			if (name.at(0) != Path::EXT_PREFIX) {  // When the file is not dot file
				name = Path::name_without_ext(obj);
				ext = Path::ext(obj);
				if (!ext.empty()) ext.insert(std::begin(ext), Path::EXT_PREFIX);
			}
		}
		std::wstring ret{ path };
		ret.append(1, Path::PATH_SEPARATOR).append(name).append(post).append(ext);

		for (int i = 1;; ++i) {
			if (!is_existing(ret)) break;
			auto count = L"(" + std::to_wstring(i) + L")";
			ret.assign(path).append(1, Path::PATH_SEPARATOR).append(name).append(post).append(count).append(ext);
		}
		return ret;
	}

	// Check whether there is an execution file that has the same name as the name of path
	static bool is_existing_same_name_execution_file(const std::wstring& path) {
		bool ret = false;
		auto temp = Path::parent(path);
		if (temp.empty()) return false;  // path is root

		find_first_file(temp, [&](const std::wstring&, const WIN32_FIND_DATA& wfd) {
			auto e = Path::ext((TCHAR*) wfd.cFileName);
			if (e == L"exe" || e == L"bat") {
				ret = true;
				return false;  // break;
			}
			return true;  // continue
		});
		return ret;
	}

	// Check whether the path is a removable disk
	static bool is_removable(const std::wstring& path) noexcept {
		return ::GetDriveType(path.c_str()) == DRIVE_REMOVABLE;
	}

	// Check whether the path is a directory
	static bool is_directory(const std::wstring& path) {
		auto attr = ::GetFileAttributes(Path::ensure_unc_prefix(path).c_str());
		return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
	}

	// Check whether the file of the path is existing
	static bool is_existing(const std::wstring& path) {
		auto attr = ::GetFileAttributes(Path::ensure_unc_prefix(path).c_str());
		return attr != INVALID_FILE_ATTRIBUTES;
	}

	// Get desktop directory path
	static std::wstring desktop_path() {
		std::wstring ret;
		PWSTR buf = nullptr;
		if (::SHGetKnownFolderPath(FOLDERID_Desktop, 0, nullptr, &buf) == S_OK) {
			ret.assign(buf);
			::CoTaskMemFree(buf);
		}
		return ret;
	}

	// Get the exe file path
	static std::wstring module_file_path() {
		std::vector<wchar_t> buf(MAX_PATH);

		while (true) {
			::GetModuleFileName(nullptr, buf.data(), buf.size());
			if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) break;
			buf.resize(buf.size() * 2);
		}
		return std::wstring(buf.data());
	}

	// Get the current directory path
	static std::wstring current_directory_path() {
		std::vector<wchar_t> buf(MAX_PATH);

		while (true) {
			const auto len = ::GetCurrentDirectory(buf.size(), buf.data());
			if (len < buf.size()) break;
			buf.resize(len);
		}
		return std::wstring(buf.data());
	}

	// Get drive size
	static void drive_size(const std::wstring& path, uint64_t& size, uint64_t& free) noexcept {
		::GetDiskFreeSpaceEx(path.c_str(), (PULARGE_INTEGER) &free, (PULARGE_INTEGER) &size, nullptr);
	}

	// Calculate file/directory size
	static bool calc_file_size(const std::wstring& path, uint64_t& size, const uint64_t& limitTime) {
		if (!is_directory(path)) {
			auto hf = ::CreateFile(path.c_str(), 0, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS, nullptr);
			if (hf == INVALID_HANDLE_VALUE) return false;
			::GetFileSizeEx(hf, (PLARGE_INTEGER) &size);
			::CloseHandle(hf);
			return true;
		}
		size = 0;
		return calc_file_size_internally(path, size, limitTime);
	}

	// Template version of find first file
	template<typename F> static bool find_first_file(const std::wstring& path, F fn) {
		auto parent{ path };
		parent += (parent.at(parent.size() - 1) == Path::PATH_SEPARATOR) ? L"*" : L"\\*";

		WIN32_FIND_DATA wfd{};
		auto sh = ::FindFirstFileEx(parent.c_str(), FindExInfoBasic, &wfd, FindExSearchNameMatch, nullptr, 0);
		if (sh == INVALID_HANDLE_VALUE) return false;
		parent.resize(parent.size() - 1);
		do {
			if (!::lstrcmp((TCHAR*) wfd.cFileName, L".") || !::lstrcmp((TCHAR*) wfd.cFileName, L"..")) continue;
			if (!fn(parent, wfd)) break;
		} while (::FindNextFile(sh, &wfd));
		::FindClose(sh);
		return true;
	}

};
