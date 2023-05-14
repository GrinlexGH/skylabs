//=========== (C) Copyright 2023 NTT All rights reserved. ===========
//
// Purpose: Error handler class header
//
//===================================================================
#pragma once

#pragma warning(disable:4365)
#include <assert.h>
#include <stdexcept>
#include <functional>
#pragma warning(default:4365)

#include "stdafx.h"

#ifdef USE_EXTENDED_ASSERT
#undef assert
#define assert(exp, res) (void)(														\
	(!!(exp)) ||																		\
	(CErrorHandler::Assert(_CRT_WIDE(__FILE__), __LINE__, _CRT_WIDE(#exp), res), 0)		\
	)
#endif

class CErrorHandler
{
public:
	static void Assert(const wchar_t* FileName, const int Line, const wchar_t* Expression, const wchar_t* Reason);
	static void Throw(const char* message);
	static void Catch(const std::function<void()>& code);
};