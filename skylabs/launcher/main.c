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
    if (!exePath) {
        PresentErrorMessage(L"Failed to allocate memory to get executable path");
        CLEANUP_AND_EXIT();
    }

    while (1) {
        DWORD size = GetModuleFileNameW(NULL, exePath, cap);
        if (size == 0) {
            ShowSystemError(L"Failed to get program path");
            CLEANUP_AND_EXIT();
        }

        if (size == cap - 1) {
            cap += 100;
            wchar_t* tmp = realloc(exePath, cap * sizeof(wchar_t));
            if (!tmp) {
                PresentErrorMessage(L"Failed to reallocate memory to get executable path");
                CLEANUP_AND_EXIT();
            }
            exePath = tmp;
        } else {
            exePath[size] = '\0';
            break;
        }
    }

    // Remove filename
    wchar_t* lastSlash = wcsrchr(exePath, L'\\');
    if (!lastSlash) {
        lastSlash = wcsrchr(exePath, L'/');
        if (!lastSlash) {
            PresentErrorMessage(L"Failed to get executable filename");
            CLEANUP_AND_EXIT();
        }
    }
    *lastSlash = L'\0';

    cap = (DWORD)(lastSlash - exePath) + _countof(LOAD_PATH);
    libPath = malloc(cap * sizeof(wchar_t));
    if (!libPath) {
        PresentErrorMessage(L"Failed to allocate memory for core path");
        CLEANUP_AND_EXIT();
    }

    swprintf(libPath, cap, L"%s" LOAD_PATH, exePath);
    free(exePath);
    exePath = NULL;

    hCore = LoadLibraryExW(libPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!hCore) {
        ShowSystemError(L"Failed to load library");
        CLEANUP_AND_EXIT();
    }
    free(libPath);
    libPath = NULL;

    main_t coreMain = (main_t)(uintptr_t)GetProcAddress(hCore, "CoreMain");
    if (!coreMain) {
        ShowSystemError(L"Failed to load library function");
        CLEANUP_AND_EXIT();
    }

    // Convert utf16 argv to utf8
    argc = 0;
    argvW = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argvW) {
        ShowSystemError(L"Failed to get command line");
        CLEANUP_AND_EXIT();
    }

    argv = calloc(argc, sizeof(char*));
    if (!argv) {
        PresentErrorMessage(L"Failed to allocate memory for command line arguments");
        CLEANUP_AND_EXIT();
    }

    for (int i = 0; i < argc; ++i) {
        int len = WideCharToMultiByte(CP_UTF8, 0, argvW[i], -1, NULL, 0, NULL, NULL);
        if (!len) {
            wchar_t msg[128];
            _snwprintf(msg, _countof(msg), L"Failed to get length of converted command line argument %d", i);
            ShowSystemError(msg);
            CLEANUP_AND_EXIT();
        }

        argv[i] = malloc(len);
        if (!argv[i]) {
            wchar_t msg[128];
            _snwprintf(msg, _countof(msg), L"Failed to allocate memory for command line argument %d", i);
            PresentErrorMessage(msg);
            CLEANUP_AND_EXIT();
        }

        len = WideCharToMultiByte(CP_UTF8, 0, argvW[i], -1, argv[i], len, NULL, NULL);
        if (!len) {
            wchar_t msg[128];
            _snwprintf(msg, _countof(msg), L"Failed to convert command line argument to UTF-8 %d", i);
            ShowSystemError(msg);
            CLEANUP_AND_EXIT();
        }
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
#define _POSIX_C_SOURCE 200112L

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/limits.h>

#define CLEANUP_AND_EXIT() do { ret = 1; goto cleanup; } while(0)
#define LOAD_PATH "/bin/core.so"

int main(int argc, char* argv[]) {
    int ret = 0;
    char* exePath = NULL;
    char* libPath = NULL;
    void* hCore = NULL;

    // Get program path
    int cap = PATH_MAX;
    exePath = malloc(sizeof(char) * cap);
    if (!exePath) {
        perror("Failed to allocate memory to get executable path");
        CLEANUP_AND_EXIT();
    }

    while (1) {
        ssize_t len = readlink("/proc/self/exe", exePath, cap - 1);
        if (len == -1) {
            perror("Failed to get program path");
            CLEANUP_AND_EXIT();
        }

        if (len == cap - 1) {
            cap += 100;
            char* tmp = realloc(exePath, sizeof(char) * cap);
            if (!tmp) {
                perror("Failed to reallocate memory to get executable path");
                CLEANUP_AND_EXIT();
            }
            exePath = tmp;
        } else {
            exePath[len] = '\0';
            break;
        }
    }

    // Remove filename
    char* lastSlash = strrchr(exePath, '/');
    if (lastSlash) {
        *lastSlash = '\0';
    }

    cap = (lastSlash - exePath) + (sizeof(LOAD_PATH) / sizeof(LOAD_PATH[0]));
    libPath = malloc(sizeof(char) * cap);
    if (!libPath) {
        perror("Failed to allocate memory for core path");
        CLEANUP_AND_EXIT();
    }

    snprintf(libPath, cap, "%s" LOAD_PATH, exePath);
    free(exePath);
    exePath = NULL;

    hCore = dlopen(libPath, RTLD_LAZY);
    if (!hCore) {
        fprintf(stderr, "Failed to load library:\n%s\n", dlerror());
        CLEANUP_AND_EXIT();
    }
    free(libPath);
    libPath = NULL;

    main_t coreMain = (main_t)dlsym(hCore, "CoreMain");
    const char* dlsymError = dlerror();
    if (dlsymError) {
        fprintf(stderr, "Failed to load library function:\n%s\n", dlsymError);
        CLEANUP_AND_EXIT();
    }

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
