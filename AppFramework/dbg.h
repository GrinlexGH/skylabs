//=========== (C) Copyright 2023 NTT All rights reserved. ===========
//
// Purpose: Error handler class header
//
//===================================================================
#pragma once
#pragma warning(disable: 4365)
#include <assert.h>

#include <string_view>
#pragma warning(default: 4365)

#ifdef _DEBUG

//Эти гениально. Оператор логического ИЛИ не будет проверять,
//то есть выполнять, правую часть если левая является истиной
#define _AssertMsg(exp, msg)																						\
	do {																											\
		int Button = 0;																								\
		if (!((exp) || (Assertation(_CRT_WIDE(__FILE__), __LINE__, _CRT_WIDE(#exp), _CRT_WIDE(msg), Button), 0))) {	\
			switch (Button) {																						\
			case IDABORT: {	PostQuitMessage(1);	}	break;															\
			case IDRETRY: {	__debugbreak();		}	break;															\
			case IDIGNORE:							break;															\
			default:								break;															\
			}																										\
		}																											\
	} while (0)
#endif

//Ты конечно можешь вызвать Assert везде, но в этом не будет смысла
void Assertation(std::wstring_view FileName, const int& Line, std::wstring_view Expression, std::wstring_view Message, int& Button);