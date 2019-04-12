#pragma once

#include <WinBase.h>


//
// Utility for temporal error mode change
// 2016/02/24
//

class ErrorMode {

	int errorMode_;

public:

	ErrorMode() : errorMode_(::SetErrorMode(SEM_FAILCRITICALERRORS)) {}

	~ErrorMode() {
		::SetErrorMode(errorMode_);
	}

};
