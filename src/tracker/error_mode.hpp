/**
 * Utility for Temporal Error Mode Change
 *
 * @author Takuto Yanagida
 * @version 2025-11-13
 */

#pragma once

#include <winbase.h>

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
