#include <stddef.h>

typedef int (*main_t)(int argc, char* argv[]);

#ifdef PLATFORM_WINDOWS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>

static void PresentError(const wchar_t* msg) {
    TASKDIALOGCONFIG tdc = { };
    tdc.cbSize = sizeof(TASKDIALOGCONFIG);
    tdc.hwndParent = NULL;
    tdc.hInstance = NULL;
    tdc.pszWindowTitle = L"Skylabs launcher notifier";
    tdc.pszContent = msg;
    tdc.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    tdc.pszMainIcon = TD_ERROR_ICON;
    tdc.dwFlags = TDF_SIZE_TO_CONTENT;

    TaskDialogIndirect(&tdc, NULL, NULL, NULL);
}

// Max user message size is 864 (or 1024 - 128 - 32)
static void PresentCError(const wchar_t* format, ...) {
    wchar_t systemMessage[128];
    _wcserror_s(systemMessage, _countof(systemMessage), errno);

    wchar_t contextMessage[1024 - 128 - 32];
    va_list args;
    va_start(args, format);
    _vsnwprintf_s(contextMessage, _countof(contextMessage), _TRUNCATE, format, args);
    va_end(args);

    wchar_t finalMessage[1024];
    _snwprintf_s(
        finalMessage, _countof(finalMessage), _TRUNCATE, L"%ls\n\nSystem reason: %ls", contextMessage, systemMessage
    );

    PresentError(finalMessage);
}

// Max user message size is 736 (or 1024 - 256 - 32)
static void PresentSystemError(const wchar_t* format, ...) {
    DWORD errorCode = GetLastError();

    wchar_t systemMessage[256];
    DWORD msgLen = FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK, NULL, errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), systemMessage, _countof(systemMessage), NULL
    );

    if (msgLen == 0) {
        _snwprintf_s(systemMessage, _countof(systemMessage), _TRUNCATE, L"Unknown Win32 Error (0x%08X)", errorCode);
    } else {
        while (msgLen > 0 && (systemMessage[msgLen - 1] == L'\r' || systemMessage[msgLen - 1] == L'\n')) {
            systemMessage[--msgLen] = L'\0';
        }
    }

    wchar_t contextMessage[1024 - 256 - 32];
    va_list args;
    va_start(args, format);
    _vsnwprintf_s(contextMessage, _countof(contextMessage), _TRUNCATE, format, args);
    va_end(args);

    wchar_t finalMessage[1024];
    _snwprintf_s(
        finalMessage, _countof(finalMessage), _TRUNCATE, L"%ls\n\nSystem reason: %ls", contextMessage, systemMessage
    );

    PresentError(finalMessage);
}

static void* SkMalloc(const size_t size) {
    void* p = malloc(size);
    if (!p) {
        PresentCError(L"Failed to allocate %zu bytes, wtf with your system bro💀", size);
        abort();
    }
    return p;
}

static void* SkRealloc(void* ptr, const size_t newSize) {
    void* p = realloc(ptr, newSize);
    if (!p) {
        PresentCError(L"Failed to reallocate %zu bytes, wtf with your system bro💀", newSize);
        abort();
    }
    return p;
}

static void* SkCalloc(const size_t num, const size_t size) {
    void* p = calloc(num, size);
    if (!p) {
        PresentCError(L"Failed to callocate %zu bytes, wtf with your system bro💀", num * size);
        abort();
    }
    return p;
}

#define LOAD_DIR L"\\bin\\"
#define LOAD_FILE L"core.dll"

static wchar_t* GetProgramPath(void) {
    wchar_t* result = NULL;
    wchar_t* exePath = NULL;

    DWORD cap = MAX_PATH;
    exePath = SkMalloc(cap * sizeof(wchar_t));

    for (;;) {
        DWORD size = GetModuleFileNameW(NULL, exePath, cap);

        if (size == 0) {
            PresentSystemError(L"Failed to get program path!");
            goto cleanup;
        }

        if (size == cap) {
            cap += MAX_PATH;
            exePath = SkRealloc(exePath, cap * sizeof(wchar_t));
        } else {
            exePath[size] = L'\0';

            result = exePath;
            exePath = NULL;
            break;
        }
    }

cleanup:
    free(exePath);
    return result;
}

static void GetCommandLineArguments(char*** argv, int* argc) {
    wchar_t** argvW = NULL;
    char** argvTemp = NULL;
    int argcTemp = 0;

    argvW = CommandLineToArgvW(GetCommandLineW(), &argcTemp);
    if (!argvW) {
        PresentSystemError(L"Failed to get command line arguments!");
        goto cleanup;
    }

    argvTemp = SkCalloc(argcTemp, sizeof(char*));

    for (int i = 0; i < argcTemp; ++i) {
        int len = WideCharToMultiByte(CP_UTF8, 0, argvW[i], -1, NULL, 0, NULL, NULL);
        if (!len) {
            PresentSystemError(L"Failed to get length of converted command line argument №%d!", i);
            goto cleanup;
        }

        argvTemp[i] = SkMalloc(len);

        len = WideCharToMultiByte(CP_UTF8, 0, argvW[i], -1, argvTemp[i], len, NULL, NULL);
        if (!len) {
            PresentSystemError(L"Failed to convert command line argument №%d to UTF-8!", i);
            goto cleanup;
        }
    }

    *argv = argvTemp;
    argvTemp = NULL;
    *argc = argcTemp;
    argcTemp = 0;

cleanup:
    if (argvTemp) {
        for (int i = 0; i < argcTemp; ++i) {
            free(argvTemp[i]);
        }
        free(argvTemp);
    }
    LocalFree(argvW);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd) {
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nShowCmd;

    int ret = 1;
    wchar_t* exePath = NULL;
    wchar_t* libPath = NULL;
    HMODULE hCore = NULL;
    LPWSTR* argvW = NULL;
    int argc = 0;
    char** argv = NULL;

    exePath = GetProgramPath();
    if (!exePath) {
        goto cleanup;
    }

    // Remove filename
    wchar_t* lastSlash = wcsrchr(exePath, L'\\');
    if (!lastSlash) {
        PresentError(L"Invalid path format!");
        goto cleanup;
    }

    *lastSlash = L'\0';

    DWORD cap = (DWORD)(lastSlash - exePath) + _countof(LOAD_DIR) + _countof(LOAD_FILE) - 1;
    libPath = SkMalloc(cap * sizeof(wchar_t));

    // Generate path to bin
    swprintf(libPath, cap, L"%ls" LOAD_DIR, exePath);
    if (!SetDllDirectoryW(libPath)) {
        PresentSystemError(L"Failed to set DLL search path!");
    }

    // Generate full dll path
    swprintf(libPath, cap, L"%ls" LOAD_DIR LOAD_FILE, exePath);
    free(exePath);
    exePath = NULL;

    hCore = LoadLibraryExW(libPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!hCore) {
        PresentSystemError(L"Failed to load library:\n%ls", libPath);
        goto cleanup;
    }
    free(libPath);
    libPath = NULL;

    main_t coreMain = (main_t)(uintptr_t)GetProcAddress(hCore, "CoreMain");
    if (!coreMain) {
        PresentSystemError(L"Failed to get \"CoreMain\" function address!");
        goto cleanup;
    }

    // Convert utf16 argv to utf8
    GetCommandLineArguments(&argv, &argc);
    if (!argv) {
        goto cleanup;
    }

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
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define CLEANUP_AND_EXIT() do { ret = 1; goto cleanup; } while(0)
#define PERROR_EXIT_CHECK(expr, msg) do { if (expr) { perror(msg); CLEANUP_AND_EXIT(); } } while(0)
#define PRINTF_EXIT_CHECK(expr, msg, ...) do { if (expr) { fprintf(stderr, msg, __VA_ARGS__); CLEANUP_AND_EXIT(); } } while(0)

#define LOAD_DIR "/lib/"
#define LOAD_FILE "core.so"

int main(int argc, char* argv[]) {
    int ret = 0;
    char* exePath = NULL;
    char* libPath = NULL;
    void* hCore = NULL;

    exePath = realpath("/proc/self/exe", NULL);
    PERROR_EXIT_CHECK(!exePath, "Failed to get executable path");

    // Got to root of the program file tree
    char* lastSlash = strrchr(exePath, '/');
    *lastSlash = '\0';
    lastSlash = strrchr(exePath, '/');
    *lastSlash = '\0';

    size_t cap = (lastSlash - exePath) + (sizeof(LOAD_DIR) / sizeof(LOAD_DIR[0])) + (sizeof(LOAD_FILE) / sizeof(LOAD_FILE[0])) - 1;
    libPath = malloc(cap * sizeof(char));
    PERROR_EXIT_CHECK(!libPath, "Failed to allocate memory for library path");

    // Generate full dll path
    snprintf(libPath, cap, "%s" LOAD_DIR LOAD_FILE, exePath);
    free(exePath);
    exePath = NULL;

    hCore = dlopen(libPath, RTLD_LAZY);
    const char* loadError = dlerror();
    PRINTF_EXIT_CHECK(!hCore, "Failed to load library:\n%s\n", loadError ? loadError : "Unknown error");
    free(libPath);
    libPath = NULL;

    main_t coreMain = (main_t)(uintptr_t)dlsym(hCore, "CoreMain");
    const char* dlsymError = dlerror();
    PRINTF_EXIT_CHECK(dlsymError, "Failed to load library function:\n%s\n", dlsymError);

    ret = coreMain(argc, argv);

cleanup:
    if (hCore) dlclose(hCore);
    free(libPath);
    free(exePath);
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
