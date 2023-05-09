//=========== (C) Copyright 2023 NTT All rights reserved. ===========
//
// Purpose: Error handler class
//
//===================================================================
#include "stdafx.h"
#include "CErrorHandler.h"

const wchar_t* assertstringgg = L"Assertion failed!\n\nFile: %s\nLine: %d\n\nExpression: %s\nReason: %s\0";

void CErrorHandler::Assert(const wchar_t* FileName, const int Line, const wchar_t* Expression, const wchar_t* Reason) {
	size_t AssertBytes = wcsnlen(assertstringgg, (1024 * 512));
	size_t FileNameBytes = wcsnlen(FileName, MAX_PATH);
	size_t ExpBytes = wcsnlen(Expression, MAX_PATH);
	size_t ResBytes = wcsnlen(Reason, (1024 * 512)); // На всякий случий.
	INT_TO_CHAR(Line, LineC);
	size_t LineBytes = strnlen(LineC, (1024 * 512));
	// 1 - Нулл-Терминатор.
	// Всё остальное посчитано используя wcsnlen.
#pragma warning(disable: 6386)
	wchar_t* assertmessage = (wchar_t*)malloc((AssertBytes + FileNameBytes + LineBytes + ExpBytes + ResBytes) + 1);
#pragma warning(default: 6386)
	if (assertmessage == NULL) {
		// Серьёзная хуйня.
		return;
	}
	wsprintf(assertmessage,
		assertstringgg,
		FileName,
		Line,
		Expression,
		Reason
	);
	int Button = MessageBox(NULL, assertmessage, L"Assertion failed!", MB_ICONERROR | MB_OKCANCEL);
	switch (Button) {
		/*case IDRETRY:
		*   // https://stackoverflow.com/questions/744055/gcc-inline-assembly-jump-to-label-outside-block?rq=1
		*   // https://stackoverflow.com/questions/6421433/address-of-labels-msvc
		*   // https://gcc.gnu.org/onlinedocs/gcc-3.2/gcc/Labels-as-Values.html
		*   // https://stackoverflow.com/questions/17357199/using-goto-function-across-different-functions
		*   // https://stackoverflow.com/questions/6166437/64bit-applications-and-inline-assembly
		*   // https://xkcd.com/292/
		    break;*/
		case IDCANCEL:
#ifdef _DEBUG
			__debugbreak();
#else
			exit(-1);
#endif
			break;
		case IDOK:
			break;
		default:
			// How tf?
			break;
	}
}

void CErrorHandler::Throw(const char* message) {
	throw std::runtime_error(message);
}

void CErrorHandler::Catch(const std::function<void()>& code) {
	try {
		code();
	}
	catch (const std::exception& ex) {
		MessageBox(NULL, (LPCWSTR)ex.what(), L"fff", MB_OK || MB_ICONERROR);
		std::exit(1);
	}
}
