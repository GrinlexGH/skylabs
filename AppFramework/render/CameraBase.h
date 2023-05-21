//=========== (C) Copyright 2023 NTT All rights reserved. ===========
//
// Purpose: Camera class
//
//===================================================================
#pragma once

#include "math/vector.h"
#include "stdafx.h"
#pragma warning(disable: 4668)
#pragma warning(disable: 5264)
#include <DirectXMath.h>
#pragma warning(default: 4668)
#pragma warning(default: 5264)

#pragma pack(push, 1)
class Camera
{
private:
	Vector3 position;
	DirectX::CXMMATRIX viewMatrix;
	DirectX::CXMMATRIX viewProjectionMatrix;
public:
	Camera(Vector3 pos, IDirect3DDevice9* Device, float fov, float nearz, float farz);
	Camera& operator=(const Camera&) = delete;

	Vector3 GetPosition();
	DirectX::XMVECTOR GetRawPosition();
	void SetPosition(Vector3 pos);
	void SetPosition(float x, float y, float z);
};
#pragma pack(pop)