/**
 *
 * Utility for Temporal Error Mode Change
 *
 * @author Takuto Yanagida
 * @version 2025-10-21
 *
 */


#pragma once

#include <WinBase.h>


class ErrorMode {

	int errorMode_;

public:

	ErrorMode() noexcept : errorMode_(::SetErrorMode(SEM_FAILCRITICALERRORS)) {}

	ErrorMode(const ErrorMode&) = delete;
	ErrorMode& operator=(const ErrorMode&) = delete;
	ErrorMode(ErrorMode&&) = delete;
	ErrorMode& operator=(ErrorMode&&) = delete;

	~ErrorMode() {
		::SetErrorMode(errorMode_);
	}

};
