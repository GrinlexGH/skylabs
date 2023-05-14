//=========== (C) Copyright 2023 NTT All rights reserved. ===========
//
// Purpose: Error handler class
//
//===================================================================
#include "stdafx.h"
#include "CErrorHandler.h"

void CErrorHandler::Assert(const wchar_t* FileName, const int Line, const wchar_t* Expression, const wchar_t* Reason) {
	wchar_t const* const TemplateString = L"Assertion failed!\n\nFile: %s\nLine: %d\n\nExpression: %s\nReason: %s";

	size_t TemplateStringSize	=	wcsnlen_s(TemplateString,	10240);
	size_t FileNameSize			=	wcsnlen_s(FileName,			MAX_PATH);
	size_t ExpSize				=	wcsnlen_s(Expression,		10240);
	size_t ReasSize				=	wcsnlen_s(Reason,			10240);
	INT_TO_CHAR(Line, LineC);
	size_t LineSize				=	strnlen(LineC,				10240);

	// 1 - Нуль-Терминатор.
	size_t MessageSize = TemplateStringSize + FileNameSize + ExpSize + ReasSize + LineSize + 1;
	wchar_t* AssertMessage = new wchar_t[MessageSize];

	swprintf_s(AssertMessage,
		MessageSize,
		TemplateString,
		FileName,
		Line,
		Expression,
		Reason);

	int Button = MessageBox(NULL, AssertMessage, L"Assertion failed!", MB_ICONERROR | MB_OKCANCEL);
	switch (Button) {
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
	delete[] AssertMessage;
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
}			//сука, если ещё кто-нибудь поставит новую строку в конце файла, я его в жопу выебу