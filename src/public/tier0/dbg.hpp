//===================================
//
// Purpose: declaration of all things related to debugging.
//
//===================================
#pragma once
/*
#include <string_view>
#ifdef _DEBUG
//Of course, you can call Assert everywhere, but this will not make sense.
void _Assertion(
	std::wstring_view FileName,
	const int& Line,
	std::wstring_view Expression,
	std::wstring_view Message,
	int& Button
);

//These are ingenious. The logical OR operator will not check,
//i.e. execute, the right side if the left side is true.
#define _AssertMsg(exp, msg)																						    \
    bool isRetry = false;                                                                                           \
	do {																											\
		int Button = 0;																								\
        isRetry = false;                                                                                            \
		if (!((exp) || (_Assertion(_CRT_WIDE(__FILE__), __LINE__, _CRT_WIDE(#exp), _CRT_WIDE(msg), Button), 0))) {	\
			switch (Button) {																						\
			case IDABORT: {	PostQuitMessage(1);	}	break;															\
			case IDRETRY: {	isRetry = true;     }	break;															\
			case IDIGNORE:							break;															\
			default:								break;															\
			}																										\
		}																											\
	} while (isRetry)
#endif

*/