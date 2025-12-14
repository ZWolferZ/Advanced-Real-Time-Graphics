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

// Scene class for encapsulating the responsibily for storing / initialising the world and whatever is in it

#pragma once

#include "constants.h"
#include "Camera.h"
#include <d3d11_1.h>
#include "GameObject.h"
#include <vector>
#include  <filesystem>
#include <map>

class Scene
{
public:
	Scene() = default;
	~Scene() = default;

	HRESULT		Init(HWND hwnd, const Microsoft::WRL::ComPtr<ID3D11Device>& device, const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context);
	void LoadTextures();
	void CreateGameObjects();
	void		CleanUp();
	Camera* GetCamera() { return m_pCamera; }

	void		Update(const float deltaTime);
	void		Draw();

	void PushBackPixelShaders(string name, Microsoft::WRL::ComPtr <ID3D11PixelShader>& pixelShader) { m_pixelShadersMap.push_back({ name, pixelShader }); }
	Microsoft::WRL::ComPtr <ID3D11PixelShader>& GetPixelShader(const string& shaderToFind);
	LightPropertiesConstantBuffer& getLightProperties() { return m_lightProperties; }
	Microsoft::WRL::ComPtr < ID3D11ShaderResourceView>& GetTexture(vector<std::pair<string, Microsoft::WRL::ComPtr < ID3D11ShaderResourceView>>>& mapToCheck, const string& textureToFind);

	void SetupLightProperties();
	void UpdateLightProperties(unsigned int index, const Light& light);
	void UpdateLightBuffer();
	vector<GameObject*>		m_vecDrawables;
	vector<std::pair<string, Microsoft::WRL::ComPtr < ID3D11PixelShader>>> m_pixelShadersMap;
	vector<std::pair<string, Microsoft::WRL::ComPtr < ID3D11ShaderResourceView>>> m_textureMap;
	vector<std::pair<string, Microsoft::WRL::ComPtr < ID3D11ShaderResourceView>>> m_normalMapTextureMap;

private:
	Camera* m_pCamera;

	Microsoft::WRL::ComPtr <ID3D11Device>			m_pd3dDevice;
	Microsoft::WRL::ComPtr <ID3D11DeviceContext>	m_pImmediateContext;
	Microsoft::WRL::ComPtr <ID3D11Buffer>			m_pConstantBuffer;
	Microsoft::WRL::ComPtr <ID3D11Buffer>			m_pLightConstantBuffer;
	LightPropertiesConstantBuffer m_lightProperties;
};
