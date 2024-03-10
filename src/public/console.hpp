#pragma once

#include <vector>
#include "baseapplication.hpp"

class CConsole : public CBaseApplication {
    static int argc;
    static std::vector<std::string> argv;
public:
    static void SetArgs(const int argC, const std::vector<std::string>& argV);
    static int GetArgc();
    static std::vector<std::string> GetArgv();
    static void Destroy();
    /**
     * @brief checks for the presence of a command line parameter
     * 
     * \param param -- parameter whose presence needs to be checked
     * \return the position (1 to argc-1) in the program's argument list
     *  where the given parameter appears, or 0 if not present
     */
    static short CheckParam(const char* param);
    static void Print(const char* format, ...);
    static void Print(const char8_t* format, ...);
    static void PrintLn(const char* format, ...);
    static void PrintLn(const char8_t* format, ...);
};

