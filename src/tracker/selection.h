/**
 *
 * File Operations
 *
 * @author Takuto Yanagida
 * @version 2021-05-09
 *
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

	static std::wstring Format(long long l) {
		std::wstring s;
		for (int d = 0; l; d++, l /= 10) {
			s = wchar_t(L'0' + (l % 10)) + (((!(d % 3) && d) ? L"," : L"") + s);
		}
		return s;
	}

	HWND hWnd_ = nullptr;
	std::vector<std::wstring> objects_;
	std::wstring defaultOpener_;
	ULONG idNotify_ = 0;

	const TypeTable& extentions_;
	const Pref& pref_;

	// Open file (specify target)
	bool OpenFile(const std::vector<std::wstring>& objs) {
		Operation so(hWnd_);
		const auto& obj = objs.front();
		std::wstring cmd;
		const auto& ext = FileSystem::is_directory(obj) ? PATH_EXT_DIR : Path::ext(obj);
		if (extentions_.get_command(pref_, ext, cmd)) {
			return so.open(objs, cmd);
		}
		// Normal file open behavior
		bool ret = so.open(obj);  // Try to open normally
		if (!ret && !FileSystem::is_directory(obj) && !defaultOpener_.empty()) {
			// In the case of a file without association
			ret = so.open(objs, defaultOpener_);
		}
		return ret;
	}

	// Register Shell Notifications
	void SetShellNotify(const std::wstring& path) {
		LPSHELLFOLDER desktopFolder{};
		LPITEMIDLIST currentFolder{};
		SHChangeNotifyEntry scne{};

		if (idNotify_) {
			::SHChangeNotifyDeregister(idNotify_);
			idNotify_ = 0;
		}
		HRESULT r;
		if (path.empty()) {
			r = ::SHGetSpecialFolderLocation(nullptr, CSIDL_DRIVES, &currentFolder);
		} else {
			if (::SHGetDesktopFolder(&desktopFolder) != NOERROR) return;
			if (!desktopFolder) return;
			std::vector<wchar_t> wideName{ path.begin(), path.end() };
			r = desktopFolder->ParseDisplayName(hWnd_, nullptr, static_cast<LPWSTR>(wideName.data()), nullptr, &currentFolder, nullptr);
			desktopFolder->Release();
		}
		if (r != S_OK) return;
		scne.pidl = currentFolder;
		scne.fRecursive = FALSE;
		idNotify_ = ::SHChangeNotifyRegister(hWnd_, 0x0001 | 0x0002 | 0x1000 | 0x8000,
			SHCNE_RENAMEITEM | SHCNE_CREATE | SHCNE_DELETE | SHCNE_MKDIR | SHCNE_RMDIR | SHCNE_UPDATEITEM | SHCNE_RENAMEFOLDER,
			WM_REQUESTUPDATE, 1, &scne);
		::CoTaskMemFree(currentFolder);
	}

	// Manually request an update
	void RequestUpdate() noexcept {
		::SendMessage(hWnd_, WM_REQUESTUPDATE, 0, 0);
	}

	// Determine file size
	bool FilesSize(uint64_t& size, const uint64_t limitTime) {
		uint64_t s;
		size = 0;

		for (const auto& e : objects_) {
			const bool success = FileSystem::calc_file_size(e, s, limitTime);
			size += s;
			if (!success) return false;
		}
		return true;
	}

	// Generate a string representing the size of the file or drive
	std::wstring FileSizeToStr(const uint64_t& size, bool success, const wchar_t* prefix) {
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
			fmt = L"%.3lf";
		}
		else if (val < 10) {
			fmt = L"%.2lf";
		}
		else if (val < 100) {
			fmt = L"%.1lf";
		}
		else {
			fmt = L"%.0lf";
		}
		wchar_t temp[100]{};
		swprintf_s(&temp[0], 100, fmt, val);

		std::wstring dest{ prefix };
		if (!success) dest.append(L">");
		dest.append(&temp[0]);
		dest.append(u);
		if (f != 0 && f != 3) dest.append(L" (").append(Format(size)).append(L" Bytes)");
		return dest;
	}

	// Generate a string representing the file's timestamp
	std::wstring& FileTimeToStr(const FILETIME& time, const wchar_t* prefix, std::wstring& dest) {
		FILETIME local{};
		SYSTEMTIME st{};

		::FileTimeToLocalFileTime(&time, &local);
		::FileTimeToSystemTime(&local, &st);

		wchar_t temp[100]{};  // pre + 19 characters
		swprintf_s(&temp[0], 100, L"%s%u-%02u-%02u (%02u:%02u:%02u)", prefix, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
		dest.assign(&temp[0]);
		return dest;
	}

public:

	Selection(const TypeTable& exts, const Pref& pref) noexcept : extentions_(exts), pref_(pref) {}

	Selection(const Selection& inst) = delete;
	Selection(Selection&& inst) = delete;
	Selection& operator=(const Selection& inst) = delete;
	Selection& operator=(Selection&& inst) = delete;

	~Selection() {
		if (idNotify_) ::SHChangeNotifyDeregister(idNotify_);
	}

	// Specify a window handle.
	void SetWindowHandle(HWND hWnd) noexcept {
		hWnd_ = hWnd;
	}

	// Set default application path to open file without association
	void SetDefaultOpener(const std::wstring& path) {
		defaultOpener_ = path;
	}

	// Add operation target file
	void Add(const std::wstring& path) {
		objects_.push_back(path);
	}

	// Clear operation target file
	void Clear() noexcept {
		objects_.clear();
	}

	// Reference of specified index element
	const std::wstring& operator[](unsigned long i) const noexcept(false) {
		return objects_.at(i);
	}

	// Size
	unsigned long Count() const noexcept {
		return objects_.size();
	}

	// Open file based on association
	bool OpenWithAssociation() {
		return OpenFile(objects_);
	}

	// Resolve the shortcut and then open
	bool OpenAfterResolve() {
		if (!Link::is_link(objects_.at(0))) return false;  // If it is not a shortcut

		// OpenBy processing
		auto path = Link::resolve(objects_.at(0));
		return OpenFile({ path });
	}

	// Open in shell function based on command line
	int OpenBy(const std::wstring& line) {
		Operation so(hWnd_);
		return so.open(objects_, line);
	}

	// Start dragging
	void StartDrag() {
		DragFile::start(objects_);
	}

	// Display shell menu
	void PopupShellMenu(const POINT& pt, UINT f) {
		SetShellNotify(Path::parent(objects_.at(0)));
		ContextMenu cm(hWnd_);
		cm.popup(objects_, TPM_RIGHTBUTTON | f, pt);
	}

	// Create New
	bool CreateNewFile(const wchar_t* org) {
		if (!FileSystem::is_directory(objects_.at(0))) return false;  // Fail if not folder

		std::wstring orig{ org };
		std::wstring npath{ objects_.at(0) };
		auto fname = Path::name(orig);
		npath.append(1, L'\\').append(fname);
		auto newPath = FileSystem::unique_name(npath);

		auto new_fname = Path::name(newPath);
		Operation so(hWnd_);
		const bool ret = so.copy_one_file(orig, objects_.at(0), new_fname);

		if (ret) RequestUpdate();
		return ret;
	}

	// Create a new folder
	bool CreateNewFolderIn() {
		if (!FileSystem::is_directory(objects_.at(0))) return false;  // Fail if not folder
		auto npath = objects_.at(0) + L"\\NewFolder";
		auto newPath = FileSystem::unique_name(npath);

		const BOOL ret = ::CreateDirectory(newPath.c_str(), nullptr);
		if (ret) RequestUpdate();
		return ret == TRUE;
	}

	// Delete
	bool DeleteFile() {
		Operation so(hWnd_);
		const bool ret = so.delete_files(objects_);
		if (ret) RequestUpdate();
		return ret;
	}

	// Make a duplicate
	bool CloneHere() {
		bool ret = false;
		Operation so(hWnd_);

		for (const auto& o : objects_) {
			auto clonePath = FileSystem::unique_name(o, L"_Clone");
			if (clonePath.empty()) return false;
			auto new_fname = Path::name(clonePath);
			auto dest_path = Path::parent(o);
			if (so.copy_one_file(o, dest_path, new_fname)) ret = true;
		}
		if (ret) RequestUpdate();
		return ret;
	}

	// Make a shortcut
	bool CreateShortcutHere() {
		bool ret = false;
		std::wstring path, target;

		for (const auto &obj : objects_) {
			if (Link::is_link(obj)) {  // When it is a link
				target = Link::resolve(obj);
				path.assign(obj);
			} else {
				target.assign(obj);
				path = obj + L".lnk";
			}
			if (Link::create(FileSystem::unique_name(path), target)) ret = true;
		}
		if (ret) RequestUpdate();
		return ret;
	}

	// Copy to desktop
	bool CopyToDesktop() {
		Operation so(hWnd_);
		bool ret = so.copy_files(objects_, FileSystem::desktop_path());
		if (ret) RequestUpdate();
		return ret;
	}

	// Move to desktop
	bool MoveToDesktop() {
		Operation so(hWnd_);
		bool ret = so.move_files(objects_, FileSystem::desktop_path());
		if (ret) RequestUpdate();
		return ret;
	}

	// Copy path to clipboard
	bool CopyPathInClipboard() {
		Clipboard cb(hWnd_);
		return cb.copy_path(objects_);
	}

	void Copy() {
		ContextMenu cm(hWnd_);
		cm.copy(objects_);
	}

	void Cut() {
		ContextMenu cm(hWnd_);
		cm.cut(objects_);
	}

	void PasteIn() {
		ContextMenu cm(hWnd_);
		SetShellNotify(objects_.at(0));
		cm.paste_in(objects_);
	}

	// Paste as a shortcut
	bool PasteAsShortcutIn() {
		Clipboard cb(hWnd_);
		const bool ret = cb.paste_as_link_in(objects_.at(0));
		if (ret) RequestUpdate();
		return ret;
	}

	// Display file properties
	void PopupFileProperty() {
		ContextMenu cm(hWnd_);
		cm.show_property(objects_);
	}

	// Change the file name
	bool Rename(const std::wstring& path, const std::wstring& newFileName) {
		Operation so(hWnd_);
		return so.rename(path, newFileName);
	}

	// Get file information string
	void InformationStrings(std::vector<std::wstring>& items) {
		if (Path::is_root(objects_.at(0))) {  // When it is a drive
			uint64_t dSize, dFree;
			FileSystem::drive_size(objects_.at(0), dSize, dFree);
			auto sizeStr = FileSizeToStr(dSize, true, L"");
			auto usedStr = FileSizeToStr(dSize - dFree, true, L"");
			items.push_back(usedStr.append(1, L'/').append(sizeStr));
			items.push_back(FileSizeToStr(dFree, true, L"Free: "));
		} else {  // When it is a normal file
			std::wstring ctStr, mtStr;
			uint64_t size;
			bool const suc = FilesSize(size, ::GetTickCount64() + 1000);
			items.push_back(FileSizeToStr(size, suc, L"Size:\t"));

			// Get date
			HANDLE hf = ::CreateFile(objects_.at(0).c_str(), 0, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS, nullptr);
			if (hf != INVALID_HANDLE_VALUE) {
				FILETIME ctime, mtime;
				::GetFileTime(hf, &ctime, nullptr, &mtime);
				::CloseHandle(hf);
				items.push_back(FileTimeToStr(ctime, L"Created:\t", ctStr));
				items.push_back(FileTimeToStr(mtime, L"Modified:\t", mtStr));
			}
		}
	}

	// Perform processing after update notification
	void DoneRequest() noexcept {
		if (idNotify_) {
			::SHChangeNotifyDeregister(idNotify_);
			idNotify_ = 0;
		}
	}

	// Execute a string command
	int Command(const std::wstring& cmd) {
		if (cmd.find(COM_CREATE_NEW) == 0) { CreateNewFile(cmd.substr(11).c_str()); return 1; }
		if (cmd == COM_NEW_FOLDER)         { CreateNewFolderIn(); return 1; }
		if (cmd == COM_DELETE)             { DeleteFile(); return 1; }
		if (cmd == COM_CLONE)              { CloneHere(); return 1; }
		if (cmd == COM_SHORTCUT)           { CreateShortcutHere(); return 1; }
		if (cmd == COM_COPY_TO_DESKTOP)    { CopyToDesktop(); return 1; }
		if (cmd == COM_MOVE_TO_DESKTOP)    { MoveToDesktop(); return 1; }
		if (cmd == COM_COPY_PATH)          { CopyPathInClipboard(); return 1; }
		if (cmd == COM_COPY)               { Copy(); return 1; }
		if (cmd == COM_CUT)                { Cut(); return 1; }
		if (cmd == COM_PASTE)              { PasteIn(); return 1; }
		if (cmd == COM_PASTE_SHORTCUT)     { PasteAsShortcutIn(); return 1; }
		if (cmd == COM_PROPERTY)           { PopupFileProperty(); return 1; }
		if (cmd == COM_OPEN)               { return OpenWithAssociation() ? -1 : 0; }
		if (cmd == COM_OPEN_RESOLVE)       { return OpenAfterResolve() ? -1 : 0; }
		return 0;
	}

};
