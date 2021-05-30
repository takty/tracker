/**
 *
 * Comparator of File Items
 *
 * @author Takuto Yanagida
 * @version 2021-05-30
 *
 */


#pragma once

#include "item.h"


// Function Object for Comparing By Names
class CompByName {

	bool rev_;

public:

	CompByName(bool rev) noexcept : rev_(rev) {}

	bool operator()(const Item* it1, const Item* it2) noexcept {
		bool ret = false;
		if (!it1 || !it2) return ret;

		if ((it1->is_dir()) ^ (it2->is_dir())) {
			if ((it1->is_dir()) > (it2->is_dir())) ret = true;
		} else {
			if (::lstrcmp(it1->name().c_str(), it2->name().c_str()) < 0) ret = true;
		}
		return rev_ ? !ret : ret;
	}

};


// Function Object for Comparing By Types
class CompByType {

	bool rev_;

public:

	CompByType(bool rev) noexcept : rev_(rev) {}

	bool operator()(const Item* it1, const Item* it2) noexcept {
		bool ret = false;
		if (!it1 || !it2) return ret;

		if ((it1->is_dir()) ^ (it2->is_dir())) {
			if (it1->is_dir() > it2->is_dir()) ret = true;
		} else {
			auto ext1 = Path::ext(it1->path());
			auto ext2 = Path::ext(it2->path());
			int res = ::lstrcmp(ext1.c_str(), ext2.c_str());
			if (res == 0) res = ::lstrcmp(it1->name().c_str(), it2->name().c_str());
			if (res < 0) ret = true;
		}
		return rev_ ? !ret : ret;
	}

};


// Function Object for Comparing By Dates
class CompByDate {

	bool rev_;

public:

	CompByDate(bool rev) noexcept : rev_(rev) {}

	bool operator()(const Item* it1, const Item* it2) noexcept {
		bool ret = false;
		if (!it1 || !it2) return ret;

		if ((it1->is_dir()) ^ (it2->is_dir())) {
			if ((it1->is_dir()) > (it2->is_dir())) ret = true;
		} else {
			ret = ::CompareFileTime(&(it1->date()), &(it2->date())) == 1;
		}
		return rev_ ? !ret : ret;
	}

};


// Function Object for Comparing By Sizes
class CompBySize {

	bool rev_;

public:

	CompBySize(bool rev) noexcept : rev_(rev) {}

	bool operator()(const Item* it1, const Item* it2) noexcept {
		bool ret = false;
		if (!it1 || !it2) return ret;

		if ((it1->is_dir()) ^ (it2->is_dir())) {
			if ((it1->is_dir()) > (it2->is_dir())) ret = true;
		} else {
			if (it1->size() == it1->size()) {
				if (::lstrcmp(it1->name().c_str(), it2->name().c_str()) < 0) ret = true;
			} else {
				if (it1->size() < it2->size()) ret = true;
			}
		}
		return rev_ ? !ret : ret;
	}

};
