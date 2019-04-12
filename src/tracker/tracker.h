#pragma once

#include <string>

#include "resource.h"


//
// Common Header
// 2019-04-12
//

#define WM_REQUESTUPDATE    (WM_APP + 1)
#define WM_RENAMEEDITCLOSED (WM_APP + 2)


//
// Sections and Keys of INI File ----------------------------------------
//

const std::wstring SECTION_WINDOW(L"Window");

	const std::wstring KEY_RECENT_NUM(L"RecentNum");				const int VAL_RECENT_NUM = 8;
	const std::wstring KEY_SHOW_HIDDEN(L"ShowHidden");			const int VAL_SHOW_HIDDEN = 0;
	const std::wstring KEY_SORT_BY(L"SortBy");					const int VAL_SORT_BY = 0;
	const std::wstring KEY_SORT_REV(L"SortRev");					const int VAL_SORT_REV = 0;

	const std::wstring KEY_MULTI_USER(L"MultiUser");				const int VAL_MULTI_USER = 0;
	const std::wstring KEY_NO_LINKED(L"NoLinked");				const std::wstring VAL_NO_LINKED(L"");
	const std::wstring KEY_POPUP_POSITION(L"PopupPosition");		const int VAL_POPUP_POSITION = 0;
	const std::wstring KEY_SIDE_AREA_WIDTH(L"SideAreaWidth");		const int VAL_SIDE_AREA_WIDTH = 14;
	const std::wstring KEY_LINE_HEIGHT(L"LineHeight");			const int VAL_LINE_HEIGHT = 14;
	const std::wstring KEY_FULL_SCREEN_CHECK(L"FullScreenCheck");	const int VAL_FULL_SCREEN_CHECK = 0;

	const std::wstring KEY_WIDTH(L"Width");						const int VAL_WIDTH = 120;
	const std::wstring KEY_HEIGHT(L"Height");						const int VAL_HEIGHT = 480;

	const std::wstring KEY_POPUP_HOT_KEY(L"PopupHotKey");			const std::wstring VAL_POPUP_HOT_KEY(L"");
	const std::wstring KEY_FONT_NAME(L"FontName");				const std::wstring VAL_FONT_NAME(L"");
	const std::wstring KEY_FONT_SIZE(L"FontSize");				const int VAL_FONT_SIZE = 13;

	const std::wstring KEY_USE_MIGEMO(L"UseMigemo");				const int VAL_USE_MIGEMO = 0; 

const std::wstring SECTION_BOOKMARK(L"Favorite");

	const int MAX_BOOKMARK = 32;
	const std::wstring KEY_FILE(L"File");

const std::wstring SECTION_HISTORY(L"History");

	const int MAX_HISTORY = 32;


//
// Command ----------------------------------------
//

const std::wstring COM_CREATE_NEW(L"<CreateNew>");
const std::wstring COM_NEW_FOLDER(L"<NewFolder>");
const std::wstring COM_DELETE(L"<Delete>");
const std::wstring COM_CLONE(L"<Clone>");
const std::wstring COM_SHORTCUT(L"<Shortcut>");
const std::wstring COM_COPY_TO_DESKTOP(L"<CopyToDesktop>");
const std::wstring COM_MOVE_TO_DESKTOP(L"<MoveToDesktop>");
const std::wstring COM_COPY_PATH(L"<CopyPath>");
const std::wstring COM_COPY(L"<Copy>");
const std::wstring COM_CUT(L"<Cut>");
const std::wstring COM_PASTE(L"<Paste>");
const std::wstring COM_PASTE_SHORTCUT(L"<PasteShortcut>");
const std::wstring COM_PROPERTY(L"<Property>");
const std::wstring COM_OPEN(L"<Open>");
const std::wstring COM_OPEN_RESOLVE(L"<OpenResolve>");
const std::wstring COM_SELECT_ALL(L"<SelectAll>");
const std::wstring COM_RENAME(L"<Rename>");
const std::wstring COM_POPUP_INFO(L"<PopupInfo>");
const std::wstring COM_CLEAR_HISTORY(L"<ClearHistory>");
const std::wstring COM_FAVORITE(L"<Favorite>");
const std::wstring COM_START_DRAG(L"<StartDrag>");
const std::wstring COM_SHELL_MENU(L"<ShellMenu>");


//
// Special Extension ----------------------------------------
//

const std::wstring EXT_FOLDER(L"<folder>");
