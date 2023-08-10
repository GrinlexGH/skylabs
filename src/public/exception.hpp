#include <exception>
#include <string>

// Custom exception class
class Exception : std::exception
{
private:
    std::wstring message;
public:
    Exception(const wchar_t* msg) {
        message = msg;
    }

    Exception(std::wstring msg) {
        message = msg;
    }

    const char* what() const {

    }
};