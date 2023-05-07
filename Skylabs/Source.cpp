#define _WIN32_WINNT_WIN10_TH2 0x295A
#define _WIN32_WINNT_WIN10_RS1 0x3839
#define _WIN32_WINNT_WIN10_RS2 0x3AD7
#define _WIN32_WINNT_WIN10_RS3 0x3FAB
#define _WIN32_WINNT_WIN10_RS4 0x42EE
#define _WIN32_WINNT_WIN10_RS5 0x4563
#pragma warning(disable: 5039)
#include <Windows.h>
#pragma warning(default: 5039)

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		} break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int iCmdShow)
{
	HWND hWnd;
	WNDCLASSEX wc;

	ZeroMemory(&wc, sizeof(WNDCLASSEX));

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = L"WindowClass1";

	RegisterClassEx(&wc);

	hWnd = CreateWindowEx(NULL,
						L"WindowClass1",
						L"Windowed Program",
						WS_OVERLAPPEDWINDOW,
						300,
						300,
						500,
						400,
						NULL,
						NULL,
						hInstance,
						NULL);

	ShowWindow(hWnd, iCmdShow);

	MSG msg;

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(hPrevInstance);
}