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

// base class for drawable / renderable objects - such as a cube

#pragma once

#include <d3d11_1.h>
#include <DirectXMath.h>
#include "wrl.h"
#include "structures.h"
#include <utility>
#include "Camera.h"

using namespace DirectX;

class IRenderable
{
public:
	IRenderable();
	virtual ~IRenderable();

	virtual HRESULT	InitCubeMesh(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pContext) = 0;
	virtual void	Update(const float deltaTime, ID3D11DeviceContext* pContext);
	virtual void	Draw(ID3D11DeviceContext* pContext, Camera* camera, ID3D11Buffer* m_pConstantBuffer);
	virtual void	Cleanup();

	const ID3D11Buffer* GetVertexBuffer() const { return m_vertexBuffer.Get(); }
	const ID3D11Buffer* GetIndexBuffer() const { return m_indexBuffer.Get(); }
	const ID3D11ShaderResourceView* GetTextureResourceView() const { return m_textureResourceView.Get(); }
	const XMFLOAT4X4* GetTransform() const { return &m_world; }
	void SetTransform(XMMATRIX newTransform);

	const ID3D11SamplerState* GetTextureSamplerState() const { return m_textureSampler.Get(); }
	ID3D11Buffer* GetMaterialConstantBuffer() const { return m_materialConstantBuffer.Get(); }

	void	SetPosition(const XMFLOAT3 position) { m_position = position; }
	void	SetScale(const XMFLOAT3 scale) { m_scale = scale; }
	void	SetRotate(const XMFLOAT3 rotation) { m_rotation = rotation; }

	XMFLOAT3	GetPosition() { return m_position; }
	XMFLOAT3	GetScale() { return m_scale; }
	XMFLOAT3	GetRotation() { return { m_rotation }; }
	void SetPixelShader(Microsoft::WRL::ComPtr <ID3D11PixelShader> pixelShader) { m_pixelShader = pixelShader; }

	Microsoft::WRL::ComPtr <ID3D11PixelShader> GetPixelShader() { return m_pixelShader; }
	void	ResetTransform() { SetPosition(m_orginalPosition); SetScale(m_orginalScale); SetRotate(m_orginalRotation); }

	void CalculateModelVectors(SimpleVertex* vertices, int vertexCount);

	void CalculateTangentBinormal(SimpleVertex v0, SimpleVertex v1, SimpleVertex v2, XMFLOAT3& normal, XMFLOAT3& tangent, XMFLOAT3& binormal);

	bool m_autoRotateX = false;
	bool m_autoRotateY = false;
	bool m_autoRotateZ = false;

	float m_autoRotationSpeed = 50.0f;

protected:

	XMFLOAT4X4													m_world;
	MaterialPropertiesConstantBuffer							m_material;

	Microsoft::WRL::ComPtr < ID3D11Buffer>						m_vertexBuffer = nullptr;
	Microsoft::WRL::ComPtr < ID3D11Buffer>						m_indexBuffer = nullptr;
	Microsoft::WRL::ComPtr < ID3D11ShaderResourceView>			m_textureResourceView = nullptr;
	Microsoft::WRL::ComPtr < ID3D11ShaderResourceView>			m_normalMapResourceView = nullptr;

	Microsoft::WRL::ComPtr < ID3D11SamplerState>				m_textureSampler = nullptr;

	Microsoft::WRL::ComPtr < ID3D11Buffer>						m_materialConstantBuffer = nullptr;
	XMFLOAT3													m_position;
	XMFLOAT3													m_orginalPosition;
	XMFLOAT3													m_scale = XMFLOAT3(1, 1, 1);
	XMFLOAT3													m_orginalScale = XMFLOAT3(1, 1, 1);
	XMFLOAT3													m_rotation;
	XMFLOAT3													m_orginalRotation;
	unsigned int												m_vertexCount = 0;

	Microsoft::WRL::ComPtr <ID3D11PixelShader> m_pixelShader = nullptr;
};
