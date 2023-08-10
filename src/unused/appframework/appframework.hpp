#ifdef _WIN32

#include <Windows.h>

class CApplication
{
private:
    static HINSTANCE hInstance;
public:
    static void SetInstance(HINSTANCE hInst);
    static HINSTANCE GetInstance();
    static bool Run();
};

#endif

