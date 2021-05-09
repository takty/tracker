/**
 *
 * OLE File Dragging
 *
 * @author Takuto Yanagida
 * @version 2021-05-09
 *
 */


#pragma once

#include <vector>
#include <string>
#include <memory>
#include <windows.h>

#include "Path.hpp"
#include "Shell.hpp"


class DragFile {

	// Implementation of OLE IDropSource
	class DropSource : public IDropSource {

		ULONG refCount_;

	public:

		DropSource() noexcept : refCount_(1) {}

		DropSource(const DropSource& inst) = delete;
		DropSource(DropSource&& inst) = delete;
		DropSource& operator=(const DropSource& inst) = delete;
		DropSource& operator=(DropSource&& inst) = delete;

		virtual ~DropSource() {}

		HRESULT __stdcall QueryInterface(const IID &iid, void **ppv) override {
			if (!ppv) return E_NOINTERFACE;
			if (iid == IID_IDropSource || iid == IID_IUnknown) {
				AddRef();
				*ppv = this;
				return S_OK;
			} else {
				*ppv = nullptr;
				return E_NOINTERFACE;
			}
		}

		ULONG __stdcall AddRef() override {
			return ::InterlockedIncrement(&refCount_);
		}

		ULONG __stdcall Release() override {
			const auto count = ::InterlockedDecrement(&refCount_);
			if (count == 0) delete this;
			return count;
		}

		HRESULT __stdcall QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState) noexcept override {
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

		HRESULT __stdcall GiveFeedback(DWORD) noexcept override {
			return DRAGDROP_S_USEDEFAULTCURSORS;
		}

	};

public:

	static void start(const std::vector<std::wstring>& paths) {
		if (paths.empty()) return;

		auto dobj = static_cast<LPDATAOBJECT>(Shell::get_ole_ui_object(paths, IID_IDataObject));
		if (!dobj) return;

		auto ds = std::make_unique<DropSource>();
		const bool notDrive = !Path::is_root(paths.front());
		DWORD dwEffect = 0;
		::DoDragDrop(dobj, ds.get(), DROPEFFECT_MOVE * notDrive | DROPEFFECT_COPY | DROPEFFECT_LINK, &dwEffect);
		ds.get()->Release();

		dobj->Release();
	}

};
