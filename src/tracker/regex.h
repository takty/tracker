//
// Bregexp Wrapper
// 2019-04-12
//


#pragma once

#include <windows.h>
#include <tchar.h>
#include <string>

#include "bregexp.h"
#include "string_converter.h"


class Regex {

	friend class Pattern;

	typedef int (*BREGEXP_MATCH)(char* str, char *target, char *targetendp, BREGEXP **rxp, char *msg);
	typedef void (*BREGEXP_FREE)(BREGEXP* rx);

	BREGEXP_MATCH bregexpMatch;
	BREGEXP_FREE bregexpFree;

	bool standBy_;
	HINSTANCE hBregexp_;

public:

	Regex() : standBy_(false) {
	}

	~Regex() {
		if(standBy_) freeLibrary();
	}

	bool loadLibrary() {
		standBy_ = false;

		hBregexp_ = ::LoadLibrary(_T("Bregexp.dll"));
		if(hBregexp_) {
			bregexpMatch = (BREGEXP_MATCH)::GetProcAddress(hBregexp_, "BMatch");
			bregexpFree = (BREGEXP_FREE)::GetProcAddress(hBregexp_, "BRegfree");
			standBy_ = true;
		}
		return standBy_;
	}

	void freeLibrary() {
		if(standBy_) {
			FreeLibrary(hBregexp_);
			standBy_ = false;
		}
	}

	bool isStandBy() {
		return standBy_;
	}

};


class Pattern {
	
	Regex* bm_;
	std::string pattern_;
	BREGEXP* rxp_;
	int* refCount_;

	StringConverter sc_;
	char msg_[80];

public:

	Pattern() : bm_(nullptr), rxp_(nullptr), refCount_(new int) {
	}

	Pattern(Regex& bm, const std::string& pattern) : bm_(&bm), pattern_(pattern), rxp_(nullptr), refCount_(new int) {
		char str[] = " ";
		bm_->bregexpMatch((char*)pattern.c_str(), str, str + 1, &rxp_, msg_);
		*refCount_ = 1;
	}

	Pattern(const Pattern& p) : bm_(p.bm_), pattern_(p.pattern_), rxp_(p.rxp_), refCount_(p.refCount_) {
		if(refCount_ != nullptr) ++(*refCount_);
	}

	~Pattern() {
		if(refCount_ != nullptr && --(*refCount_) == 0) {
			bm_->bregexpFree(rxp_);
			delete refCount_;
		}
	}

	Pattern& operator=(const Pattern& p) {
		bm_ = p.bm_;
		pattern_ = p.pattern_;
		rxp_ = p.rxp_;
		if(refCount_ != nullptr) delete refCount_;
		refCount_ = p.refCount_;
		if(refCount_ != nullptr) ++(*refCount_);
	}

	bool match(const wstring& str) {
		if(refCount_ == nullptr) return false;
		const char* mbs = sc_.convert(str);
		return bm_->bregexpMatch((char*)pattern_.c_str(), (char*)mbs, (char*)mbs + ::strlen(mbs), &rxp_, msg_) != 0;
	}

};
