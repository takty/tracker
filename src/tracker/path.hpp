/**
 * File Path Operations
 *
 * @author Takuto Yanagida
 * @version 2025-11-10
 */

#pragma once

#include <string>
#include <vector>
#include <algorithm>

const std::wstring PATH_EXT_DIR(L"<folder>");

namespace path {

	constexpr wchar_t PATH_SEPARATOR   = L'\\';  // File path separator
	constexpr wchar_t DRIVE_IDENTIFIER = L':';   // Drive identifier
	constexpr wchar_t EXT_PREFIX       = L'.';   // Extension prefix

	constexpr wchar_t const * const UNC_PREFIX = L"\\\\?\\";

	// Extract file name (UNC ok)
	std::wstring name(const std::wstring& path) noexcept {
		const auto size = path.size();
		const auto last = (path.back() == PATH_SEPARATOR) ? size - 2 : size - 1;
		const auto pos  = path.find_last_of(PATH_SEPARATOR, last);

		if (pos == std::wstring::npos) {  // File name only
			return path.substr(0, last + 1);
		}
		return path.substr(pos + 1, last - pos);
	}

	// Extract file name without extention
	std::wstring name_without_ext(const std::wstring& path) noexcept {
		auto ret = name(path);
		const auto pos = ret.find_last_of(EXT_PREFIX);
		if (pos != std::wstring::npos) ret.resize(pos);
		return ret;
	}

	// Extract file extention
	std::wstring ext(const std::wstring& path) noexcept {
		auto n = name(path);
		const auto pos = n.find_last_of(EXT_PREFIX);

		if (pos == std::wstring::npos) {  // No extention
			return L"";  // Return empty
		}
		auto ret = n.substr(pos + 1);
		std::transform(std::begin(ret), std::end(ret), std::begin(ret), ::towlower);  // To lower case
		return ret;
	}

	// Extract parent path
	std::wstring parent(const std::wstring& path) noexcept {
		const auto size = path.size();
		const auto pos = path.find_last_of(PATH_SEPARATOR);

		if (pos == std::wstring::npos) {  // Abnormal
			return L"";  // Return empty
		}
		if (1 < size && path.at(pos - 1) == DRIVE_IDENTIFIER) {  // When root 'C:\'
			if (pos == size - 1) {  // Pos is the tail end
				return L"";  // Return empty
			} else {  // Return containing '\'
				return path.substr(0, pos + 1);
			}
		}
		return path.substr(0, pos);
	}

	// Quote path
	std::wstring quote(const std::wstring& path) noexcept {
		return (L'\"' + path).append(1, L'\"');
	}

	// Ensure the path begin with UNC prefix "\\?\"
	std::wstring ensure_unc_prefix(const std::wstring& path) noexcept {
		if (0 == path.compare(0, 4, UNC_PREFIX)) {
			return path;
		}
		return UNC_PREFIX + path;
	}

	// Ensure the path does not begin with UNC prefix "\\?\"
	std::wstring ensure_no_unc_prefix(const std::wstring& path) noexcept {
		if (0 == path.compare(0, 4, UNC_PREFIX)) {
			return path.substr(4);
		}
		return path;
	}

	// Ensure the path begin with UNC prefix "\\?\" if the path is too long.
	std::wstring ensure_unc_prefix_if_needed(const std::wstring& path) noexcept {
		const auto p = ensure_no_unc_prefix(path);
		if (MAX_PATH - 1 < p.size()) {
			return ensure_unc_prefix(p);
		}
		return p;
	}

	// Translate relative path to absolute path
	std::wstring absolute_path(const std::wstring& path, const std::wstring& module_file_path) noexcept {
		if (path.at(0) == EXT_PREFIX && path.at(1) == PATH_SEPARATOR) {
			auto ret = parent(module_file_path);
			return ret.append(path, 1);
		}
		return std::wstring(path);
	}

	// Make quoted and space-separated path string
	std::wstring space_separated_quoted_paths_string(const std::vector<std::wstring>& paths) noexcept {
		std::wstring ret;
		for (const auto& path : paths) {
			ret.append(1, L'\"').append(ensure_unc_prefix_if_needed(path));
			if (path.back() == DRIVE_IDENTIFIER) {
				ret.append(1, PATH_SEPARATOR);
			}
			ret.append(L"\" ");
		}
		ret.pop_back();  // Remove last space
		return ret;
	}

	// Make NULL-separated and double-NULL-terminated path string
	std::wstring null_separated_paths_string(const std::vector<std::wstring>& paths) noexcept {
		std::wstring ret;
		for (const auto& path : paths) {
			ret.append(path);
			if (path.back() == DRIVE_IDENTIFIER) {
				ret.append(1, PATH_SEPARATOR);
			}
			ret.append(1, L'\0');
		}
		ret.append(1, L'\0');
		return ret;
	}

	// Check whether the path is root
	bool is_root(const std::wstring& path) noexcept {
		const auto size = path.size();
		return
			(1 < size && path.at(size - 2) == DRIVE_IDENTIFIER && path.at(size - 1) == PATH_SEPARATOR) ||
			(0 < size && path.at(size - 1) == DRIVE_IDENTIFIER);
	}

};
