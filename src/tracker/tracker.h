/**
 * Common Header
 *
 * @author Takuto Yanagida
 * @version 2025-11-19
 */

#pragma once

#include <string>
#include "resource.h"

#define WM_REQUESTUPDATE    (WM_APP + 1)
#define WM_RENAMEEDITCLOSED (WM_APP + 2)

//
// Sections and Keys of INI File -----------------------------------------------
//

const std::wstring SECTION_WINDOW(L"Window");

	const std::wstring KEY_MAX_HISTORY(L"MaxHistory");				constexpr int VAL_MAX_HISTORY      = 32;
	const std::wstring KEY_SHOW_HIDDEN(L"ShowHidden");				constexpr int VAL_SHOW_HIDDEN      = 0;
	const std::wstring KEY_SORT_BY(L"SortBy");						constexpr int VAL_SORT_BY          = 0;
	const std::wstring KEY_SORT_REV(L"SortRev");					constexpr int VAL_SORT_REV         = 0;
	const std::wstring KEY_SORT_HISTORY(L"SortHistory");			constexpr int VAL_SORT_HISTORY     = 1;
	const std::wstring KEY_SORT_HISTORY_BY(L"SortHistoryBy");		constexpr int VAL_SORT_HISTORY_BY  = 1;
	const std::wstring KEY_SORT_HISTORY_REV(L"SortHistoryRev");		constexpr int VAL_SORT_HISTORY_REV = 0;

	const std::wstring KEY_MULTI_USER(L"MultiUser");				constexpr int VAL_MULTI_USER        = 0;
	const std::wstring KEY_NO_LINKED(L"NoLinked");					const std::wstring VAL_NO_LINKED(L"");
	const std::wstring KEY_POPUP_POSITION(L"PopupPosition");		constexpr int VAL_POPUP_POSITION    = 0;
	const std::wstring KEY_SIDE_AREA_WIDTH(L"SideAreaWidth");		constexpr int VAL_SIDE_AREA_WIDTH   = 18;
	const std::wstring KEY_LINE_HEIGHT(L"LineHeight");				constexpr int VAL_LINE_HEIGHT       = 20;
	const std::wstring KEY_FULL_SCREEN_CHECK(L"FullScreenCheck");	constexpr int VAL_FULL_SCREEN_CHECK = 1;

	const std::wstring KEY_WIDTH(L"Width");							constexpr int VAL_WIDTH  = 256;
	const std::wstring KEY_HEIGHT(L"Height");						constexpr int VAL_HEIGHT = 768;

	const std::wstring KEY_POPUP_HOT_KEY(L"PopupHotKey");			const std::wstring VAL_POPUP_HOT_KEY(L"");
	const std::wstring KEY_FONT_NAME(L"FontName");					const std::wstring VAL_FONT_NAME(L"");
	const std::wstring KEY_FONT_SIZE(L"FontSize");					constexpr int VAL_FONT_SIZE = 16;

	const std::wstring KEY_USE_MIGEMO(L"UseMigemo");				constexpr int VAL_USE_MIGEMO = 0;

const std::wstring SECTION_BOOKMARK(L"Favorite");

	constexpr int MAX_BOOKMARK = 32;
	const std::wstring KEY_FILE(L"File");

const std::wstring SECTION_HISTORY(L"History");

	constexpr int MAX_HISTORY = 32;

//
// Command ---------------------------------------------------------------------
//

const std::wstring CMD_CREATE_NEW(L"<CreateNew>");
const std::wstring CMD_NEW_FOLDER(L"<NewFolder>");
const std::wstring CMD_DELETE(L"<Delete>");
const std::wstring CMD_CLONE(L"<Clone>");
const std::wstring CMD_SHORTCUT(L"<Shortcut>");
const std::wstring CMD_COPY_TO_DESKTOP(L"<CopyToDesktop>");
const std::wstring CMD_MOVE_TO_DESKTOP(L"<MoveToDesktop>");
const std::wstring CMD_COPY_PATH(L"<CopyPath>");
const std::wstring CMD_COPY(L"<Copy>");
const std::wstring CMD_CUT(L"<Cut>");
const std::wstring CMD_PASTE(L"<Paste>");
const std::wstring CMD_PASTE_SHORTCUT(L"<PasteShortcut>");
const std::wstring CMD_PROPERTY(L"<Property>");
const std::wstring CMD_OPEN(L"<Open>");
const std::wstring CMD_OPEN_RESOLVE(L"<OpenResolve>");
const std::wstring CMD_SELECT_ALL(L"<SelectAll>");
const std::wstring CMD_RENAME(L"<Rename>");
const std::wstring CMD_POPUP_INFO(L"<PopupInfo>");
const std::wstring CMD_CLEAR_HISTORY(L"<ClearHistory>");
const std::wstring CMD_FAVORITE(L"<Favorite>");
const std::wstring CMD_START_DRAG(L"<StartDrag>");
const std::wstring CMD_SHELL_MENU(L"<ShellMenu>");
