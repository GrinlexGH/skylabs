#pragma once

#if defined(COMPILER_GCC) && defined(PLATFORM_UNIX)
    #define DLL_EXPORT __attribute__((visibility("default")))
    #define DLL_IMPORT
#else
    #define DLL_EXPORT __declspec(dllexport)
    #define DLL_IMPORT __declspec(dllimport)
#endif

#ifdef PUBLIC_DLL_EXPORT
    #define PUBLIC_INTERFACE extern "C" DLL_EXPORT
    #define PUBLIC_GLOBAL extern DLL_EXPORT
    #define PUBLIC_CLASS DLL_EXPORT
#else
    #define PUBLIC_INTERFACE extern "C" DLL_IMPORT
    #define PUBLIC_GLOBAL extern DLL_IMPORT
    #define PUBLIC_CLASS DLL_IMPORT
#endif
