/**
 * Shell Execution
 *
 * @author Takuto Yanagida
 * @version 2025-11-13
 */

#pragma once

#include <string>

#include <shlobj.h>

#include "gsl/gsl"
#include "path.hpp"
#include "file_system.hpp"

namespace execute {

	bool shell_execute(HWND wnd, const std::wstring& obj, const wchar_t* opt = nullptr) noexcept {
		[[gsl::suppress(type.7)]]
		SHELLEXECUTEINFO sei{};
		sei.cbSize       = sizeof(SHELLEXECUTEINFO);
		sei.fMask        = SEE_MASK_FLAG_NO_UI | SEE_MASK_NOASYNC | SEE_MASK_FLAG_LOG_USAGE;  // To suppress that a caution dialog is shown
		sei.hwnd         = wnd;
		sei.lpVerb       = nullptr;
		sei.lpFile       = obj.c_str();
		sei.lpParameters = opt;  // A nullptr and an empty string make difference
		sei.lpDirectory  = nullptr;
		sei.nShow        = SW_SHOW;
		sei.hInstApp     = nullptr;
		if (::ShellExecuteEx(&sei) == TRUE) {
			return true;
		}
		// Work around for executing files in OneDrive
		sei.lpVerb       = L"open";
		sei.lpFile       = L"explorer";
		sei.lpParameters = obj.c_str();
		return ::ShellExecuteEx(&sei) == TRUE;
	}

	// Open file
	bool open(HWND wnd, const std::wstring& obj) {
		if (file_system::is_directory(obj) && file_system::is_existing_same_name_execution_file(obj)) {
			std::wstring qobj(L"\"");
			qobj.append(obj).append(L"\\\"");
			return shell_execute(wnd, qobj);
		}
		return shell_execute(wnd, obj, L"");
	}

	// Open files with a specific application
	bool open(HWND wnd, const std::vector<std::wstring>& objs, const std::wstring& line) {
		auto ret = file_system::extract_command_line_string(line, objs);
		return shell_execute(wnd, ret.first, ret.second.c_str());
	}

};
