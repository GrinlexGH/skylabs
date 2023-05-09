//=========== (C) Copyright 2023 NTT All rights reserved. ===========
//
// Purpose: Application class header
//
//===================================================================
#include "stdafx.h"

class CApplication
{
public:
	CApplication();
	~CApplication();

	void Init(HWND _handle);
	void Kill();
	void Render();

	IDirect3D9 *D3DObject;
	IDirect3DDevice9 *Device;
};