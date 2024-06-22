#include "platform.hpp"
#include <SDL.h>

#ifdef _WIN32

#include <Windows.h>

void OnSize(HWND hwnd, UINT flag, int width, int height) {
  UNUSED(hwnd);
  UNUSED(flag);
  UNUSED(width);
  UNUSED(height);
  // Handle resizing
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                            LPARAM lParam) {
  switch (uMsg) {
  case WM_SIZE: {
    int width = LOWORD(lParam);  // Macro to get the low-order word.
    int height = HIWORD(lParam); // Macro to get the high-order word.

    // Respond to the message:
    OnSize(hwnd, (UINT)wParam, width, height);
  } break;
  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

HINSTANCE hinst;
HWND hwndMain;

DLL_EXPORT int CoreInit(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine,
             int nShowCmd) {
#else
DllExport int CoreInit(int argc, char **argv) {
#endif
#ifdef _WIN32
  UNUSED(hInstance);
  UNUSED(hPrevInstance);
  UNUSED(lpCmdLine);
  UNUSED(nShowCmd);
#endif
  return 0;
  /*
  MSG msg;
  BOOL bRet;
  WNDCLASS wc;
  UNREFERENCED_PARAMETER(lpCmdLine);

  // Register the window class for the main window.

  if (!hPrevInstance)
  {
      wc.style = 0;
      wc.lpfnWndProc = (WNDPROC)WindowProc;
      wc.cbClsExtra = 0;
      wc.cbWndExtra = 0;
      wc.hInstance = hInstance;
      wc.hIcon = LoadIcon((HINSTANCE)NULL,
          IDI_APPLICATION);
      wc.hCursor = LoadCursor((HINSTANCE)NULL,
          IDC_ARROW);
      wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
      wc.lpszMenuName = L"MainMenu";
      wc.lpszClassName = L"MainWndClass";

      if (!RegisterClass(&wc))
          return FALSE;
  }

  hinst = hInstance;  // save instance handle

  // Create the main window.

  hwndMain = CreateWindow(L"MainWndClass", L"Sample",
      WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
      CW_USEDEFAULT, CW_USEDEFAULT, (HWND)NULL,
      (HMENU)NULL, hinst, (LPVOID)NULL);

  // If the main window cannot be created, terminate
  // the application.

  if (!hwndMain)
      return FALSE;

  // Show the window and paint its contents.

  ShowWindow(hwndMain, nShowCmd);
  UpdateWindow(hwndMain);

  // Start the message loop.

  while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
  {
      if (bRet == -1)
      {
          // handle the error and possibly exit
      }
      else
      {
          TranslateMessage(&msg);
          DispatchMessage(&msg);
      }
  }

  // Return the exit code to the system.

  return (int)msg.wParam;*/
}
