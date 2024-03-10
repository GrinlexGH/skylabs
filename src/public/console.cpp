#ifdef _WIN32
#include <Windows.h>
#endif
#include <iostream>
#include <cstdarg>
#include <bit>
#include "console.hpp"
#include "common.hpp"
#include "unicode.hpp"

int CConsole::argc = 0;
std::vector<std::string> CConsole::argv;

void CConsole::SetArgs(const int argC, const std::vector<std::string>& argV) {
    argc = argC;
    argv = argV;
}

int CConsole::GetArgc() {
    return argc;
}

std::vector<std::string> CConsole::GetArgv() {
    return argv;
}

void CConsole::Destroy() {
    argv.clear();
#ifdef _WIN32
    if (CBaseApplication::isDebugMode()) {
        FreeConsole();
    }
#endif
}

short CConsole::CheckParam(const char* param) {
    for (short i = 1; i < argc; i++)
    {
        if (M_strcmp(param, GetArgv()[i].c_str()))
            return i;
    }
    return 0;
}

void CConsole::Print(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void CConsole::Print(const char8_t* format, ...) {
    std::string msg = std::bit_cast<const char*>(format);
    va_list args;
    va_start(args, format);
    vprintf(msg.c_str(), args);
    va_end(args);
}

void CConsole::PrintLn(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    std::cout << std::endl;
}

void CConsole::PrintLn(const char8_t* format, ...) {
    std::string msg = std::bit_cast<const char*>(format);
    va_list args;
    va_start(args, format);
    vprintf(msg.c_str(), args);
    va_end(args);
    std::cout << std::endl;
}

