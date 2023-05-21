//=========== (C) Copyright 2023 NTT All rights reserved. ===========
//
// Purpose: Camera class
//
//===================================================================
#pragma once


#include "math/vector.h"

#include <d3d9.h>
#pragma warning(disable: 4668)
#pragma warning(disable: 5264)
#include <DirectXMath.h>
#pragma warning(default: 4668)
#pragma warning(default: 5264)

#pragma warning(disable : 4820)
//#pragma warning(disable : 4626)
//#pragma warning(disable : 5027)

class Camera
{
public:

	Camera(Vector3 pos, IDirect3DDevice9* Device, float fov, float nearz, float farz);

	Vector3 GetPosition();
	DirectX::XMVECTOR GetRawPosition();
	void SetPosition(Vector3 pos);
	void SetPosition(float x, float y, float z);
	
private:
	DirectX::CXMMATRIX viewMatrix;
	DirectX::CXMMATRIX viewProjectionMatrix;
	Vector3 position = Vector3(0,0,0);
};

