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
    TASKDIALOGCONFIG tdc;
    ZeroMemory(&tdc, sizeof(TASKDIALOGCONFIG));

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

static void PresentCError(const wchar_t* format, ...) {
    wchar_t systemMessage[64];
    _wcserror_s(systemMessage, _countof(systemMessage), errno);

    wchar_t contextMessage[1024];
    va_list args;
    va_start(args, format);
    _vsnwprintf_s(contextMessage, _countof(contextMessage), _TRUNCATE, format, args);
    va_end(args);

    wchar_t finalMessage[_countof(systemMessage) + _countof(contextMessage) + 32];
    _snwprintf_s(
        finalMessage, _countof(finalMessage), _TRUNCATE, L"%ls\n\nCRT error description: %ls", contextMessage, systemMessage
    );

    PresentError(finalMessage);
}

static void GetSystemErrorMessage(wchar_t* message, const DWORD size, const DWORD errorCode) {
    DWORD msgLen = FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK, NULL, errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), message, size, NULL
    );

    if (msgLen == 0) {
        _snwprintf_s(message, size, _TRUNCATE, L"Unknown Win32 Error (0x%08X)", errorCode);
    } else {
        while (msgLen > 0 && (message[msgLen - 1] == L'\r' || message[msgLen - 1] == L'\n')) {
            message[--msgLen] = L'\0';
        }
    }
}

static void PresentSystemError(const wchar_t* format, ...) {
    wchar_t systemMessage[256];
    GetSystemErrorMessage(systemMessage, _countof(systemMessage), GetLastError());

    wchar_t contextMessage[1024];
    va_list args;
    va_start(args, format);
    _vsnwprintf_s(contextMessage, _countof(contextMessage), _TRUNCATE, format, args);
    va_end(args);

    wchar_t finalMessage[_countof(systemMessage) + _countof(contextMessage) + 32];
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
        const DWORD size = GetModuleFileNameW(NULL, exePath, cap);

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
    int argc = 0;
    char** argv = NULL;

    exePath = GetProgramPath();
    if (!exePath) {
        goto cleanup;
    }

    // Remove filename
    wchar_t* lastSlash = wcsrchr(exePath, L'\\');
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
    if (hCore) FreeLibrary(hCore);
    free(libPath);
    free(exePath);

    return ret;
}

static void EnableVTP() {
    const HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (handle == INVALID_HANDLE_VALUE) {
        wchar_t systemMessage[256];
        GetSystemErrorMessage(systemMessage, _countof(systemMessage), GetLastError());
        printf("Failed to get stdout handle:\n%ls", systemMessage);
        return;
    }

    const DWORD vtpFlags = ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT;

    DWORD originalMode = 0;
    if (!GetConsoleMode(handle, &originalMode)) {
        wchar_t systemMessage[256];
        GetSystemErrorMessage(systemMessage, _countof(systemMessage), GetLastError());
        printf("Failed to get console mode:\n%ls", systemMessage);
        return;
    }

    if (!SetConsoleMode(handle, originalMode | vtpFlags)) {
        wchar_t systemMessage[256];
        GetSystemErrorMessage(systemMessage, _countof(systemMessage), GetLastError());
        printf("Failed to set virtual terminal processing flags:\n%ls", systemMessage);
    }
}

/* Dummy main for console in debug */
int main() {
    // Setup windows console
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    EnableVTP();

    return wWinMain(GetModuleHandleW(NULL), NULL, GetCommandLineW(), SW_SHOWNORMAL);
}

#elifdef PLATFORM_LINUX
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

#include <dlfcn.h>
#include <unistd.h>

#define countof(arr) (sizeof(arr) / sizeof((arr)[0]))

static void PresentCError(const char* format, ...) {
    char systemMessage[128];
    strerror_r(errno, systemMessage, countof(systemMessage));

    char contextMessage[1024];
    va_list args;
    va_start(args, format);
    snprintf(contextMessage, countof(contextMessage), format, args);
    va_end(args);

    fprintf(stderr, "%s\nSystem error description: %s\n", contextMessage, systemMessage);
}

static void* SkMalloc(const size_t size) {
    void* p = malloc(size);
    if (!p) {
        PresentCError("Failed to allocate %zu bytes, wtf with your system bro💀", size);
        abort();
    }
    return p;
}

#define CLEANUP_AND_EXIT() do { ret = 1; goto cleanup; } while(0)
#define PERROR_EXIT_CHECK(expr, msg) do { if (expr) { perror(msg); CLEANUP_AND_EXIT(); } } while(0)
#define PRINTF_EXIT_CHECK(expr, msg, ...) do { if (expr) { fprintf(stderr, msg, __VA_ARGS__); CLEANUP_AND_EXIT(); } } while(0)

#define LOAD_DIR "/lib/"
#define LOAD_FILE "core.so"

int main(int argc, char* argv[]) {
    int ret = 1;
    char* exePath = NULL;
    char* libPath = NULL;
    void* hCore = NULL;

    exePath = realpath("/proc/self/exe", NULL);
    if (!exePath) {
        PresentCError("Failed to get executable path!");
        goto cleanup;
    }

    // Got to root of the program file tree
    char* lastSlash = strrchr(exePath, '/');
    *lastSlash = '\0';
    lastSlash = strrchr(exePath, '/');
    *lastSlash = '\0';

    size_t cap = (lastSlash - exePath) + (sizeof(LOAD_DIR) / sizeof(LOAD_DIR[0])) + (sizeof(LOAD_FILE) / sizeof(LOAD_FILE[0])) - 1;
    libPath = SkMalloc(cap * sizeof(char));

    // Generate full dll path
    snprintf(libPath, cap, "%s" LOAD_DIR LOAD_FILE, exePath);
    free(exePath);
    exePath = NULL;

    hCore = dlopen(libPath, RTLD_LAZY);
    const char* loadError = dlerror();
    if (!hCore) {
        fprintf(stderr, "Failed to load library:\n%s\n", loadError ? loadError : "Unknown error");
        goto cleanup;
    }
    free(libPath);
    libPath = NULL;

    main_t coreMain = (main_t)(uintptr_t)dlsym(hCore, "CoreMain");
    const char* dlsymError = dlerror();
    if (!coreMain) {
        fprintf(stderr, "Failed to load library function:\n%s\n", dlsymError);
        goto cleanup;
    }

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
    int ret = 1;
    SDL_SharedObject* hCore = NULL;

    hCore = SDL_LoadObject(LOAD_PATH);
    if (!hCore) {
        SDL_Log("Failed to load library:\n%s\n", SDL_GetError());
        goto cleanup;
    }

    main_t coreMain = (main_t)(uintptr_t)SDL_LoadFunction(hCore, "CoreMain");
    if (!coreMain) {
        SDL_Log("Failed to load library function:\n%s\n", SDL_GetError());
    }

    ret = coreMain(argc, argv);

cleanup:
    if (hCore) SDL_UnloadObject(hCore);
    return ret;
}

#else
#error "Unsupported OS"
#endif
