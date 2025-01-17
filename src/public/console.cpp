#include "console.hpp"

#include "publicapi.hpp"

CDefaultConsoleMessage& operator<<(CDefaultConsoleMessage& s, std::ostream& (*f)(std::ostream&)) {
    f(std::cout);
    return s;
}

CDefaultConsoleMessage& operator<<(CDefaultConsoleMessage& s, std::ostream& (*f)(std::ios&)) {
    f(std::cout);
    return s;
}

CDefaultConsoleMessage& operator<<(CDefaultConsoleMessage& s, std::ostream& (*f)(std::ios_base&)) {
    f(std::cout);
    return s;
}


CColorfulConsoleMessage& operator<<(CColorfulConsoleMessage& s, std::ostream& (*f)(std::ostream&)) {
    f(std::cout);
    return s;
}

CColorfulConsoleMessage& operator<<(CColorfulConsoleMessage& s, std::ostream& (*f)(std::ios&)) {
    f(std::cout);
    return s;
}

CColorfulConsoleMessage& operator<<(CColorfulConsoleMessage& s, std::ostream& (*f)(std::ios_base&)) {
    f(std::cout);
    return s;
}

PUBLIC_CLASS CDefaultConsoleMessage Msg;
PUBLIC_CLASS CColorfulConsoleMessage Warning { { 215, 135, 0 } };
PUBLIC_CLASS CColorfulConsoleMessage Error { { 175, 0, 0 } };
