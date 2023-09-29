#pragma once

#include <string>

#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

#define CurrentFunction (std::string(__PRETTY_FUNCTION__))

#define UNUSED(x) (void)(x)

#define DllExport __declspec(dllexport)



