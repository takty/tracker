/**
 *
 * File Operations
 *
 * @author Takuto Yanagida
 * @version 2021-05-30
 *
 */


#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include <format>
#include <locale>

#include "tracker.h"
#include "file_utils.hpp"
#include "type_table.h"
#include "pref.hpp"


class Selection {

	static std::wstring format(long long l) noexcept {
		std::wstring s;
		for (int d = 0; l; d++, l /= 10) {
			s = wchar_t(L'0' + (l % 10)) + (((!(d % 3) && d) ? L"," : L"") + s);
		}
		return s;
	}

	HWND hwnd_ = nullptr;
	std::vector<std::wstring> objects_;
	std::wstring default_opener_;
	ULONG id_notification_ = 0;

	const TypeTable& extentions_;
	const Pref& pref_;

	// Open file (specify target)
	bool open_file(const std::vector<std::wstring>& objs) noexcept(false) {
		Operation so(hwnd_);
		const auto& obj = objs.front();
		std::wstring cmd;
		const auto& ext = FileSystem::is_directory(obj) ? PATH_EXT_DIR : Path::ext(obj);
		if (extentions_.get_command(pref_, ext, cmd)) {
			return so.open(objs, cmd);
		}
		// Normal file open behavior
		bool ret = so.open(obj);  // Try to open normally
		if (!ret && !FileSystem::is_directory(obj) && !default_opener_.empty()) {
			// In the case of a file without association
			ret = so.open(objs, default_opener_);
		}
		return ret;
	}

	// Register Shell Notifications
	void set_shell_notification(const std::wstring& path) noexcept(false) {
		LPSHELLFOLDER desktop_folder{};
		LPITEMIDLIST current_folder{};
		SHChangeNotifyEntry scne{};

		if (id_notification_) {
			::SHChangeNotifyDeregister(id_notification_);
			id_notification_ = 0;
		}
		HRESULT r{};
		if (path.empty()) {
			r = ::SHGetSpecialFolderLocation(nullptr, CSIDL_DRIVES, &current_folder);
		} else {
			if (::SHGetDesktopFolder(&desktop_folder) != NOERROR) return;
			if (!desktop_folder) return;
			std::vector<wchar_t> wideName{ path.begin(), path.end() };
			r = desktop_folder->ParseDisplayName(hwnd_, nullptr, wideName.data(), nullptr, &current_folder, nullptr);
			desktop_folder->Release();
		}
		if (r != S_OK) return;
		scne.pidl = current_folder;
		scne.fRecursive = FALSE;
		id_notification_ = ::SHChangeNotifyRegister(hwnd_, 0x0001 | 0x0002 | 0x1000 | 0x8000,
			SHCNE_RENAMEITEM | SHCNE_CREATE | SHCNE_DELETE | SHCNE_MKDIR | SHCNE_RMDIR | SHCNE_UPDATEITEM | SHCNE_RENAMEFOLDER,
			WM_REQUESTUPDATE, 1, &scne);
		::CoTaskMemFree(current_folder);
	}

	// Manually request an update
	void request_update() noexcept {
		::SendMessage(hwnd_, WM_REQUESTUPDATE, 0, 0);
	}

	// Determine file size
	bool files_size(uint64_t& size, const uint64_t limit_time) noexcept {
		uint64_t s{};
		size = 0;

		for (const auto& e : objects_) {
			const bool success = FileSystem::calc_file_size(e, s, limit_time);
			size += s;
			if (!success) return false;
		}
		return true;
	}

	// Generate a string representing the size of the file or drive
	std::wstring file_size_to_string(const uint64_t& size, bool success, const wchar_t* prefix) noexcept(false) {
		int f{};
		const wchar_t* u{};
		double val{};

		if (size >= (1 << 30)) {
			f = 3;
			u = L" GB";
			val = double(size) / (1ULL << 30);
		}
		else if (size >= (1 << 20)) {
			f = 2;
			u = L" MB";
			val = double(size) / (1ULL << 20);
		}
		else if (size >= (1 << 10)) {
			f = 1;
			u = L" kB";
			val = double(size) / (1ULL << 10);
		}
		else {
			u = L" Bytes";
			val = double(size) / 1ULL;
		}
		const wchar_t* fmt{};
		if (val < 1) {
			fmt = L"{:.3f}";
		}
		else if (val < 10) {
			fmt = L"{:.2f}";
		}
		else if (val < 100) {
			fmt = L"{:.1f}";
		}
		else {
			fmt = L"{:.0f}";
		}
		std::wstring dest{ prefix };
		if (!success) dest.append(L">");
		dest.append(std::format(fmt, val)).append(u);
		if (f == 1 || f == 2) dest.append(L" (").append(format(size)).append(L" Bytes)");
		return dest;
	}

	// Generate a string representing the file's timestamp
	std::wstring file_date_to_string(const FILETIME& time, const wchar_t* prefix) const noexcept(false) {
		FILETIME local{};
		SYSTEMTIME st{};

		::FileTimeToLocalFileTime(&time, &local);
		::FileTimeToSystemTime(&local, &st);

		return std::format(L"{:s}{:d}-{:02d}-{:02d} ({:02d}:{:02d}:{:02d})", prefix, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	}


	// ------------------------------------------------------------------------


	// Create New
	bool create_new(const wchar_t* org) noexcept(false) {
		if (!FileSystem::is_directory(objects_.at(0))) return false;  // Fail if not folder

		std::wstring orig{ org };
		std::wstring npath{ objects_.at(0) };
		auto fname = Path::name(orig);
		npath.append(1, L'\\').append(fname);
		auto newPath = FileSystem::unique_name(npath);

		auto new_fname = Path::name(newPath);
		Operation so(hwnd_);
		const bool ret = so.copy_one_file(orig, objects_.at(0), new_fname);

		if (ret) request_update();
		return ret;
	}

	// Create a new folder
	bool create_new_folder_in() noexcept(false) {
		if (!FileSystem::is_directory(objects_.at(0))) return false;  // Fail if not folder
		auto npath = objects_.at(0) + L"\\NewFolder";
		auto newPath = FileSystem::unique_name(npath);

		const bool ret = FileSystem::create_directory(newPath);
		if (ret) request_update();
		return ret;
	}

	// Delete
	bool delete_file() noexcept(false) {
		Operation so(hwnd_);
		const bool ret = so.delete_files(objects_);
		if (ret) request_update();
		return ret;
	}

	// Make a duplicate
	bool clone_here() noexcept(false) {
		bool ret = false;
		Operation so(hwnd_);

		for (const auto& o : objects_) {
			auto clonePath = FileSystem::unique_name(o, L"_Clone");
			if (clonePath.empty()) return false;
			auto new_fname = Path::name(clonePath);
			auto dest_path = Path::parent(o);
			if (so.copy_one_file(o, dest_path, new_fname)) ret = true;
		}
		if (ret) request_update();
		return ret;
	}

	// Make a shortcut
	bool create_shortcut_here() noexcept(false) {
		bool ret = false;
		std::wstring path, target;

		for (const auto& obj : objects_) {
			if (Link::is_link(obj)) {  // When it is a link
				target = Link::resolve(obj);
				path.assign(obj);
			}
			else {
				target.assign(obj);
				path = obj + L".lnk";
			}
			if (Link::create(FileSystem::unique_name(path), target)) ret = true;
		}
		if (ret) request_update();
		return ret;
	}

	// Copy to desktop
	bool copy_to_desktop() noexcept(false) {
		Operation so(hwnd_);
		bool ret = so.copy_files(objects_, FileSystem::desktop_path());
		if (ret) request_update();
		return ret;
	}

	// Move to desktop
	bool move_to_desktop() noexcept(false) {
		Operation so(hwnd_);
		bool ret = so.move_files(objects_, FileSystem::desktop_path());
		if (ret) request_update();
		return ret;
	}

	// Copy path to clipboard
	bool copy_path_in_clipboard() noexcept(false) {
		Clipboard cb(hwnd_);
		return cb.copy_path(objects_);
	}

	void copy() noexcept(false) {
		ContextMenu cm(hwnd_);
		cm.copy(objects_);
	}

	void cut() noexcept(false) {
		ContextMenu cm(hwnd_);
		cm.cut(objects_);
	}

	void paste_in() noexcept(false) {
		ContextMenu cm(hwnd_);
		set_shell_notification(objects_.at(0));
		cm.paste_in(objects_);
	}

	// Paste as a shortcut
	bool paste_as_shortcut_in() noexcept(false) {
		Clipboard cb(hwnd_);
		const bool ret = cb.paste_as_link_in(objects_.at(0));
		if (ret) request_update();
		return ret;
	}

public:

	Selection(const TypeTable& exts, const Pref& pref) noexcept : extentions_(exts), pref_(pref) {}

	Selection(const Selection& inst)            = delete;
	Selection(Selection&& inst)                 = delete;
	Selection& operator=(const Selection& inst) = delete;
	Selection& operator=(Selection&& inst)      = delete;

	~Selection() noexcept {
		if (id_notification_) ::SHChangeNotifyDeregister(id_notification_);
	}

	// Specify a window handle.
	void set_window_handle(HWND hwnd) noexcept {
		hwnd_ = hwnd;
	}

	// Set default application path to open file without association
	void set_default_opener(const std::wstring& path) noexcept {
		default_opener_ = path;
	}


	// ------------------------------------------------------------------------


	// Add operation target file
	void add(const std::wstring& path) noexcept {
		objects_.push_back(path);
	}

	// Clear operation target file
	void clear() noexcept {
		objects_.clear();
	}

	// Reference of specified index element
	const std::wstring& operator[](unsigned long i) const noexcept {
		return objects_.at(i);
	}

	// Size
	unsigned long size() const noexcept {
		return objects_.size();
	}


	// ------------------------------------------------------------------------


	// Open file based on association
	bool open_by_association() noexcept(false) {
		return open_file(objects_);
	}

	// Resolve the shortcut and then open
	bool open_after_resolve() noexcept(false) {
		if (!Link::is_link(objects_.at(0))) return false;  // If it is not a shortcut

		// OpenBy processing
		auto path = Link::resolve(objects_.at(0));
		return open_file({ path });
	}

	// Open in shell function based on command line
	int open_with(const std::wstring& line) noexcept {
		Operation so(hwnd_);
		return so.open(objects_, line);
	}


	// ------------------------------------------------------------------------


	// Start dragging
	void start_drag() noexcept(false) {
		DragFile::start(objects_);
	}

	// Display shell menu
	void popup_shell_menu(const POINT& pt, UINT f) noexcept(false) {
		set_shell_notification(Path::parent(objects_.at(0)));
		ContextMenu cm(hwnd_);
		cm.popup(objects_, TPM_RIGHTBUTTON | f, pt);
	}

	// Display file properties
	void popup_file_property() noexcept(false) {
		ContextMenu cm(hwnd_);
		cm.show_property(objects_);
	}

	// Change the file name
	bool rename(const std::wstring& path, const std::wstring& newFileName) noexcept(false) {
		Operation so(hwnd_);
		return so.rename(path, newFileName);
	}

	// Get file information string
	void make_info_strings(std::vector<std::wstring>& items) noexcept(false) {
		if (Path::is_root(objects_.at(0))) {  // When it is a drive
			uint64_t dSize, dFree;
			FileSystem::drive_size(objects_.at(0), dSize, dFree);
			auto sizeStr = file_size_to_string(dSize, true, L"");
			auto usedStr = file_size_to_string(dSize - dFree, true, L"");
			items.push_back(usedStr.append(1, L'/').append(sizeStr));
			items.push_back(file_size_to_string(dFree, true, L"Free: "));
		} else {  // When it is a normal file
			uint64_t size;
			bool const suc = files_size(size, ::GetTickCount64() + 1000);
			items.push_back(file_size_to_string(size, suc, L"Size:\t"));

			// Get date
			HANDLE hf = ::CreateFile(objects_.at(0).c_str(), 0, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS, nullptr);
			if (hf != INVALID_HANDLE_VALUE) {
				FILETIME ctime, mtime;
				::GetFileTime(hf, &ctime, nullptr, &mtime);
				::CloseHandle(hf);
				items.push_back(file_date_to_string(ctime, L"Created:\t"));
				items.push_back(file_date_to_string(mtime, L"Modified:\t"));
			}
		}
	}

	// Perform processing after update notification
	void done_request() noexcept {
		if (id_notification_) {
			::SHChangeNotifyDeregister(id_notification_);
			id_notification_ = 0;
		}
	}

	// Execute a string command
	int command(const std::wstring& cmd) noexcept(false) {
		if (cmd.find(COM_CREATE_NEW) == 0) { create_new(cmd.substr(11).c_str()); return 1; }
		if (cmd == COM_NEW_FOLDER)         { create_new_folder_in(); return 1; }
		if (cmd == COM_DELETE)             { delete_file(); return 1; }
		if (cmd == COM_CLONE)              { clone_here(); return 1; }
		if (cmd == COM_SHORTCUT)           { create_shortcut_here(); return 1; }
		if (cmd == COM_COPY_TO_DESKTOP)    { copy_to_desktop(); return 1; }
		if (cmd == COM_MOVE_TO_DESKTOP)    { move_to_desktop(); return 1; }
		if (cmd == COM_COPY_PATH)          { copy_path_in_clipboard(); return 1; }
		if (cmd == COM_COPY)               { copy(); return 1; }
		if (cmd == COM_CUT)                { cut(); return 1; }
		if (cmd == COM_PASTE)              { paste_in(); return 1; }
		if (cmd == COM_PASTE_SHORTCUT)     { paste_as_shortcut_in(); return 1; }
		if (cmd == COM_PROPERTY)           { popup_file_property(); return 1; }
		if (cmd == COM_OPEN)               { return open_by_association() ? -1 : 0; }
		if (cmd == COM_OPEN_RESOLVE)       { return open_after_resolve() ? -1 : 0; }
		return 0;
	}

};
