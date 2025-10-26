/**
 * Bregexp Wrapper
 *
 * @author Takuto Yanagida
 * @version 2025-10-25
 */

#pragma once

#include <windows.h>
#include <tchar.h>
#include <string>

#include "classes.h"
#include "bregexp.h"
#include "string_converter.h"

class Regex {

	friend class Pattern;

	typedef int (*BREGEXP_MATCH)(char* str, char *target, char *targetendp, BREGEXP **rxp, char *msg);
	typedef void (*BREGEXP_FREE)(BREGEXP* rx);

	BREGEXP_MATCH bregexpMatch_;
	BREGEXP_FREE  bregexpFree_;

	bool      standBy_  = false;
	HINSTANCE hBregexp_ = nullptr;

public:

	Regex() noexcept : bregexpMatch_(nullptr), bregexpFree_(nullptr) {}

	Regex(const Regex&) = delete;
	Regex& operator=(const Regex&) = delete;
	Regex(Regex&&) = delete;
	Regex& operator=(Regex&&) = delete;

	~Regex() {
		if (standBy_) freeLibrary();
	}

	bool loadLibrary() noexcept {
		standBy_ = false;

		hBregexp_ = ::LoadLibrary(_T("Bregexp.dll"));
		if (hBregexp_) {
			bregexpMatch_ = reinterpret_cast<BREGEXP_MATCH>(::GetProcAddress(hBregexp_, "BMatch"));
			bregexpFree_  = reinterpret_cast<BREGEXP_FREE>(::GetProcAddress(hBregexp_, "BRegfree"));
			standBy_     = true;
		}
		return standBy_;
	}

	void freeLibrary() noexcept {
		if(standBy_) {
			FreeLibrary(hBregexp_);
			standBy_ = false;
		}
	}

	bool isStandBy() const noexcept {
		return standBy_;
	}

};

class Pattern {

	Regex*      bm_;
	std::string pattern_;
	BREGEXP*    rxp_;
	std::shared_ptr<int> refCount_;

	StringConverter sc_;
	char            msg_[80];

public:

	Pattern(Regex& bm, const std::string& pattern) : bm_(&bm), pattern_(pattern), rxp_(nullptr), refCount_(std::make_shared<int>()), msg_("") {
		char str[] = " ";
		bm_->bregexpMatch_(const_cast<char*>(pattern.data()), &str[0], &str[1], &rxp_, &msg_[0]);
		*refCount_ = 1;
	}

	Pattern(const Pattern& p) : bm_(p.bm_), pattern_(p.pattern_), rxp_(p.rxp_), refCount_(p.refCount_), msg_("") {
		if (refCount_) ++(*refCount_);
	}

	Pattern() = delete;
	Pattern(Pattern&&) = delete;
	Pattern& operator=(Pattern&&) = delete;

	~Pattern() {
		if (refCount_ && --(*refCount_) == 0) {
			bm_->bregexpFree_(rxp_);
		}
	}

	Pattern& operator=(const Pattern& p) {
		bm_       = p.bm_;
		pattern_  = p.pattern_;
		rxp_      = p.rxp_;
		refCount_ = p.refCount_;
		if (refCount_) ++(*refCount_);
	}

	bool match(const std::wstring& str) {
		if (!refCount_) return false;
		auto mbs = sc_.convert(str);
		auto cs  = mbs.get();
		return bm_->bregexpMatch_(pattern_.data(), cs, cs + ::strlen(cs), &rxp_, &msg_[0]) != 0;
	}

};
