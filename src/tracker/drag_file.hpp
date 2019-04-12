#pragma once

#include <vector>
#include <string>

#include <windows.h>

#include "Path.hpp"
#include "Shell.hpp"


//
// Utility class for ole file dragging
// 2016/03/04
//

class DragFile {

	// Implementation of OLE IDropSource
	class DropSource : public IDropSource {

		ULONG refCount_;

	public:

		DropSource() : refCount_(1) {}

		~DropSource() {}

		virtual HRESULT __stdcall QueryInterface(const IID &iid, void **ppv) {
			if (iid == IID_IDropSource || iid == IID_IUnknown) {
				AddRef();
				*ppv = this;
				return S_OK;
			} else {
				*ppv = nullptr;
				return E_NOINTERFACE;
			}
		}

		virtual ULONG __stdcall AddRef() {
			return ::InterlockedIncrement(&refCount_);
		}

		virtual ULONG __stdcall Release() {
			auto count = ::InterlockedDecrement(&refCount_);
			if (count == 0) delete this;
			return count;
		}

		virtual HRESULT __stdcall QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState) {
			if (fEscapePressed) {
				return DRAGDROP_S_CANCEL;
			}
			if (!(grfKeyState & MK_LBUTTON) && !(grfKeyState & MK_RBUTTON) && !(grfKeyState & MK_MBUTTON)) {
				return DRAGDROP_S_DROP;
			}
			if ((((grfKeyState & MK_LBUTTON) ? 1 : 0) + ((grfKeyState & MK_RBUTTON) ? 1 : 0) + ((grfKeyState & MK_MBUTTON) ? 1 : 0)) > 1) {
				return DRAGDROP_S_CANCEL;
			}
			return S_OK;
		}

		virtual HRESULT __stdcall GiveFeedback(DWORD) {
			return DRAGDROP_S_USEDEFAULTCURSORS;
		}

	};

public:

	static void start(const std::vector<std::wstring>& paths) {
		if (paths.empty()) return;

		auto dobj = (LPDATAOBJECT)Shell::get_ole_ui_object(paths, IID_IDataObject);
		if (!dobj) return;

		auto ds = new DropSource();
		bool notDrive = !Path::is_root(paths.front());
		DWORD dwEffect;
		::DoDragDrop(dobj, ds, DROPEFFECT_MOVE * notDrive | DROPEFFECT_COPY | DROPEFFECT_LINK, &dwEffect);
		ds->Release();

		dobj->Release();
	}

};
