#ifdef _WIN32

#include <Windows.h>
#include <string>

#include "unicode.hpp"

std::string getWinapiErrorMessage() {
  wchar_t *errorMsg = nullptr;
  ::FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, GetLastError(),
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), errorMsg, 0,
                   nullptr);
  std::string finalMsg{narrow(errorMsg)};
  ::LocalFree(errorMsg);
  return finalMsg;
}

#else
#error
#endif
