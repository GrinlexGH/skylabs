#include <exception>
#include <string>

class CException : public std::exception
{
private:
    std::wstring reason;
public:
    CException(std::wstring what);
    std::wstring what();
};