//=========== (C) Copyright 2023 NTT All rights reserved. ===========
//
// Purpose: Precompiled header
//
//===================================================================
#pragma once
#define _WIN32_WINNT_WIN10_TH2 0x295A
#define _WIN32_WINNT_WIN10_RS1 0x3839
#define _WIN32_WINNT_WIN10_RS2 0x3AD7
#define _WIN32_WINNT_WIN10_RS3 0x3FAB
#define _WIN32_WINNT_WIN10_RS4 0x42EE
#define _WIN32_WINNT_WIN10_RS5 0x4563

#pragma warning(disable: 4820)
#pragma warning(disable: 5039)
#include <Windows.h>
#include <d3d9.h>
#pragma warning(default: 5039)
#pragma warning(default: 4820)

#define INT_TO_CHAR(num, numstr) char numstr [(((sizeof num) * CHAR_BIT) + 2)/3 + 2];	\
								 sprintf_s(##numstr, "%d", ##num)
