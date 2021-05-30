/**
 *
 * Utility for Temporal Error Mode Change
 *
 * @author Takuto Yanagida
 * @version 2021-05-30
 *
 */


#pragma once

#include <winbase.h>


class ErrorMode {

	UINT mode_;

public:

	ErrorMode() noexcept : mode_(::SetErrorMode(SEM_FAILCRITICALERRORS)) {}

	ErrorMode(const ErrorMode& inst)            = delete;
	ErrorMode(ErrorMode&& inst)                 = delete;
	ErrorMode& operator=(const ErrorMode& inst) = delete;
	ErrorMode& operator=(ErrorMode&& inst)      = delete;

	~ErrorMode() noexcept {
		::SetErrorMode(mode_);
	}

};
