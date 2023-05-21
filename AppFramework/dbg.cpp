//=========== (C) Copyright 2023 NTT All rights reserved. ===========
//
// Purpose: Error handler class
//
//===================================================================
#include "pizdos.h"
#include "psapi.h"
#pragma warning(disable: 4365)
#include <string>
#pragma warning(default: 4365)
#include "dbg.h"
#include <d3d9.h>

//Короче, std::wstring нужен для манипуляций с текстом, а std::wsting_view нужен только чтобы прочитать строку без изменений
void Assertation(std::wstring_view FileName, const int& Line, std::wstring_view Expression, std::wstring_view Message, int& Button) {
	constexpr wchar_t TemplateString[] = L"Assertion failed!\n\nProgram: %s\nFile: %s\nLine: %d\n\nExpression: %s\nMessage: %s";
	wchar_t ProgramName[MAX_PATH];
	GetModuleFileNameEx(GetCurrentProcess(), NULL, ProgramName, MAX_PATH);
	std::wstring LineC = std::to_wstring(Line);
	// 1 - Нуль-Терминатор.
	size_t MessageSize = wcsnlen_s(TemplateString, 10240) + wcsnlen_s(ProgramName, 10240) + FileName.size() + Expression.size() + Message.size() + LineC.size() + 1;
	wchar_t* AssertMessage = new wchar_t[MessageSize];
	swprintf(AssertMessage, MessageSize, TemplateString, ProgramName , FileName.data(), Line, Expression.data(), Message.data());
	Button = MessageBox(NULL, AssertMessage, L"Assertion failed!", MB_ICONERROR | MB_ABORTRETRYIGNORE);

	delete[] AssertMessage;
	/*switch (Button) {
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
}