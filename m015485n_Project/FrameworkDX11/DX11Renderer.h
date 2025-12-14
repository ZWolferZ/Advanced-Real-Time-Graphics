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

// DX 11 Renderer class for encapsulating the responsibily of rendering, or calling the render methods of renderable objects

#pragma once

#include <d3d11_1.h>

#include "GameObject.h"

#include <vector>
#include <unordered_map>

#include "ImGuiRendering.h"

class Scene;

typedef vector<GameObject*> vecTypeDrawables;

struct ImGuiParameterState
{
	int selected_radio;
};

class DX11Renderer
{
public:
	DX11Renderer() = default;
	~DX11Renderer() = default;

	HRESULT Init(HWND hwnd);
	void	CleanUp();

	void	Update(const float deltaTime);

	// a helper method - todo: move to a unique class to reduce dependency on Renderer
	static HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);

	void Input(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void UpdateKeyInputs();

	float m_currentDeltaTime = 0.0f;
	float m_totalTime = 0.0f;

private: // methods
	HRESULT InitDevice(HWND hwnd);
	void    CleanupDevice();
	//void	initIMGUI(HWND hwnd);
	//void	IMGUIDraw(const unsigned int FPS);
	//void	startIMGUIDraw();
	//void	completeIMGUIDraw();
	void	CentreMouseInWindow(HWND hWnd);

private: // properties

	ImGuiRendering* m_imguiRenderer = nullptr;
	std::unordered_map<UINT8, bool > inputs;

	D3D_DRIVER_TYPE									m_driverType = D3D_DRIVER_TYPE_NULL;
	D3D_FEATURE_LEVEL								m_featureLevel = D3D_FEATURE_LEVEL_11_0;
	Microsoft::WRL::ComPtr <ID3D11Device>			m_pd3dDevice;
	Microsoft::WRL::ComPtr <ID3D11Device1>			m_pd3dDevice1;
	Microsoft::WRL::ComPtr <ID3D11DeviceContext>	m_pImmediateContext;
	Microsoft::WRL::ComPtr <ID3D11DeviceContext1>	m_pImmediateContext1;
	Microsoft::WRL::ComPtr <IDXGISwapChain>			m_pSwapChain;
	Microsoft::WRL::ComPtr <IDXGISwapChain1>		m_pSwapChain1;
	Microsoft::WRL::ComPtr <ID3D11RenderTargetView> m_pRenderTargetView;
	Microsoft::WRL::ComPtr <ID3D11Texture2D>		m_pDepthStencil;
	Microsoft::WRL::ComPtr <ID3D11DepthStencilView> m_pDepthStencilView;

	Microsoft::WRL::ComPtr <ID3D11VertexShader>		m_pVertexShader;
	Microsoft::WRL::ComPtr <ID3D11PixelShader>		m_pPixelShader;
	Microsoft::WRL::ComPtr <ID3D11PixelShader>		m_pSolidPixelShader;
	Microsoft::WRL::ComPtr <ID3D11PixelShader>		m_pTextureUnLitPixelShader;
	Microsoft::WRL::ComPtr <ID3D11InputLayout>		m_pVertexLayout;

	Scene* m_pScene;
};
