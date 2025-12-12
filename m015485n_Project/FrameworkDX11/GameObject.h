// MIT License
// Copyright (c) 2025 David White
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// A hard coded cube

#pragma once

#include <d3d11_1.h>
#include <DirectXMath.h>
#include <utility>

#include "wrl.h"
#include "structures.h"
#include "IRenderable.h"
#include <string>

using namespace DirectX;

class GameObject : public IRenderable
{
public:
	GameObject(XMFLOAT3 Position, XMFLOAT3 Rotation, XMFLOAT3 Scale, string ObjectName, vector<GameObject*>& drawList, ID3D11Device* m_pd3dDevice, ID3D11DeviceContext* m_pImmediateContext, Microsoft::WRL::ComPtr <ID3D11PixelShader> pixelShader, Microsoft::WRL::ComPtr < ID3D11ShaderResourceView> texture);
	~GameObject();

	string GetObjectName() { return objectName; }

	HRESULT	initCubeMesh(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pContext);

private: // variables

	string objectName = "null";
};
