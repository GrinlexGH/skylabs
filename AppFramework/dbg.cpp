//=========== (C) Copyright 2023 NTT All rights reserved. ===========
//
// Purpose: Error handler class
//
//===================================================================
#include "stdafx.h"
#include <string>
#include "dbg.h"

//Короче, std::wstring нужен для манипуляций с текстом, а std::wsting_view нужен только чтобы прочитать строку без изменений
void CErrorHandler::Assertation(std::wstring_view FileName, const int& Line, std::wstring_view Expression, std::wstring_view Reason, int& Button) {
	wchar_t const* const TemplateString = L"Assertion failed!\n\nFile: %s\nLine: %d\n\nExpression: %s\nReason: %s";

	//size_t TemplateStringSize	=	wcsnlen_s(TemplateString,	10240);
	//size_t FileNameSize			=	wcsnlen_s(FileName,			MAX_PATH);
	//size_t ExpSize				=	wcsnlen_s(Expression,		10240);
	//size_t ReasSize				=	wcsnlen_s(Reason,			10240);
	size_t TemplateStringSize = wcslen(TemplateString);
	size_t FileNameSize = FileName.size();
	size_t ExpSize = Expression.size();
	size_t ReasSize = Reason.size();
	std::wstring LineC = std::to_wstring(Line);
	size_t LineSize = LineC.size();
	//INT_TO_CHAR(Line, LineC);
	//size_t LineSize				=	strnlen(LineC,				10240);

	// 1 - Нуль-Терминатор.
	size_t MessageSize = TemplateStringSize + FileNameSize + ExpSize + ReasSize + LineSize + 1;
	std::wstring AssertMessage(MessageSize, L'\0');

	swprintf_s(AssertMessage.c_str(),
		MessageSize,
		TemplateString,
		FileName,
		Line,
		Expression,
		Reason);

	Button = MessageBox(NULL, AssertMessage, L"Assertion failed!", MB_ICONERROR | MB_ABORTRETRYIGNORE);

	/*switch (Button) {
		/*
		// Скорей всего не будет.
		case IDRETRY:
		    // https://stackoverflow.com/questions/744055/gcc-inline-assembly-jump-to-label-outside-block?rq=1
		    // https://stackoverflow.com/questions/6421433/address-of-labels-msvc
		    // https://gcc.gnu.org/onlinedocs/gcc-3.2/gcc/Labels-as-Values.html
		    // https://stackoverflow.com/questions/17357199/using-goto-function-across-different-functions
		    // https://stackoverflow.com/questions/6166437/64bit-applications-and-inline-assembly
		    // https://xkcd.com/292/
		    break;
	}*/
	delete[] AssertMessage;
}