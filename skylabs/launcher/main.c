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
        _snwprintf(buf, len, L"%s:\n%s", msg, detail ? detail : L"");
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
    if (errorMsg) LocalFree(errorMsg);
}

#define CLEANUP_AND_EXIT() do { ret = 1; goto cleanup; } while(0)
#define MESSAGE_EXIT_CHECK(expr, msg) do { if (expr) { PresentErrorMessage(msg); CLEANUP_AND_EXIT(); } } while(0)
#define SYSTEM_EXIT_CHECK(expr, msg) do { if (expr) { ShowSystemError(msg); CLEANUP_AND_EXIT(); } } while(0)
#define PRINTF_EXIT_CHECK(expr, msg, ...) do { if (expr) { wchar_t msgBuf[128]; _snwprintf(msgBuf, _countof(msgBuf), msg, __VA_ARGS__); ShowSystemError(msgBuf); CLEANUP_AND_EXIT(); } } while(0)
#define LOAD_PATH L"\\bin\\core.dll"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd) {
    (void)hInstance; (void)hPrevInstance; (void)lpCmdLine; (void)nShowCmd;

    int ret = 1;
    wchar_t* exePath = NULL;
    wchar_t* libPath = NULL;
    HMODULE hCore = NULL;
    int argc = 0;
    char** argv = NULL;
    LPWSTR* argvW = NULL;

    // Get program path
    DWORD cap = MAX_PATH;
    exePath = malloc(cap * sizeof(wchar_t));
    MESSAGE_EXIT_CHECK(!exePath, L"Failed to allocate memory to get executable path");

    while (1) {
        DWORD size = GetModuleFileNameW(NULL, exePath, cap);
        SYSTEM_EXIT_CHECK(size == 0, L"Failed to get program path");

        if (size == cap - 1) {
            cap += 100;
            wchar_t* tmp = realloc(exePath, cap * sizeof(wchar_t));
            MESSAGE_EXIT_CHECK(!tmp, L"Failed to reallocate memory to get executable path");
            exePath = tmp;
        } else {
            exePath[size] = '\0';
            break;
        }
    }

    // Remove filename
    wchar_t* lastSlash = wcsrchr(exePath, L'\\');
    *lastSlash = L'\0';

    cap = (DWORD)(lastSlash - exePath) + _countof(LOAD_PATH);
    libPath = malloc(cap * sizeof(wchar_t));
    MESSAGE_EXIT_CHECK(!libPath, L"Failed to allocate memory for core path");

    swprintf(libPath, cap, L"%s" LOAD_PATH, exePath);
    free(exePath);
    exePath = NULL;

    hCore = LoadLibraryExW(libPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    SYSTEM_EXIT_CHECK(!hCore, L"Failed to load library");
    free(libPath);
    libPath = NULL;

    main_t coreMain = (main_t)(uintptr_t)GetProcAddress(hCore, "CoreMain");
    SYSTEM_EXIT_CHECK(!coreMain, L"Failed to load library function");

    // Convert utf16 argv to utf8
    argc = 0;
    argvW = CommandLineToArgvW(GetCommandLineW(), &argc);
    SYSTEM_EXIT_CHECK(!argvW, L"Failed to get command line");

    argv = calloc(argc, sizeof(char*));
    MESSAGE_EXIT_CHECK(!argv, L"Failed to allocate memory for command line arguments");

    for (int i = 0; i < argc; ++i) {
        int len = WideCharToMultiByte(CP_UTF8, 0, argvW[i], -1, NULL, 0, NULL, NULL);
        PRINTF_EXIT_CHECK(!len, L"Failed to get length of converted command line argument %d", i);

        argv[i] = malloc(len);
        PRINTF_EXIT_CHECK(!len, L"Failed to allocate memory for command line argument %d", i);

        len = WideCharToMultiByte(CP_UTF8, 0, argvW[i], -1, argv[i], len, NULL, NULL);
        PRINTF_EXIT_CHECK(!len, L"Failed to convert command line argument %d to UTF-8", i);
    }
    LocalFree(argvW);
    argvW = NULL;

    ret = coreMain(argc, argv);

cleanup:
    if (exePath) free(exePath);
    if (libPath) free(libPath);
    if (hCore) FreeLibrary(hCore);
    if (argvW) LocalFree(argvW);
    if (argv) {
        for (int i = 0; i < argc; ++i) {
            if (argv[i]) free(argv[i]);
        }
        free(argv);
    }
    return ret;
}

int main() {
    /* Dummy main for console in debug */
    return wWinMain(GetModuleHandleW(NULL), NULL, GetCommandLineW(), SW_SHOWNORMAL);
}

#elifdef PLATFORM_LINUX
#define _XOPEN_SOURCE 700

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/limits.h>

#define CLEANUP_AND_EXIT() do { ret = 1; goto cleanup; } while(0)
#define PERROR_EXIT_CHECK(expr, msg) do { if (expr) { perror(msg); CLEANUP_AND_EXIT(); } } while(0)
#define PRINTF_EXIT_CHECK(expr, msg, ...) do { if (expr) { fprintf(stderr, msg, __VA_ARGS__); CLEANUP_AND_EXIT(); } } while(0)

#define LOAD_PATH "/bin/core.so"

int main(int argc, char* argv[]) {
    int ret = 0;
    char* exePath = NULL;
    char* libPath = NULL;
    void* hCore = NULL;

    // Get program path
    int cap = PATH_MAX;
    exePath = malloc(sizeof(char) * cap);
    PERROR_EXIT_CHECK(!exePath, "Failed to allocate memory to get executable path");

    exePath = realpath("/proc/self/exe", NULL);

    // Remove filename
    char* lastSlash = strrchr(exePath, '/');
    *lastSlash = '\0';

    cap = (lastSlash - exePath) + (sizeof(LOAD_PATH) / sizeof(LOAD_PATH[0]));
    libPath = malloc(sizeof(char) * cap);
    PERROR_EXIT_CHECK(!libPath, "Failed to allocate memory for core path");

    snprintf(libPath, cap, "%s" LOAD_PATH, exePath);
    free(exePath);
    exePath = NULL;

    hCore = dlopen(libPath, RTLD_LAZY);
    PRINTF_EXIT_CHECK(!hCore, "Failed to load library:\n%s\n", dlerror());
    free(libPath);
    libPath = NULL;

    main_t coreMain = (main_t)(uintptr_t)dlsym(hCore, "CoreMain");
    const char* dlsymError = dlerror();
    PRINTF_EXIT_CHECK(dlsymError, "Failed to load library function:\n%s\n", dlsymError);

    ret = coreMain(argc, argv);

cleanup:
    if (exePath) free(exePath);
    if (libPath) free(libPath);
    if (hCore) dlclose(hCore);
    return ret;
}

#else
#error "Unsupported OS"
#endif
