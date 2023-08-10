#include <cstring>
#include <cstdlib>

#define sizeofarray(arr) (sizeof(arr)/sizeof(*arr))

const wchar_t *GetWC(const char *c)
{
    const size_t cSize = strlen(c)+1;
    wchar_t* wc = new wchar_t[cSize];
    mbstowcs (wc, c, cSize);

    return wc;
}