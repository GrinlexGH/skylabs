#ifdef _WIN32

#include "appframework/appframework.hpp"
#include "tier0/exception.hpp"
#include "tier0/otherstuff.hpp"
#include <SDL.h>
#include <Windows.h>

void CApplication::SetInstance(HINSTANCE hInst) {
    hInstance = hInst;
}

HINSTANCE CApplication::GetInstance() {
    return hInstance;
}

bool CApplication::Run() {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        wchar_t errorMessage[4096] = L"SDL_InitError:\n%s";
        _snwprintf(errorMessage, sizeofarray(errorMessage), GetWC(SDL_GetError()));
        errorMessage[sizeofarray(errorMessage) - 1] = '\0';
        throw CException(errorMessage);
    }
    return true;
}

#endif

