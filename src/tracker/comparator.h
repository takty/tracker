/**
 * Comparator of File Items
 *
 * @author Takuto Yanagida
 * @version 2026-04-29
 */

#pragma once

#include <memory>

#include <shlwapi.h>

#include "item.h"

template <typename Derived> class CompBase {

	bool rev_;

public:

	CompBase(bool rev) noexcept : rev_(rev) {}

	bool operator()(const Item* it1, const Item* it2) const noexcept {
		if (!it1 || !it2) {
			return false;
		}
		#pragma warning(suppress: 26491)
		const bool ret = (it1->is_dir() != it2->is_dir())
			? it1->is_dir() > it2->is_dir()
			: static_cast<const Derived*>(this)->cmp(*it1, *it2);
		return rev_ ? !ret : ret;
	}

};

// Function Object for Comparing By Names
class CompByName : public CompBase<CompByName> {

public:

	using CompBase::CompBase;

	bool cmp(const Item& it1, const Item& it2) const noexcept {
		return ::StrCmpLogicalW(it1.name().c_str(), it2.name().c_str()) < 0;
	}

};

// Function Object for Comparing By Types
class CompByType : public CompBase<CompByType> {

public:

	using CompBase::CompBase;

	bool cmp(const Item& it1, const Item& it2) const noexcept {
		auto ext1 = path::ext(it1.path());
		auto ext2 = path::ext(it2.path());
		int res = ::lstrcmp(ext1.c_str(), ext2.c_str());
		if (res == 0) {
			res = ::StrCmpLogicalW(it1.name().c_str(), it2.name().c_str());
		}
		return res < 0;
	}

};

// Function Object for Comparing By Dates
class CompByDate : public CompBase<CompByDate> {

public:

	using CompBase::CompBase;

	bool cmp(const Item& it1, const Item& it2) const noexcept {
		return ::CompareFileTime(&(it1.time()), &(it2.time())) == 1;
	}

};

// Function Object for Comparing By Sizes
class CompBySize : public CompBase<CompBySize> {

public:

	using CompBase::CompBase;

	bool cmp(const Item& it1, const Item& it2) const noexcept {
		if (it1.size() == it2.size()) {
			return ::StrCmpLogicalW(it1.name().c_str(), it2.name().c_str()) < 0;
		}
		return it1.size() < it2.size();
	}

};
