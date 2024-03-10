#pragma once

#include <string>

#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

#define UNUSED(x) (void)(x)

#if defined(_MSC_VER)
    // Microsoft
    #define DllExport extern "C" __declspec(dllexport)
    #define DllImport __declspec(dllimport)
#elif defined(__GNUC__)
    // GCC
    #define DllExport extern "C" __attribute__((visibility("default")))
    #define DllImport
#endif

