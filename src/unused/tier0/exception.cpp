#include "tier0/exception.hpp"

CException::CException(std::wstring what) {
    reason = what;
}

std::wstring CException::what() {
    return reason;
}