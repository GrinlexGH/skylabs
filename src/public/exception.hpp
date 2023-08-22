#pragma once

#include <exception>
#include "string.hpp"

class Exception : std::exception
{
private:
    String message;
public:
    /*Exception(const wchar_t* msg) {
        message = msg;
    }

    Exception(std::wstring msg) {
        message = msg;
    }

    const char* what() const {
        
    }*/
};

