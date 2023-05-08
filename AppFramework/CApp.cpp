//=========== (C) Copyright 2023 NTT All rights reserved. ===========
//
//	purpose: application class
//
//===================================================================
#include "CApp.h"

CApp::CApp()
{
	D3DObject = nullptr;
	Device = nullptr;
}

CApp::~CApp()
{
}

void CApp::Init(HWND _handle)
{
	D3DObject = Direct3DCreate9(D3D_SDK_VERSION);
	if (D3DObject == nullptr)
	{
	Error:
		MessageBox(NULL, L"Unable to initialize D3D Device!!!!", L"Error!", MB_OK | MB_ICONERROR);
		return;
	}

	// Parameters for D3D. 
	D3DPRESENT_PARAMETERS d3dparams;
	ZeroMemory(&d3dparams, sizeof(D3DPRESENT_PARAMETERS));

	RECT rect;
	if (GetWindowRect(_handle, &rect))
	{
		d3dparams.BackBufferWidth = (UINT)(rect.right - rect.left);
		d3dparams.BackBufferHeight = (UINT)(rect.bottom - rect.top);
	} else
	{
		d3dparams.BackBufferWidth = 800;
		d3dparams.BackBufferHeight = 600;
	}

	d3dparams.hDeviceWindow = _handle;
	d3dparams.Windowed = true;
	d3dparams.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
	d3dparams.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dparams.AutoDepthStencilFormat = D3DFMT_D16;
	d3dparams.EnableAutoDepthStencil = true;

	// Creating Real Device
	if (D3DObject->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, _handle, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dparams, &Device) != S_OK)
	{
		goto Error;
	}

}
void CApp::Release()
{

}

void CApp::Render()
{

}