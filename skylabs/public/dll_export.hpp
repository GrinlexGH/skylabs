#pragma once

#include "public_export.h"

#if defined(PLATFORM_WINDOWS) && !defined(COMPILER_GCC)
    #define DLL_EXPORT __declspec(dllexport)
    #define DLL_IMPORT __declspec(dllimport)
#else
    #define DLL_EXPORT __attribute__((visibility("default")))
    #define DLL_IMPORT
#endif

#define PUBLIC_INTERFACE extern "C" PUBLIC_EXPORT
#define PUBLIC_GLOBAL extern PUBLIC_EXPORT
#define PUBLIC_CLASS PUBLIC_EXPORT
