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

class CErrorHandler
{
public:
	static void Assert(bool condition, const char* message) {
		//if (condition) {
			assert(condition && *message);
			//std::exit(1);
		//}
	}

	static void Throw(const char* message) {
		throw std::runtime_error(message);
	}

	static void Catch(const std::function<void()>& code) {
		try {
			code();
		}
		catch (const std::exception& ex) {
			MessageBox(NULL, (LPCWSTR)ex.what(), L"fff", MB_OK || MB_ICONERROR);
			std::exit(1);
		}
	}
};