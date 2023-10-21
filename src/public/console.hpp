#pragma once

#include "baseapplication.hpp"

class CConsole : public CBaseApplication {
    static int argc;
    static char** argv;
public:
    static void SetArgs(const int argC, char** argV);
    static int GetArgc();
    static const char** GetArgv();
    static void Destroy();
    /**
     * @brief checks for the presence of a command line parameter
     * 
     * \param param -- parameter whose presence needs to be checked
     * \return the position (1 to argc-1) in the program's argument list
     *  where the given parameter appears, or 0 if not present
     */
    static short CheckParam(const char* param);
    static void Print(const char* msg);
    static void Print(const wchar_t* msg);
    static void Print(const char8_t* msg);
};

