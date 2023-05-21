//=========== (C) Copyright 2023 NTT All rights reserved. ===========
//
// Purpose: Camera class
//
//===================================================================

#include "CameraBase.h"

Camera::Camera(Vector3 pos, IDirect3DDevice9* Device, float fov, float nearz, float farz) : 
	position(pos),
	viewMatrix(DirectX::XMMatrixLookAtLH(DirectX::XMVectorSet(position.x, position.y, position.z, 0), DirectX::XMVectorSet(0, 0, 0, 0), DirectX::XMVectorSet(0, 1, 0, 0))), 
	viewProjectionMatrix(DirectX::XMMatrixPerspectiveFovLH(fov, 600 / 800, nearz, farz))
{
	Device->SetTransform(D3DTS_VIEW, (D3DMATRIX*)&viewMatrix);
	Device->SetTransform(D3DTS_PROJECTION, (D3DMATRIX*)&viewProjectionMatrix);
}

Vector3 Camera::GetPosition()
{
	return position;
}

DirectX::XMVECTOR Camera::GetRawPosition()
{
	return DirectX::XMVectorSet(position.x, position.y, position.z, 0);
}

void Camera::SetPosition(Vector3 pos)
{
	position = pos;
}

void Camera::SetPosition(float x, float y, float z)
{
	position = { x, y, z };
}