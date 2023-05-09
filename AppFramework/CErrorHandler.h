//=========== (C) Copyright 2023 NTT All rights reserved. ===========
//
// Purpose: Error handler class
//
//===================================================================
#pragma once
#pragma warning(disable:4365)
#include <assert.h>
#include <exception>
#include <stdexcept>
#include <functional>
#pragma warning(default:4365)
#undef assert

#define assert(exp, res) (void)(																				\
	(!!(exp)) ||																								\
	(CErrorHandler::Assert(_CRT_WIDE(__FILE__), __LINE__, _CRT_WIDE(#exp), res), 0)		\
)

#define INT_TO_CHAR(num, numstr) char numstr [(((sizeof num) * CHAR_BIT) + 2)/3 + 2]; \
								 sprintf_s(##numstr, "%d", ##num)

class CErrorHandler
{
public:
	static void Assert(const wchar_t* FileName, const int Line, const wchar_t* Expression, const wchar_t* Reason);
	static void Throw(const char* message);
	static void Catch(const std::function<void()>& code);
};
