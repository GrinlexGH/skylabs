#define _WIN32_WINNT_WIN10_TH2 0x295A
#define _WIN32_WINNT_WIN10_RS1 0x3839
#define _WIN32_WINNT_WIN10_RS2 0x3AD7
#define _WIN32_WINNT_WIN10_RS3 0x3FAB
#define _WIN32_WINNT_WIN10_RS4 0x42EE
#define _WIN32_WINNT_WIN10_RS5 0x4563
#pragma warning(disable: 5039)
#include <Windows.h>
#pragma warning(default: 5039)

#pragma warning(disable: 4820)
#include <d3d9.h>
#pragma warning(default: 4820)

#pragma comment(lib, "d3d9.lib")

class CApp
{
public:
	CApp();
	~CApp();

	void Init(HWND _handle);
	void Release();
	void Render();

	IDirect3D9 *D3DObject;
	IDirect3DDevice9 *Device;
};

