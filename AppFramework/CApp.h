#include "stdafx.h"

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