#include <stddef.h>

typedef int (*main_t)(int argc, char* argv[]);

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>

static void PresentErrorMessage(const wchar_t* msg) {
    MessageBoxW(NULL, msg, L"Launcher error", MB_OK | MB_ICONERROR);
}

static void ShowError(const wchar_t* msg, const wchar_t* detail) {
    size_t len = wcslen(msg) + (detail ? wcslen(detail) : 0) + 10;
    wchar_t* buf = malloc(len * sizeof(wchar_t));
    if (buf) {
        _snwprintf_s(buf, len, _TRUNCATE, L"%s:\n%s", msg, detail ? detail : L"");
        PresentErrorMessage(buf);
        free(buf);
    } else {
        MessageBoxW(NULL, detail, msg, MB_OK | MB_ICONERROR);
    }
}

static void ShowSystemError(const wchar_t* msg) {
    wchar_t* errorMsg = NULL;
    FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&errorMsg, 0, NULL
    );
    ShowError(msg, errorMsg);
    LocalFree(errorMsg);
}

#define MESSAGE_CLEANUP_CHECK(expr, msg) do { if (expr) { PresentErrorMessage(msg); goto cleanup; } } while(0)
#define SYSTEM_CLEANUP_CHECK(expr, msg) do { if (expr) { ShowSystemError(msg); goto cleanup; } } while(0)

#define CLEANUP_AND_EXIT() do { ret = 1; goto cleanup; } while(0)
#define EXIT_CHECK(expr) do { if (expr) { CLEANUP_AND_EXIT(); } } while(0)
#define MESSAGE_EXIT_CHECK(expr, msg) do { if (expr) { PresentErrorMessage(msg); CLEANUP_AND_EXIT(); } } while(0)
#define SYSTEM_EXIT_CHECK(expr, msg) do { if (expr) { ShowSystemError(msg); CLEANUP_AND_EXIT(); } } while(0)
#define PRINTF_EXIT_CHECK(expr, msg, ...) do { if (expr) { wchar_t msgBuf[128]; _snwprintf_s(msgBuf, _countof(msgBuf), _TRUNCATE, msg, __VA_ARGS__); ShowSystemError(msgBuf); CLEANUP_AND_EXIT(); } } while(0)

#define LOAD_DIR L"\\bin\\"
#define LOAD_FILE L"core.dll"

wchar_t* GetProgramPath(void) {
    wchar_t* exePath = NULL;

    DWORD cap = MAX_PATH;
    exePath = malloc(cap * sizeof(wchar_t));
    MESSAGE_CLEANUP_CHECK(!exePath, L"Failed to allocate memory to get executable path");

    while (1) {
        DWORD size = GetModuleFileNameW(NULL, exePath, cap);
        SYSTEM_CLEANUP_CHECK(size == 0, L"Failed to get program path");

        if (size == cap - 1) {
            cap += 100;
            wchar_t* tmp = realloc(exePath, cap * sizeof(wchar_t));
            MESSAGE_CLEANUP_CHECK(!tmp, L"Failed to reallocate memory to get executable path");
            exePath = tmp;
        } else {
            exePath[size] = '\0';
            break;
        }
    }

    return exePath;

cleanup:
    free(exePath);
    return NULL;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd) {
    (void)hInstance; (void)hPrevInstance; (void)lpCmdLine; (void)nShowCmd;

    int ret = 0;
    wchar_t* exePath = NULL;
    wchar_t* libPath = NULL;
    HMODULE hCore = NULL;
    LPWSTR* argvW = NULL;
    int argc = 0;
    char** argv = NULL;

    exePath = GetProgramPath();
    EXIT_CHECK(!exePath);

    // Remove filename
    wchar_t* lastSlash = wcsrchr(exePath, L'\\');
    *lastSlash = L'\0';

    DWORD cap = (DWORD)(lastSlash - exePath) + _countof(LOAD_DIR) + _countof(LOAD_FILE) - 1;
    libPath = malloc(cap * sizeof(wchar_t));
    MESSAGE_EXIT_CHECK(!libPath, L"Failed to allocate memory for library path");

    // Generate path to bin
    swprintf(libPath, cap, L"%ls" LOAD_DIR, exePath);
    SYSTEM_EXIT_CHECK(!SetDllDirectoryW(libPath), L"Failed to set DLL directory");

    // Generate full dll path
    swprintf(libPath, cap, L"%ls" LOAD_DIR LOAD_FILE, exePath);
    free(exePath);
    exePath = NULL;

    hCore = LoadLibraryExW(libPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    SYSTEM_EXIT_CHECK(!hCore, L"Failed to load library");
    free(libPath);
    libPath = NULL;

    main_t coreMain = (main_t)(uintptr_t)GetProcAddress(hCore, "CoreMain");
    SYSTEM_EXIT_CHECK(!coreMain, L"Failed to load library function");

    // Convert utf16 argv to utf8
    argvW = CommandLineToArgvW(GetCommandLineW(), &argc);
    SYSTEM_EXIT_CHECK(!argvW, L"Failed to get command line");

    argv = calloc(argc, sizeof(char*));
    MESSAGE_EXIT_CHECK(!argv, L"Failed to allocate memory for command line arguments");

    for (int i = 0; i < argc; ++i) {
        int len = WideCharToMultiByte(CP_UTF8, 0, argvW[i], -1, NULL, 0, NULL, NULL);
        PRINTF_EXIT_CHECK(!len, L"Failed to get length of converted command line argument %d", i);

        argv[i] = malloc(len);
        PRINTF_EXIT_CHECK(!argv[i], L"Failed to allocate memory for command line argument %d", i);

        len = WideCharToMultiByte(CP_UTF8, 0, argvW[i], -1, argv[i], len, NULL, NULL);
        PRINTF_EXIT_CHECK(!len, L"Failed to convert command line argument %d to UTF-8", i);
    }
    LocalFree(argvW);
    argvW = NULL;

    ret = coreMain(argc, argv);

cleanup:
    if (argv) {
        for (int i = 0; i < argc; ++i) {
            free(argv[i]);
        }
        free(argv);
    }
    LocalFree(argvW);
    if (hCore) FreeLibrary(hCore);
    free(libPath);
    free(exePath);

    return ret;
}

int main() {
    /* Dummy main for console in debug */
    return wWinMain(GetModuleHandleW(NULL), NULL, GetCommandLineW(), SW_SHOWNORMAL);
}

#elifdef PLATFORM_LINUX
#include <dlfcn.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define CLEANUP_AND_EXIT() do { ret = 1; goto cleanup; } while(0)
#define PRINTF_EXIT_CHECK(expr, msg, ...) do { if (expr) { fprintf(stderr, msg, __VA_ARGS__); CLEANUP_AND_EXIT(); } } while(0)

#define LOAD_PATH "core.so"

int main(int argc, char* argv[]) {
    int ret = 0;
    void* hCore = NULL;

    hCore = dlopen(LOAD_PATH, RTLD_LAZY);
    const char* loadError = dlerror();
    PRINTF_EXIT_CHECK(!hCore, "Failed to load library:\n%s\n", loadError ? loadError : "Unknown error");

    main_t coreMain = (main_t)(uintptr_t)dlsym(hCore, "CoreMain");
    const char* dlsymError = dlerror();
    PRINTF_EXIT_CHECK(dlsymError, "Failed to load library function:\n%s\n", dlsymError);

    ret = coreMain(argc, argv);

cleanup:
    if (hCore) dlclose(hCore);
    return ret;
}

#elifdef PLATFORM_ANDROID
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_loadso.h>
#include <SDL3/SDL_log.h>

#define CLEANUP_AND_EXIT() do { ret = 1; goto cleanup; } while(0)
#define PRINTF_EXIT_CHECK(expr, msg, ...) do { if (expr) { SDL_Log(msg, __VA_ARGS__); CLEANUP_AND_EXIT(); } } while(0)

#define LOAD_PATH "core.so"

int main(int argc, char* argv[]) {
    int ret = 0;
    SDL_SharedObject* hCore = NULL;

    hCore = SDL_LoadObject(LOAD_PATH);
    PRINTF_EXIT_CHECK(!hCore, "Failed to load library:\n%s\n", SDL_GetError());

    main_t coreMain = (main_t)(uintptr_t)SDL_LoadFunction(hCore, "CoreMain");
    PRINTF_EXIT_CHECK(!coreMain, "Failed to load library function:\n%s\n", SDL_GetError());

    ret = coreMain(argc, argv);

cleanup:
    if (hCore) SDL_UnloadObject(hCore);
    return ret;
}

#else
#error "Unsupported OS"
#endif
