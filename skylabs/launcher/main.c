#include <stddef.h>

typedef int (*main_t)(int argc, char* argv[]);

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>

static void ShowErrorW(const wchar_t* msg, const wchar_t* detail) {
    size_t len = wcslen(msg) + (detail ? wcslen(detail) : 0) + 10;
    wchar_t* buf = (wchar_t*)malloc(len * sizeof(wchar_t));
    if (buf) {
        _snwprintf(buf, len, L"%s\n%s", msg, detail ? detail : L"");
        MessageBoxW(NULL, buf, L"Launcher error", MB_OK | MB_ICONERROR);
        free(buf);
    }
}

static void ShowSystemError(const wchar_t* msg) {
    wchar_t* errorMsg = NULL;
    FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&errorMsg, 0, NULL
    );
    ShowErrorW(msg, errorMsg);
    if (errorMsg) LocalFree(errorMsg);
}

int WINAPI wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPWSTR /*lpCmdLine*/, int /*nShowCmd*/) {
    int ret = 1;

    // Get program path
    DWORD cap = MAX_PATH;
    wchar_t* exePath = (wchar_t*)malloc(cap * sizeof(wchar_t));
    while (1) {
        DWORD size = GetModuleFileNameW(NULL, exePath, cap);
        if (size == 0) {
            ShowSystemError(L"Failed to get program path:");
            free(exePath);
            return 1;
        }
        if (size < cap) break;
        cap += 100;
        exePath = (wchar_t*)realloc(exePath, cap * sizeof(wchar_t));
    }

    // Remove filename
    wchar_t* lastSlash = wcsrchr(exePath, L'\\');
    if (lastSlash) {
        *lastSlash = L'\0';
    }

    size_t libPathCap = wcslen(exePath) + 50;
    wchar_t* libPath = (wchar_t*)malloc(libPathCap * sizeof(wchar_t));
    _snwprintf(libPath, libPathCap, L"%s\\bin\\core.dll", exePath);
    free(exePath);

    HMODULE hCore = LoadLibraryExW(libPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    free(libPath);

    if (!hCore) {
        ShowSystemError(L"Failed to load library:");
        return 1;
    }

    main_t coreMain = (main_t)(uintptr_t)GetProcAddress(hCore, "CoreMain");
    if (!coreMain) {
        ShowSystemError(L"Failed to load library function 'CoreMain':");
        FreeLibrary(hCore);
        return 1;
    }

    // Convert utf16 argv to utf8
    int argc = 0;
    LPWSTR* argvW = CommandLineToArgvW(GetCommandLineW(), &argc);
    char** argv = (char**)malloc(argc * sizeof(char*));

    for (int i = 0; i < argc; ++i) {
        int len = WideCharToMultiByte(CP_UTF8, 0, argvW[i], -1, NULL, 0, NULL, NULL);
        argv[i] = (char*)malloc(len);
        WideCharToMultiByte(CP_UTF8, 0, argvW[i], -1, argv[i], len, NULL, NULL);
    }
    LocalFree(argvW);

    ret = coreMain(argc, argv);

    for (int i = 0; i < argc; ++i) {
        free(argv[i]);
    }
    free(argv);
    FreeLibrary(hCore);

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
#include <linux/limits.h>

int main(int argc, char* argv[]) {
    char exePath[PATH_MAX];

    // Get program path
    ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
    if (len == -1) {
        perror("Failed to get program path");
        return 1;
    }
    exePath[len] = '\0';

    // Remove filename
    char* lastSlash = strrchr(exePath, '/');
    if (lastSlash) {
        *lastSlash = '\0';
    }

    char libPath[PATH_MAX];
    snprintf(libPath, sizeof(libPath), "%s/bin/core.so", exePath);

    void* hCore = dlopen(libPath, RTLD_LAZY);
    if (!hCore) {
        fprintf(stderr, "Failed to load library:\n%s\n", dlerror());
        return 1;
    }

    main_t coreMain = (main_t)dlsym(hCore, "CoreMain");
    const char* dlsym_error = dlerror();
    if (dlsym_error) {
        fprintf(stderr, "Failed to load library function:\n%s\n", dlsym_error);
        dlclose(hCore);
        return 1;
    }

    int ret = coreMain(argc, argv);

    dlclose(hCore);
    return ret;
}

#else
#error "Unsupported OS"
#endif
