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

//��� ���������. �������� ����������� ��� �� ����� ���������,
//�� ���� ���������, ������ ����� ���� ����� �������� �������
#ifdef _DEBUG

#define _AssertMsg(exp, reas)																										\
	do {																															\
		int Button = 0;																												\
		if (!((exp) || (CErrorHandler::Assertation(_CRT_WIDE(__FILE__), __LINE__, _CRT_WIDE(#exp), _CRT_WIDE(reas), Button), 0))) {	\
			switch (Button) {																										\
			case IDABORT: { exit(-1);		}	break;																				\
			case IDRETRY: { __debugbreak();	}	break;																				\
			case IDIGNORE:						break;																				\
			default:							break;																				\
			}																														\
		}																															\
	} while (0)
#endif

class CErrorHandler
{
public:
	//�� ������� ������ ������� Assert �� CErrorHandler �� � ���� �� ����� ������
	static void Assertation(std::wstring_view FileName, const int& Line, std::wstring_view Expression, std::wstring_view Reason, int& Button);
};