/**
 * File Operations
 *
 * @author Takuto Yanagida
 * @version 2025-11-18
 */

#pragma once

#include <windows.h>
#include <vector>
#include <string>

#include "tracker.h"
#include "file_utils.hpp"
#include "type_table.h"
#include "pref.hpp"

class Selection {

	HWND hwnd_ = nullptr;
	std::vector<std::wstring> objects_;
	std::wstring default_opener_;
	unsigned long id_notify_{};

	const TypeTable& exts_;
	const Pref& pref_;

	// Open file (specify target)
	bool open_file(const std::vector<std::wstring>& objs) {
		const auto& obj = objs.front();

		std::wstring cmd;
		auto ext = file_system::is_directory(obj) ? PATH_EXT_DIR : path::ext(obj);
		bool ret = false;

		if (exts_.get_command(pref_, ext, cmd)) {
			ret = execute::open(hwnd_, objs, cmd);
		} else {
			// Normal file open behavior
			ret = execute::open(hwnd_, obj);  // Try to open normally
			if (!ret && !file_system::is_directory(obj) && !default_opener_.empty()) {
				// In the case of a file without association
				ret = execute::open(hwnd_, objs, default_opener_);
			}
		}
		return ret;
	}

	// Manually request an update
	void request_update() const noexcept {
		::SendMessage(hwnd_, WM_REQUESTUPDATE, 0, 0);
	}

public:

	Selection(const TypeTable& exts, const Pref& pref) noexcept : exts_(exts), pref_(pref) {}
	Selection(const Selection&) = delete;
	Selection& operator=(const Selection&) = delete;
	Selection(Selection&&) = delete;
	Selection& operator=(Selection&&) = delete;

	~Selection() {
		shell::clear_shell_notify(id_notify_);
	}

	// Specify a window handle.
	void set_window_handle(HWND hwnd) noexcept {
		hwnd_ = hwnd;
	}

	// Set default application path to open file without association
	void set_default_opener(const std::wstring& path) {
		default_opener_ = path;
	}

	// Add operation target file
	void add(const std::wstring& path) {
		objects_.push_back(path);
	}

	// Clear operation target file
	void clear() noexcept {
		objects_.clear();
	}

	// Reference of specified index element
	const std::wstring& operator[](size_t i) const {
		return objects_.at(i);
	}

	// Size
	size_t size() const noexcept {
		return objects_.size();
	}

	// Open file based on association
	bool open_with_association() {
		return open_file(objects_);
	}

	// Resolve the shortcut and then open
	bool open_after_resolve() {
		if (!link::is_link(objects_.front())) return false;  // If it is not a shortcut

		// OpenBy processing
		auto path = link::resolve(objects_.front());
		return open_file({ path });
	}

	// Open in shell function based on command line
	int open_by(const std::wstring& line) {
		const bool ret = execute::open(hwnd_, objects_, line);
		return ret;
	}

	// Start dragging
	void start_drag() const {
		file_drag::start(objects_);
	}

	// Display shell menu
	void popup_shell_menu(const POINT& pt, UINT f) {
		id_notify_ = shell::set_shell_notify(id_notify_, hwnd_, WM_REQUESTUPDATE, path::parent(objects_.front()));
		ContextMenu cm(hwnd_);
		cm.popup(objects_, TPM_RIGHTBUTTON | f, pt);
	}

	// Create New
	bool create_new_file(const std::wstring& orig) {
		if (!file_system::is_directory(objects_.front())) return false;  // Fail if not folder

		auto fname = path::name(orig);
		std::wstring npath{ objects_.front() };
		npath.append(1, L'\\').append(fname);
		auto new_path  = file_system::unique_name(npath);
		auto new_fname = path::name(new_path);

		const bool ret = operation::copy_one_file(orig, objects_.front(), new_fname);
		if (ret) {
			request_update();
		}
		return ret;
	}

	// Create a new folder
	bool create_new_folder_in() {
		if (!file_system::is_directory(objects_.front())) {
			return false;  // Fail if not folder
		}
		auto npath    = objects_.front() + L"\\NewFolder";
		auto new_path = file_system::unique_name(npath);

		const bool ret = ::CreateDirectory(new_path.c_str(), nullptr) == TRUE;
		if (ret) {
			request_update();
		}
		return ret;
	}

	// Delete
	bool delete_file() {
		const bool ret = operation::delete_files(objects_);
		if (ret) {
			request_update();
		}
		return ret;
	}

	// Make a duplicate
	bool clone_here() {
		bool ret = false;

		for (const auto& o : objects_) {
			auto clone_path = file_system::unique_name(o, L"_Clone");
			if (clone_path.empty()) {
				ret = false;
				break;
			}
			auto new_fname = path::name(clone_path);
			auto dest_path = path::parent(o);
			if (operation::copy_one_file(o, dest_path, new_fname)) {
				ret = true;
			}
		}
		if (ret) {
			request_update();
		}
		return ret;
	}

	// Make a shortcut
	bool create_shortcut_here() {
		bool ret = false;
		std::wstring path, target;

		for (auto& obj : objects_) {
			if (link::is_link(obj)) {  // When it is a link
				target = link::resolve(obj);
				path.assign(obj);
			} else {
				target.assign(obj);
				path = obj + L".lnk";
			}
			if (link::create(file_system::unique_name(path), target)) {
				ret = true;
			}
		}
		if (ret) {
			request_update();
		}
		return ret;
	}

	// Copy to desktop
	bool copy_to_desktop() {
		const bool ret = operation::copy_files(objects_, file_system::desktop_path());
		if (ret) {
			request_update();
		}
		return ret;
	}

	// Move to desktop
	bool move_to_desktop() {
		const bool ret = operation::move_files(objects_, file_system::desktop_path());
		if (ret) {
			request_update();
		}
		return ret;
	}

	// Copy path to clipboard
	bool copy_path_in_clipboard() {
		return clipboard::copy_path(objects_);
	}

	void copy() const {
		shell::copy(objects_);
	}

	void cut() const {
		shell::cut(objects_);
	}

	void paste_in() {
		id_notify_ = shell::set_shell_notify(id_notify_, hwnd_, WM_REQUESTUPDATE, objects_.front());
		shell::paste_in(objects_);
	}

	// Paste as a shortcut
	bool paste_as_shortcut_in() {
		const bool ret = clipboard::paste_as_link_in(objects_.front());
		if (ret) {
			request_update();
		}
		return ret;
	}

	// Display file properties
	void popup_file_property() const {
		shell::show_property(objects_);
	}

	// Change the file name
	bool rename(const std::wstring& path, const std::wstring& new_file_name) {
		return operation::rename(path, new_file_name);
	}

	// Get file information string
	void create_information_strings(std::vector<std::wstring>& items) {
		if (path::is_root(objects_.front())) {  // When it is a drive
			uint64_t dsize, dfree;
			file_system::drive_size(objects_.front(), dsize, dfree);
			auto size_str = info::file_size_to_str(dsize, true, L"");
			auto used_str = info::file_size_to_str(dsize - dfree, true, L"");
			items.push_back(used_str.append(1, L'/').append(size_str));
			items.push_back(info::file_size_to_str(dfree, true, L"Free: "));
		} else {  // When it is a normal file
			std::wstring ct_str, mt_str;
			uint64_t size;
			const bool suc = file_system::calc_file_size(objects_, size, ::GetTickCount64() + 1000);
			items.push_back(info::file_size_to_str(size, suc, L"Size:\t"));

			// Get date
			HANDLE hf = ::CreateFile(objects_.front().c_str(), 0, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS, nullptr);
			if (hf != INVALID_HANDLE_VALUE) {
				FILETIME ctime, mtime;
				::GetFileTime(hf, &ctime, nullptr, &mtime);
				::CloseHandle(hf);
				items.push_back(info::file_time_to_str(ctime, L"Created:\t", ct_str));
				items.push_back(info::file_time_to_str(mtime, L"Modified:\t", mt_str));
			}
		}
	}

	// Perform processing after update notification
	void done_request() noexcept {
		id_notify_ = shell::clear_shell_notify(id_notify_);
	}

	// Execute a string command
	int command(const std::wstring& cmd) {
		if (cmd.find(CMD_CREATE_NEW) == 0) { create_new_file(cmd.substr(CMD_CREATE_NEW.size())); return 1; }
		if (cmd == CMD_NEW_FOLDER)         { create_new_folder_in(); return 1; }
		if (cmd == CMD_DELETE)             { delete_file(); return 1; }
		if (cmd == CMD_CLONE)              { clone_here(); return 1; }
		if (cmd == CMD_SHORTCUT)           { create_shortcut_here(); return 1; }
		if (cmd == CMD_COPY_TO_DESKTOP)    { copy_to_desktop(); return 1; }
		if (cmd == CMD_MOVE_TO_DESKTOP)    { move_to_desktop(); return 1; }
		if (cmd == CMD_COPY_PATH)          { copy_path_in_clipboard(); return 1; }
		if (cmd == CMD_COPY)               { copy(); return 1; }
		if (cmd == CMD_CUT)                { cut(); return 1; }
		if (cmd == CMD_PASTE)              { paste_in(); return 1; }
		if (cmd == CMD_PASTE_SHORTCUT)     { paste_as_shortcut_in(); return 1; }
		if (cmd == CMD_PROPERTY)           { popup_file_property(); return 1; }
		if (cmd == CMD_OPEN)               { return open_with_association() ? -1 : 0; }
		if (cmd == CMD_OPEN_RESOLVE)       { return open_after_resolve() ? -1 : 0; }
		return 0;
	}

};
