/**
 *
 * Comparator of File Items
 *
 * @author Takuto Yanagida
 * @version 2025-10-21
 *
 */


#pragma once

#include "Item.h"


// Function Object for Comparing By Names
class CompByName {

	bool rev_;

public:

	CompByName(bool rev) noexcept : rev_(rev) {}

	bool operator()(const Item* it1, const Item* it2) noexcept {
		if (it1 == nullptr || it2 == nullptr) {
			return false;
		}
		bool ret = false;
		if ((it1->IsDir()) ^ (it2->IsDir())) {
			if ((it1->IsDir()) > (it2->IsDir())) ret = true;
		} else {
			if (::lstrcmp(it1->Name().c_str(), it2->Name().c_str()) < 0) ret = true;
		}
		return rev_ ? !ret : ret;
	}

};


// Function Object for Comparing By Types
class CompByType {

	bool rev_;

public:

	CompByType(bool rev) noexcept : rev_(rev) {}

	bool operator()(const Item* it1, const Item* it2) {
		if (it1 == nullptr || it2 == nullptr) {
			return false;
		}
		bool ret = false;
		if ((it1->IsDir()) ^ (it2->IsDir())) {
			if (it1->IsDir() > it2->IsDir()) ret = true;
		} else {
			auto ext1 = Path::ext(it1->Path());
			auto ext2 = Path::ext(it2->Path());
			int res = ::lstrcmp(ext1.c_str(), ext2.c_str());
			if (res == 0) res = ::lstrcmp(it1->Name().c_str(), it2->Name().c_str());
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
		if (it1 == nullptr || it2 == nullptr) {
			return false;
		}
		bool ret = false;
		if ((it1->IsDir()) ^ (it2->IsDir())) {
			if ((it1->IsDir()) > (it2->IsDir())) ret = true;
		} else {
			ret = ::CompareFileTime(&(it1->Date()), &(it2->Date())) == 1;
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
		if (it1 == nullptr || it2 == nullptr) {
			return false;
		}
		bool ret = false;
		if ((it1->IsDir()) ^ (it2->IsDir())) {
			if ((it1->IsDir()) > (it2->IsDir())) ret = true;
		} else {
			if (it1->Size() == it1->Size()) {
				if (::lstrcmp(it1->Name().c_str(), it2->Name().c_str()) < 0) ret = true;
			} else {
				if (it1->Size() < it2->Size()) ret = true;
			}
		}
		return rev_ ? !ret : ret;
	}

};
