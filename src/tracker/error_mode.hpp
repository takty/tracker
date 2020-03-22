/**
 *
 * Utility for Temporal Error Mode Change
 *
 * @author Takuto Yanagida
 * @version 2020-03-22
 *
 */


#pragma once

#include <WinBase.h>


class ErrorMode {

	int errorMode_;

public:

	ErrorMode() : errorMode_(::SetErrorMode(SEM_FAILCRITICALERRORS)) {}

	~ErrorMode() {
		::SetErrorMode(errorMode_);
	}

};
