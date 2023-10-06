#pragma once

#include <string>

#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

#define CurrentFunction (std::string(__PRETTY_FUNCTION__))

#define UNUSED(x) (void)(x)

#if defined(_MSC_VER)
    // Microsoft
    #define DllExport extern "C" __declspec(dllexport)
    #define DLLIMPORT __declspec(dllimport)
#elif defined(__GNUC__)
    // GCC
    #define DLLEXPORT __attribute__((visibility("default")))
    #define DLLIMPORT
#endif

