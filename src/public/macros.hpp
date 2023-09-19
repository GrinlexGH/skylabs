#pragma once

#include <string>
#include <source_location>

#define CurrentFunction (std::string(std::source_location::current().function_name()))

#define UNUSED(x) (void)(x)

#define DllExport   __declspec(dllexport)

