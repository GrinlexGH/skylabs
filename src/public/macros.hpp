#pragma once

#include <string>

#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

#define CurrentFunction (std::string(__PRETTY_FUNCTION__))

#define UNUSED(x) (void)(x)

#ifdef _WIN32
#define WideCharIsUTF16
#else
#define WideCharIsUTF32
#endif

#if defined(_MSC_VER)
    // Microsoft
    #define DllExport extern "C" __declspec(dllexport)
    #define DllImport __declspec(dllimport)
#elif defined(__GNUC__)
    // GCC
    #define DllExport extern "C" __attribute__((visibility("default")))
    #define DllImport
#endif

