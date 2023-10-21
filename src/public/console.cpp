#ifdef _WIN32
#include <Windows.h>
#endif
#include "console.hpp"
#include "common.hpp"

int CConsole::argc = 0;
char** CConsole::argv = 0;

void CConsole::SetArgs(const int argC, char** argV) {
    argc = argC;
    argv = argV;
}

int CConsole::GetArgc() {
    return argc;
}

const char** CConsole::GetArgv() {
    return const_cast<const char**>(argv);
}

void CConsole::Destroy() {
    for (int i = 0; i < argc; ++i) {
        delete[] argv[i];
    }
#ifdef _WIN32
    if (CBaseApplication::isDebugMode()) {
        FreeConsole();
    }
#endif
}

short CConsole::CheckParam(const char* param) {
    short i;
    for (i = 1; i < argc; i++)
    {
        if (!argv[i])
            continue;       // NEXTSTEP sometimes clears appkit vars.
        if (M_strcmp(param, argv[i]))
            return i;
    }
    return 0;
}

void CConsole::Print(const char* msg) {
    UNUSED(msg);
}

void CConsole::Print(const wchar_t* msg) {
    UNUSED(msg);
}

void CConsole::Print(const char8_t* msg) {
    UNUSED(msg);
}

