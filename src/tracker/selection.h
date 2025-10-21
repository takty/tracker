/**
 *
 * File Operations
 *
 * @author Takuto Yanagida
 * @version 2025-10-21
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

	const TypeTable& extensions_;
	const Pref& pref_;

	// Open file (specify target)
	bool OpenFile(const std::vector<std::wstring>& objs) {
		Operation so(hWnd_);
		const auto& obj = objs.front();
		std::wstring cmd;
		auto ext = FileSystem::is_directory(obj) ? PATH_EXT_DIR : Path::ext(obj);
		if (extensions_.get_command(pref_, ext, cmd)) {
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
		HRESULT r{};
		if (path.empty()) {
			r = ::SHGetSpecialFolderLocation(nullptr, CSIDL_DRIVES, &currentFolder);
		} else {
			if (::SHGetDesktopFolder(&desktopFolder) != NOERROR) return;
			wchar_t* wideName = (wchar_t*)path.c_str();
			r = desktopFolder->ParseDisplayName(hWnd_, nullptr, wideName, nullptr, &currentFolder, nullptr);
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
	void RequestUpdate() const noexcept {
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
		static const wchar_t *u[] = { L" Bytes", L" kB", L" MB", L" GB" };
		static const int s[] = { 0, 10, 20, 30 };

		std::wstring dest;
		int f = 0;
		if (size >= (1 << 30)) f = 3;
		else if (size >= (1 << 20)) f = 2;
		else if (size >= (1 << 10)) f = 1;
		const double val = double(size) / (1ULL << s[f]);

		int pre = 0;
		if (val < 100) ++pre;
		if (val < 10) ++pre;
		if (val < 1) ++pre;

		wchar_t format[100]{}, temp[100]{};
		swprintf_s(&format[0], 100, L"%s%s%%.%dlf%s", prefix, (!success ? L">" : L""), pre, u[f]);
		swprintf_s(&temp[0], 100, &format[0], val);
		dest.assign(&temp[0]);
		if (f != 0 && f != 3) dest.append(L" (").append(Format(size)).append(L" Bytes)");
		return dest;
	}

	// Generate a string representing the file's timestamp
	std::wstring& FileTimeToStr(const FILETIME& time, const wchar_t* prefix, std::wstring& dest) {
		FILETIME local;
		SYSTEMTIME st;

		::FileTimeToLocalFileTime(&time, &local);
		::FileTimeToSystemTime(&local, &st);

		wchar_t temp[100]{};  // pre + 19 characters
		swprintf_s(&temp[0], 100, L"%s%d/%02d/%02d (%02d:%02d:%02d)", prefix, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
		dest.assign(&temp[0]);
		return dest;
	}

public:

	Selection(const TypeTable& exts, const Pref& pref) noexcept : extensions_(exts), pref_(pref) {}

	Selection(const Selection&) = delete;
	Selection& operator=(const Selection&) = delete;
	Selection(Selection&&) = delete;
	Selection& operator=(Selection&&) = delete;

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
	const std::wstring& operator[](size_t i) const noexcept {
		return objects_[i];
	}

	// Size
	size_t Count() const noexcept {
		return objects_.size();
	}

	// Open file based on association
	bool OpenWithAssociation() {
		return OpenFile(objects_);
	}

	// Resolve the shortcut and then open
	bool OpenAfterResolve() {
		if (!Link::is_link(objects_[0])) return false;  // If it is not a shortcut

		// OpenBy processing
		auto path = Link::resolve(objects_[0]);
		return OpenFile({ path });
	}

	// Open in shell function based on command line
	int OpenBy(const std::wstring& line) {
		Operation so(hWnd_);
		return so.open(objects_, line);
	}

	// Start dragging
	void StartDrag() const {
		DragFile::start(objects_);
	}

	// Display shell menu
	void PopupShellMenu(const POINT& pt, UINT f) {
		SetShellNotify(Path::parent(objects_[0]));
		ContextMenu cm(hWnd_);
		cm.popup(objects_, TPM_RIGHTBUTTON | f, pt);
	}

	// Create New
	bool CreateNewFile(const wchar_t* org) {
		if (!FileSystem::is_directory(objects_[0])) return false;  // Fail if not folder

		std::wstring orig{ org };
		std::wstring npath{ objects_[0] };
		auto fname = Path::name(orig);
		npath.append(1, L'\\').append(fname);
		auto newPath = FileSystem::unique_name(npath);

		auto new_fname = Path::name(newPath);
		Operation so(hWnd_);
		const bool ret = so.copy_one_file(orig, objects_[0], new_fname);

		if (ret) RequestUpdate();
		return ret;
	}

	// Create a new folder
	bool CreateNewFolderIn() {
		if (!FileSystem::is_directory(objects_[0])) return false;  // Fail if not folder
		auto npath = objects_[0] + L"\\NewFolder";
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

		for (auto& obj : objects_) {
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
		const bool ret = so.copy_files(objects_, FileSystem::desktop_path());
		if (ret) RequestUpdate();
		return ret;
	}

	// Move to desktop
	bool MoveToDesktop() {
		Operation so(hWnd_);
		const bool ret = so.move_files(objects_, FileSystem::desktop_path());
		if (ret) RequestUpdate();
		return ret;
	}

	// Copy path to clipboard
	bool CopyPathInClipboard() {
		const Clipboard cb(hWnd_);
		return cb.copy_path(objects_);
	}

	void Copy() {
		const ContextMenu cm(hWnd_);
		cm.copy(objects_);
	}

	void Cut() {
		const ContextMenu cm(hWnd_);
		cm.cut(objects_);
	}

	void PasteIn() {
		const ContextMenu cm(hWnd_);
		SetShellNotify(objects_[0]);
		cm.paste_in(objects_);
	}

	// Paste as a shortcut
	bool PasteAsShortcutIn() {
		const Clipboard cb(hWnd_);
		const bool ret = cb.paste_as_link_in(objects_[0]);
		if (ret) RequestUpdate();
		return ret;
	}

	// Display file properties
	void PopupFileProperty() {
		const ContextMenu cm(hWnd_);
		cm.show_property(objects_);
	}

	// Change the file name
	bool Rename(const std::wstring& path, const std::wstring& newFileName) {
		Operation so(hWnd_);
		return so.rename(path, newFileName);
	}

	// Get file information string
	void InformationStrings(std::vector<std::wstring>& items) {
		if (Path::is_root(objects_[0])) {  // When it is a drive
			uint64_t dSize, dFree;
			FileSystem::drive_size(objects_[0], dSize, dFree);
			auto sizeStr = FileSizeToStr(dSize, true, L"");
			auto usedStr = FileSizeToStr(dSize - dFree, true, L"");
			items.push_back(usedStr.append(1, L'/').append(sizeStr));
			items.push_back(FileSizeToStr(dFree, true, L"Free: "));
		} else {  // When it is a normal file
			std::wstring ctStr, mtStr;
			uint64_t size;
			const bool suc = FilesSize(size, ::GetTickCount64() + 1000);
			items.push_back(FileSizeToStr(size, suc, L"Size:\t"));

			// Get date
			HANDLE hf = ::CreateFile(objects_[0].c_str(), 0, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS, nullptr);
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
		if (cmd.find(COM_CREATE_NEW) == 0) { CreateNewFile(cmd.c_str() + 11); return 1; }
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
