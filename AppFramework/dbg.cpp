//=========== (C) Copyright 2023 NTT All rights reserved. ===========
//
// Purpose: Error handler class
//
//===================================================================
#include "stdafx.h"
#include <string>
#include "dbg.h"

//������, std::wstring ����� ��� ����������� � �������, � std::wsting_view ����� ������ ����� ��������� ������ ��� ���������
void CErrorHandler::Assertation(std::wstring_view FileName, const int& Line, std::wstring_view Expression, std::wstring_view Reason, int& Button) {
	constexpr wchar_t TemplateString[] = L"Assertion failed!\n\nFile: %s\nLine: %d\n\nExpression: %s\nReason: %s";

	std::wstring LineC = std::to_wstring(Line);
	// 1 - ����-����������.
	size_t MessageSize = wcsnlen_s(TemplateString, 10240) + FileName.size() + Expression.size() + Reason.size() + LineC.size() + 1;
	wchar_t* AssertMessage = new wchar_t[MessageSize];
	swprintf(AssertMessage, MessageSize, TemplateString, FileName.data(), Line, Expression.data(), Reason.data());

	Button = MessageBox(NULL, AssertMessage, L"Assertion failed!", MB_ICONERROR | MB_ABORTRETRYIGNORE);

	delete[] AssertMessage;
	/*switch (Button) {
			/*
			// ������ ����� �� �����.
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