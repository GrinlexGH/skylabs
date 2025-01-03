#pragma once

#if defined(COMPILER_MSVC)
    #define DLL_EXPORT extern "C" __declspec(dllexport)
    #define DLL_IMPORT extern "C" __declspec(dllimport)
    #define DLL_CLASS_EXPORT __declspec(dllexport)
    #define DLL_CLASS_IMPORT __declspec(dllimport)
    #define DLL_GLOBAL_EXPORT extern __declspec(dllexport)
    #define DLL_GLOBAL_IMPORT extern __declspec(dllimport)
#elif defined(COMPILER_GCC)
    #ifdef PLATFORM_LINUX
        #define DLL_EXPORT_ATTRIBUTE __attribute__((visibility("default")))
        #define DLL_IMPORT_ATTRIBUTE
    #else
        #define DLL_EXPORT_ATTRIBUTE __declspec(dllexport)
        #define DLL_IMPORT_ATTRIBUTE __declspec(dllimport)
    #endif
    #define DLL_EXPORT extern "C" DLL_EXPORT_ATTRIBUTE
    #define DLL_IMPORT extern "C" DLL_IMPORT_ATTRIBUTE
    #define DLL_CLASS_EXPORT DLL_EXPORT_ATTRIBUTE
    #define DLL_CLASS_IMPORT DLL_IMPORT_ATTRIBUTE
    #define DLL_GLOBAL_EXPORT extern DLL_EXPORT_ATTRIBUTE
    #define DLL_GLOBAL_IMPORT extern DLL_IMPORT_ATTRIBUTE
#endif

#ifdef PUBLIC_DLL_EXPORT
    #define PLATFORM_INTERFACE DLL_EXPORT
    #define PLATFORM_GLOBAL DLL_GLOBAL_EXPORT
    #define PLATFORM_CLASS DLL_CLASS_EXPORT
#else
    #define PLATFORM_INTERFACE DLL_IMPORT
    #define PLATFORM_GLOBAL DLL_GLOBAL_IMPORT
    #define PLATFORM_CLASS DLL_CLASS_IMPORT
#endif
