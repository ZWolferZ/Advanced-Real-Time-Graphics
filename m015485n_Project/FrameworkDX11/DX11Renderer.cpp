#include "DX11Renderer.h"
#include "Scene.h"

#include <d3dcompiler.h>
#include <string>

#include "globals.h"

void DX11Renderer::ToggleFullScreen()
{
	m_isBorderlessFullscreen = !m_isBorderlessFullscreen;

	if (m_isBorderlessFullscreen)
	{
		// Save windowed rect
		GetWindowRect(m_handle, &m_windowedRect);

		// Get monitor info
		HMONITOR hMon = MonitorFromWindow(m_handle, MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi = { sizeof(mi) };
		GetMonitorInfo(hMon, &mi);

		// Remove borders
		SetWindowLong(
			m_handle,
			GWL_STYLE,
			WS_POPUP | WS_VISIBLE
		);

		// Resize to monitor
		SetWindowPos(
			m_handle,
			HWND_TOP,
			mi.rcMonitor.left,
			mi.rcMonitor.top,
			mi.rcMonitor.right - mi.rcMonitor.left,
			mi.rcMonitor.bottom - mi.rcMonitor.top,
			SWP_FRAMECHANGED | SWP_NOOWNERZORDER
		);
	}
	else
	{
		// Restore windowed mode
		SetWindowLong(
			m_handle,
			GWL_STYLE,
			WS_OVERLAPPEDWINDOW | WS_VISIBLE
		);

		SetWindowPos(
			m_handle,
			HWND_TOP,
			m_windowedRect.left,
			m_windowedRect.top,
			m_windowedRect.right - m_windowedRect.left,
			m_windowedRect.bottom - m_windowedRect.top,
			SWP_FRAMECHANGED | SWP_NOOWNERZORDER
		);
	}
}

HRESULT DX11Renderer::Init(HWND hwnd)
{
	m_handle = hwnd;
	InitDevice(hwnd);

	m_pScene = new Scene;

	CreatePostProcessConstantBuffer();

	vector<Microsoft::WRL::ComPtr <ID3D11ShaderResourceView>> renderSRVs = {
		m_gBuffer.depth.rendersrv,
		m_gBuffer.normals.srv,
		m_gBuffer.worldPos.srv,
		m_gBuffer.albedo.srv,
		m_gBuffer.lightAccumulation.srv
	};

	m_imguiRenderer = new ImGuiRendering(hwnd, m_pd3dDevice.Get(), m_pImmediateContext.Get(), renderSRVs);

	// Compile the vertex shader
	ID3DBlob* pVSBlob = nullptr;
	HRESULT hr = DX11Renderer::CompileShaderFromFile(L"shader.fx", "VS", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the vertex shader
	hr = m_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_pVertexShader);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	}

	// Define the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	UINT numElements = ARRAYSIZE(layout);

	// Create the input layout
	hr = m_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(), &m_pVertexLayout);
	pVSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Set the input layout
	m_pImmediateContext->IASetInputLayout(m_pVertexLayout.Get());

	// Compile the pixel shader
	ID3DBlob* pPSBlob = nullptr;
	hr = CompileShaderFromFile(L"shader.fx", "PS", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the pixel shader
	hr = m_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPixelShader);

	// Compile the pixel shader
	hr = CompileShaderFromFile(L"shader.fx", "PSSolid", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the pixel shader
	hr = m_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pSolidPixelShader);

	hr = CompileShaderFromFile(L"shader.fx", "PSTextureUnLit", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the pixel shader
	hr = m_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pTextureUnLitPixelShader);

	hr = CompileShaderFromFile(L"shader.fx", "PSWriteGBuffer", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the pixel shader
	hr = m_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_WriteGBuffer);

	hr = CompileShaderFromFile(L"shader.fx", "PSWriteAlbedoBuffer", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the pixel shader
	hr = m_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_WriteAlbedo);

	hr = CompileShaderFromFile(L"shader.fx", "PS_DeferredLighting", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the pixel shader
	hr = m_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_LightAccumulationWrite);

	hr = CompileShaderFromFile(L"shader.fx", "PS_VisualiseDepth", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the pixel shader
	hr = m_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_visualiseDepth);


	hr = CompileShaderFromFile(L"shader.fx", "QuadPSFowardRendered", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the pixel shader
	hr = m_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_ForwardRendering);


	pPSBlob->Release();
	if (FAILED(hr))
		return hr;
	m_pScene->PushBackPixelShaders("Solid Pixel Shader", m_pSolidPixelShader);
	m_pScene->PushBackPixelShaders("Texture Pixel Shader", m_pPixelShader);
	m_pScene->PushBackPixelShaders("Texture UnLit Pixel Shader", m_pTextureUnLitPixelShader);

	CreateFullScreenQuad();

	m_pScene->Init(hwnd, m_pd3dDevice, m_pImmediateContext);

	return hr;
}

void DX11Renderer::CreateFullScreenQuad()
{
	SCREEN_VERTEX svQuad[4];

	svQuad[0].pos = XMFLOAT4(-1.0f, 1.0f, 0.0f, 1.0f);
	svQuad[0].tex = XMFLOAT2(0.0f, 0.0f);

	svQuad[1].pos = XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f);
	svQuad[1].tex = XMFLOAT2(1.0f, 0.0f);

	svQuad[2].pos = XMFLOAT4(-1.0f, -1.0f, 0.0f, 1.0f);
	svQuad[2].tex = XMFLOAT2(0.0f, 1.0f);

	svQuad[3].pos = XMFLOAT4(1.0f, -1.0f, 0.0f, 1.0f);
	svQuad[3].tex = XMFLOAT2(1.0f, 1.0f);

	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SCREEN_VERTEX) * 4;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData = {};
	InitData.pSysMem = svQuad;
	HRESULT hr = m_pd3dDevice->CreateBuffer(&bd, &InitData, g_pScreenQuadVB.GetAddressOf());
	if (FAILED(hr))
	{
		MessageBox(nullptr, L"Failed to init FullScreen Quad VBuffer", L"Error", MB_OK);
	}

	ID3DBlob* pVSBlob = nullptr;
	hr = DX11Renderer::CompileShaderFromFile(L"shader.fx", "QuadVS", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
	}

	// Create the vertex shader
	hr = m_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pQuadVS);
	if (FAILED(hr))
	{
		pVSBlob->Release();
	}

	// Define the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);

	// Create the input layout
	hr = m_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(), &g_pQuadLayout);
	pVSBlob->Release();
	if (FAILED(hr))	MessageBox(nullptr, L"Failed to create FullScreen Quad inputlayout", L"Error", MB_OK);

	ID3DBlob* pPSBlob = nullptr;
	hr = CompileShaderFromFile(L"shader.fx", "QuadPS", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
	}

	// Create the pixel shader
	hr = m_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pQuadPS);

	pPSBlob->Release();

	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = m_pd3dDevice->CreateSamplerState(&sampDesc, &m_textureSampler);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"Failed to init sampler in Full Screen Quad.", L"Error", MB_OK);
	}
}

HRESULT DX11Renderer::InitDevice(HWND hwnd)
{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect(hwnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		m_driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDevice(nullptr, m_driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
			D3D11_SDK_VERSION, &m_pd3dDevice, &m_featureLevel, &m_pImmediateContext);

		if (hr == E_INVALIDARG)
		{
			// DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
			hr = D3D11CreateDevice(nullptr, m_driverType, nullptr, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1,
				D3D11_SDK_VERSION, &m_pd3dDevice, &m_featureLevel, &m_pImmediateContext);
		}

		if (SUCCEEDED(hr))
			break;
	}
	if (FAILED(hr))
		return hr;

	// Obtain DXGI factory from device (since we used nullptr for pAdapter above)
	IDXGIFactory1* dxgiFactory = nullptr;
	{
		IDXGIDevice* dxgiDevice = nullptr;
		hr = m_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
		if (SUCCEEDED(hr))
		{
			IDXGIAdapter* adapter = nullptr;
			hr = dxgiDevice->GetAdapter(&adapter);
			if (SUCCEEDED(hr))
			{
				hr = adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory));
				adapter->Release();
			}
			dxgiDevice->Release();
		}
	}
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"Failed to create device.", L"Error", MB_OK);
		return hr;
	}

	// Create swap chain
	IDXGIFactory2* dxgiFactory2 = nullptr;
	hr = dxgiFactory->QueryInterface(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory2));
	if (dxgiFactory2)
	{
		// DirectX 11.1 or later
		hr = m_pd3dDevice->QueryInterface(__uuidof(ID3D11Device1), &m_pd3dDevice1);
		if (SUCCEEDED(hr))
		{
			(void)m_pImmediateContext->QueryInterface(__uuidof(ID3D11DeviceContext1), &m_pImmediateContext1);
		}

		DXGI_SWAP_CHAIN_DESC1 sd = {};
		sd.Width = width;
		sd.Height = height;
		sd.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		sd.SampleDesc.Count = 1;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = 2;
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		sd.Scaling = DXGI_SCALING_STRETCH;
		sd.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
		sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

		hr = dxgiFactory2->CreateSwapChainForHwnd(
			m_pd3dDevice.Get(),
			hwnd,
			&sd,
			nullptr,
			nullptr,
			&m_pSwapChain1
		);

		if (SUCCEEDED(hr))
		{
			hr = m_pSwapChain1->QueryInterface(__uuidof(IDXGISwapChain), &m_pSwapChain);
		}

		dxgiFactory2->Release();
	}
	else
	{
		// DirectX 11.0 systems
		DXGI_SWAP_CHAIN_DESC sd = {};
		sd.BufferCount = 1;
		sd.BufferDesc.Width = width;
		sd.BufferDesc.Height = height;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = hwnd;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.Windowed = TRUE;

		hr = dxgiFactory->CreateSwapChain(m_pd3dDevice.Get(), &sd, &m_pSwapChain);
	}

	// Note this tutorial doesn't handle full-screen swapchains so we block the ALT+ENTER shortcut
	dxgiFactory->MakeWindowAssociation(hwnd, 0);

	dxgiFactory->Release();

	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"Failed to create swapchain.", L"Error", MB_OK);
		return hr;
	}

	// Create a render target view
	ID3D11Texture2D* pBackBuffer = nullptr;
	hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"Failed to create a back buffer.", L"Error", MB_OK);
		return hr;
	}

	hr = m_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_PresentedRenderTargetView);
	pBackBuffer->Release();
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"Failed to create a render target.", L"Error", MB_OK);
		return hr;
	}
	D3D11_TEXTURE2D_DESC textureDesc;
	ZeroMemory(&textureDesc, sizeof(textureDesc));

	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;

	hr = m_pd3dDevice->CreateTexture2D(&textureDesc, NULL, &g_SceneRenderTargetTexture);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"Failed to create a render target Texture 2D.", L"Error", MB_OK);
		return hr;
	}

	renderTargetViewDesc.Format = textureDesc.Format;
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	renderTargetViewDesc.Texture2D.MipSlice = 0;
	hr = m_pd3dDevice->CreateRenderTargetView(g_SceneRenderTargetTexture.Get(), &renderTargetViewDesc, g_SceneRenderTargetView.GetAddressOf());
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"Failed to create a render target view", L"Error", MB_OK);
		return hr;
	}

	shaderResourceViewDesc.Format = textureDesc.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;

	hr = m_pd3dDevice->CreateShaderResourceView(g_SceneRenderTargetTexture.Get(), &shaderResourceViewDesc, g_SceneShaderResourceView.GetAddressOf());
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"Failed to create a render target shader resource view", L"Error", MB_OK);
		return hr;
	}

	// Create depth stencil texture
	D3D11_TEXTURE2D_DESC descDepth = {};
	descDepth.Width = width;
	descDepth.Height = height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_R24G8_TYPELESS;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL |
		D3D11_BIND_SHADER_RESOURCE;;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	hr = m_pd3dDevice->CreateTexture2D(&descDepth, nullptr, m_gBuffer.depth.m_pDepthStencil.GetAddressOf());
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"Failed to create a depth / stencil texture.", L"Error", MB_OK);
		return hr;
	}

	// Create the depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
	descDSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	hr = m_pd3dDevice->CreateDepthStencilView(m_gBuffer.depth.m_pDepthStencil.Get(), &descDSV, m_gBuffer.depth.m_pDepthStencilView.GetAddressOf());
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"Failed to create a depth / stencil view.", L"Error", MB_OK);
		return hr;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC descSRV = {};
	descSRV.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	descSRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	descSRV.Texture2D.MipLevels = 1;
	descSRV.Texture2D.MostDetailedMip = 0;

	hr = m_pd3dDevice->CreateShaderResourceView(
		m_gBuffer.depth.m_pDepthStencil.Get(),
		&descSRV,
		m_gBuffer.depth.srv.GetAddressOf()
	);

	D3D11_TEXTURE2D_DESC normalDesc = {};
	normalDesc.Width = width;
	normalDesc.Height = height;
	normalDesc.MipLevels = 1;
	normalDesc.ArraySize = 1;
	normalDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT; // normals
	normalDesc.SampleDesc.Count = 1;
	normalDesc.Usage = D3D11_USAGE_DEFAULT;
	normalDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	hr = m_pd3dDevice->CreateTexture2D(&normalDesc, nullptr, m_gBuffer.normals.texture.GetAddressOf());

	hr = m_pd3dDevice->CreateRenderTargetView(m_gBuffer.normals.texture.Get(), nullptr, m_gBuffer.normals.rtv.GetAddressOf());

	hr = m_pd3dDevice->CreateShaderResourceView(m_gBuffer.normals.texture.Get(), nullptr, m_gBuffer.normals.srv.GetAddressOf());

	hr = m_pd3dDevice->CreateTexture2D(&normalDesc, nullptr, m_gBuffer.depth.renderTexture.GetAddressOf());
	hr = m_pd3dDevice->CreateRenderTargetView(m_gBuffer.depth.renderTexture.Get(), nullptr, m_gBuffer.depth.rtv.GetAddressOf());
	hr = m_pd3dDevice->CreateShaderResourceView(m_gBuffer.depth.renderTexture.Get(), nullptr, m_gBuffer.depth.rendersrv.GetAddressOf());

	D3D11_TEXTURE2D_DESC descPosition = {};
	descPosition.Width = width;
	descPosition.Height = height;
	descPosition.MipLevels = 1;
	descPosition.ArraySize = 1;
	descPosition.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	descPosition.SampleDesc.Count = 1;
	descPosition.Usage = D3D11_USAGE_DEFAULT;
	descPosition.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	hr = m_pd3dDevice->CreateTexture2D(&descPosition, nullptr, m_gBuffer.worldPos.texture.GetAddressOf());

	hr = m_pd3dDevice->CreateRenderTargetView(m_gBuffer.worldPos.texture.Get(), nullptr, m_gBuffer.worldPos.rtv.GetAddressOf());

	hr = m_pd3dDevice->CreateShaderResourceView(m_gBuffer.worldPos.texture.Get(), nullptr, m_gBuffer.worldPos.srv.GetAddressOf());

	D3D11_TEXTURE2D_DESC descAlbedo = {};
	descAlbedo.Width = width;
	descAlbedo.Height = height;
	descAlbedo.MipLevels = 1;
	descAlbedo.ArraySize = 1;
	descAlbedo.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	descAlbedo.SampleDesc.Count = 1;
	descAlbedo.Usage = D3D11_USAGE_DEFAULT;
	descAlbedo.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	descAlbedo.CPUAccessFlags = 0;

	hr = m_pd3dDevice->CreateTexture2D(&descAlbedo, nullptr, m_gBuffer.albedo.texture.GetAddressOf());
	hr = m_pd3dDevice->CreateRenderTargetView(m_gBuffer.albedo.texture.Get(), nullptr, m_gBuffer.albedo.rtv.GetAddressOf());
	hr = m_pd3dDevice->CreateShaderResourceView(m_gBuffer.albedo.texture.Get(), nullptr, m_gBuffer.albedo.srv.GetAddressOf());

	// Light accumulation texture
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;

	hr = m_pd3dDevice->CreateTexture2D(&desc, nullptr, m_gBuffer.lightAccumulation.texture.GetAddressOf());
	hr = m_pd3dDevice->CreateRenderTargetView(m_gBuffer.lightAccumulation.texture.Get(), nullptr, m_gBuffer.lightAccumulation.rtv.GetAddressOf());
	hr = m_pd3dDevice->CreateShaderResourceView(m_gBuffer.lightAccumulation.texture.Get(), nullptr, m_gBuffer.lightAccumulation.srv.GetAddressOf());

	// Get the raw pointer.
	ID3D11RenderTargetView* rtv = m_PresentedRenderTargetView.Get();
	m_pImmediateContext->OMSetRenderTargets(1, &rtv, m_gBuffer.depth.m_pDepthStencilView.Get());

	// Setup the viewport
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	m_pImmediateContext->RSSetViewports(1, &vp);

	return S_OK;
}

void DX11Renderer::CleanUp()
{
	CleanupDevice();

	m_imguiRenderer->ShutDownImGui();

	m_pScene->CleanUp();
	delete m_pScene;
}

void DX11Renderer::CleanupDevice()
{
	// Remove any bound render target or depth/stencil buffer
	ID3D11RenderTargetView* nullViews[] = { nullptr };
	m_pImmediateContext->OMSetRenderTargets(_countof(nullViews), nullViews, nullptr);

	if (m_pImmediateContext) m_pImmediateContext->ClearState();
	// Flush the immediate context to force cleanup
	if (m_pImmediateContext1) m_pImmediateContext1->Flush();
	m_pImmediateContext->Flush();

	// no need to release DX assets as they are com pointers

	ID3D11Debug* debugDevice = nullptr;
	m_pd3dDevice->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&debugDevice));

	m_pd3dDevice.Reset();

	if (debugDevice != nullptr)
	{
		// handy for finding dx memory leaks
		debugDevice->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
	}
}

//--------------------------------------------------------------------------------------
// Helper for compiling shaders with D3DCompile
//
// With VS 11, we could load up prebuilt .cso files instead...
//--------------------------------------------------------------------------------------
HRESULT DX11Renderer::CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
	HRESULT hr = S_OK;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows
	// the shaders to be optimized and to run exactly the way they will run in
	// the release configuration of this program.
	dwShaderFlags |= D3DCOMPILE_DEBUG;

	// Disable optimizations to improve shader debugging
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ID3DBlob* pErrorBlob = nullptr;
	hr = D3DCompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel,
		dwShaderFlags, 0, ppBlobOut, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
			pErrorBlob->Release();
		}
		return hr;
	}
	if (pErrorBlob) pErrorBlob->Release();

	return S_OK;
}

void DX11Renderer::Input(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static bool mouseDown = false;

	extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return;

	UINT8 key = static_cast<UINT8>(wParam);

	switch (message)
	{
	case WM_KEYDOWN:
		inputs[key] = true;

		break;
	case WM_KEYUP:
		inputs[key] = false;

		break;
	case WM_RBUTTONDOWN:
		mouseDown = true;
		ShowCursor(false);

		break;
	case WM_RBUTTONUP:
		mouseDown = false;
		ShowCursor(true);

		break;
	case WM_MOUSEMOVE:
	{
		if (!mouseDown)
		{
			break;
		}
		// Get the dimensions of the window
		RECT rect;
		GetClientRect(hWnd, &rect);

		// Calculate the center position of the window
		POINT windowCenter;
		windowCenter.x = (rect.right - rect.left) / 2;
		windowCenter.y = (rect.bottom - rect.top) / 2;

		// Convert the client area point to screen coordinates
		ClientToScreen(hWnd, &windowCenter);

		// Get the current cursor position
		POINTS mousePos = MAKEPOINTS(lParam);
		POINT cursorPos = { mousePos.x, mousePos.y };
		ClientToScreen(hWnd, &cursorPos);

		// Calculate the delta from the window center
		POINT delta;
		delta.x = cursorPos.x - windowCenter.x;
		delta.y = cursorPos.y - windowCenter.y;

		// Update the camera with the delta
		// (You may need to convert POINT to POINTS or use the deltas as is)
		m_pScene->GetCamera()->UpdateLookAt({ static_cast<short>(delta.x), static_cast<short>(delta.y) });

		// Recenter the cursor
		SetCursorPos(windowCenter.x, windowCenter.y);
	}
	break;
	case WM_ACTIVATE:
		if (LOWORD(wParam) != WA_INACTIVE) {
			CentreMouseInWindow(hWnd);
		}
		break;
	}
}

void DX11Renderer::UpdateKeyInputs()
{
	if (KeyHeld('W')) m_pScene->GetCamera()->MoveForward(m_pScene->GetCamera()->m_cameraMoveSpeed * m_currentDeltaTime);
	if (KeyHeld('A')) m_pScene->GetCamera()->StrafeLeft(m_pScene->GetCamera()->m_cameraMoveSpeed * m_currentDeltaTime);
	if (KeyHeld('S')) m_pScene->GetCamera()->MoveBackward(m_pScene->GetCamera()->m_cameraMoveSpeed * m_currentDeltaTime);
	if (KeyHeld('D')) m_pScene->GetCamera()->StrafeRight(m_pScene->GetCamera()->m_cameraMoveSpeed * m_currentDeltaTime);

	if (KeyHeld('E') || KeyHeld(VK_SPACE))
		m_pScene->GetCamera()->MoveUp(m_pScene->GetCamera()->m_cameraMoveSpeed * m_currentDeltaTime);

	if (KeyHeld('Q') || KeyHeld(VK_SHIFT))
		m_pScene->GetCamera()->MoveDown(m_pScene->GetCamera()->m_cameraMoveSpeed * m_currentDeltaTime);

	if (KeyPressed('R'))
		m_pScene->GetCamera()->Reset();

	if (KeyPressed(VK_ESCAPE))
		PostQuitMessage(0);

	if (KeyPressed(VK_F1))
		m_imguiRenderer->VSyncEnabled = !m_imguiRenderer->VSyncEnabled;

	if (KeyPressed(VK_F11) || KeyPressed('F')) ToggleFullScreen();

	previnputs = inputs;
}

bool DX11Renderer::KeyHeld(int key)
{
	return inputs[key];
}

bool DX11Renderer::KeyPressed(int key)
{
	return inputs[key] && !previnputs[key];
}

bool DX11Renderer::KeyReleased(int key)
{
	return !inputs[key] && previnputs[key];
}

void DX11Renderer::OnResize(UINT width, UINT height)
{
	if (!m_pSwapChain)
		return;

	// Unbind render targets
	ID3D11RenderTargetView* nullRTV[] = { nullptr };
	m_pImmediateContext->OMSetRenderTargets(1, nullRTV, nullptr);

	// Release size-dependent resources
	m_PresentedRenderTargetView.Reset();

	m_gBuffer.Reset();

	g_SceneRenderTargetView.Reset();
	g_SceneRenderTargetTexture.Reset();
	g_SceneShaderResourceView.Reset();

	// Resize swap chain buffers
	HRESULT hr = m_pSwapChain->ResizeBuffers(
		0,              // keep buffer count
		width,
		height,
		DXGI_FORMAT_UNKNOWN, // keep format
		DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING
	);
	if (FAILED(hr))
		return;

	// Recreate back buffer RTV
	Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
	m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
	m_pd3dDevice->CreateRenderTargetView(
		backBuffer.Get(),
		nullptr,
		&m_PresentedRenderTargetView
	);

	D3D11_TEXTURE2D_DESC textureDesc;
	ZeroMemory(&textureDesc, sizeof(textureDesc));

	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;

	hr = m_pd3dDevice->CreateTexture2D(&textureDesc, NULL, &g_SceneRenderTargetTexture);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"Failed to create a render target Texture 2D.", L"Error", MB_OK);
	}

	renderTargetViewDesc.Format = textureDesc.Format;
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	renderTargetViewDesc.Texture2D.MipSlice = 0;
	hr = m_pd3dDevice->CreateRenderTargetView(g_SceneRenderTargetTexture.Get(), &renderTargetViewDesc, g_SceneRenderTargetView.GetAddressOf());
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"Failed to create a render target view", L"Error", MB_OK);
	}

	shaderResourceViewDesc.Format = textureDesc.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;

	hr = m_pd3dDevice->CreateShaderResourceView(g_SceneRenderTargetTexture.Get(), &shaderResourceViewDesc, g_SceneShaderResourceView.GetAddressOf());
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"Failed to create a render target shader resource view", L"Error", MB_OK);
	}

	// Create depth stencil texture
	D3D11_TEXTURE2D_DESC descDepth = {};
	descDepth.Width = width;
	descDepth.Height = height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_R24G8_TYPELESS;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL |
		D3D11_BIND_SHADER_RESOURCE;;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	hr = m_pd3dDevice->CreateTexture2D(&descDepth, nullptr, m_gBuffer.depth.m_pDepthStencil.GetAddressOf());
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"Failed to create a depth / stencil texture.", L"Error", MB_OK);
	}

	// Create the depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
	descDSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	hr = m_pd3dDevice->CreateDepthStencilView(m_gBuffer.depth.m_pDepthStencil.Get(), &descDSV, m_gBuffer.depth.m_pDepthStencilView.GetAddressOf());
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"Failed to create a depth / stencil view.", L"Error", MB_OK);
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC descSRV = {};
	descSRV.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	descSRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	descSRV.Texture2D.MipLevels = 1;
	descSRV.Texture2D.MostDetailedMip = 0;

	hr = m_pd3dDevice->CreateShaderResourceView(
		m_gBuffer.depth.m_pDepthStencil.Get(),
		&descSRV,
		m_gBuffer.depth.srv.GetAddressOf()
	);

	D3D11_TEXTURE2D_DESC normalDesc = {};
	normalDesc.Width = width;
	normalDesc.Height = height;
	normalDesc.MipLevels = 1;
	normalDesc.ArraySize = 1;
	normalDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT; // normals
	normalDesc.SampleDesc.Count = 1;
	normalDesc.Usage = D3D11_USAGE_DEFAULT;
	normalDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	hr = m_pd3dDevice->CreateTexture2D(&normalDesc, nullptr, m_gBuffer.normals.texture.GetAddressOf());

	hr = m_pd3dDevice->CreateRenderTargetView(m_gBuffer.normals.texture.Get(), nullptr, m_gBuffer.normals.rtv.GetAddressOf());

	hr = m_pd3dDevice->CreateShaderResourceView(m_gBuffer.normals.texture.Get(), nullptr, m_gBuffer.normals.srv.GetAddressOf());

	hr = m_pd3dDevice->CreateTexture2D(&normalDesc, nullptr, m_gBuffer.depth.renderTexture.GetAddressOf());
	hr = m_pd3dDevice->CreateRenderTargetView(m_gBuffer.depth.renderTexture.Get(), nullptr, m_gBuffer.depth.rtv.GetAddressOf());
	hr = m_pd3dDevice->CreateShaderResourceView(m_gBuffer.depth.renderTexture.Get(), nullptr, m_gBuffer.depth.rendersrv.GetAddressOf());

	D3D11_TEXTURE2D_DESC descPosition = {};
	descPosition.Width = width;
	descPosition.Height = height;
	descPosition.MipLevels = 1;
	descPosition.ArraySize = 1;
	descPosition.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	descPosition.SampleDesc.Count = 1;
	descPosition.Usage = D3D11_USAGE_DEFAULT;
	descPosition.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	hr = m_pd3dDevice->CreateTexture2D(&descPosition, nullptr, m_gBuffer.worldPos.texture.GetAddressOf());

	hr = m_pd3dDevice->CreateRenderTargetView(m_gBuffer.worldPos.texture.Get(), nullptr, m_gBuffer.worldPos.rtv.GetAddressOf());

	hr = m_pd3dDevice->CreateShaderResourceView(m_gBuffer.worldPos.texture.Get(), nullptr, m_gBuffer.worldPos.srv.GetAddressOf());

	D3D11_TEXTURE2D_DESC descAlbedo = {};
	descAlbedo.Width = width;
	descAlbedo.Height = height;
	descAlbedo.MipLevels = 1;
	descAlbedo.ArraySize = 1;
	descAlbedo.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	descAlbedo.SampleDesc.Count = 1;
	descAlbedo.Usage = D3D11_USAGE_DEFAULT;
	descAlbedo.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	descAlbedo.CPUAccessFlags = 0;

	hr = m_pd3dDevice->CreateTexture2D(&descAlbedo, nullptr, m_gBuffer.albedo.texture.GetAddressOf());
	hr = m_pd3dDevice->CreateRenderTargetView(m_gBuffer.albedo.texture.Get(), nullptr, m_gBuffer.albedo.rtv.GetAddressOf());
	hr = m_pd3dDevice->CreateShaderResourceView(m_gBuffer.albedo.texture.Get(), nullptr, m_gBuffer.albedo.srv.GetAddressOf());

	// Light accumulation texture
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;

	hr = m_pd3dDevice->CreateTexture2D(&desc, nullptr, m_gBuffer.lightAccumulation.texture.GetAddressOf());
	hr = m_pd3dDevice->CreateRenderTargetView(m_gBuffer.lightAccumulation.texture.Get(), nullptr, m_gBuffer.lightAccumulation.rtv.GetAddressOf());
	hr = m_pd3dDevice->CreateShaderResourceView(m_gBuffer.lightAccumulation.texture.Get(), nullptr, m_gBuffer.lightAccumulation.srv.GetAddressOf());

	// Get the raw pointer.
	ID3D11RenderTargetView* rtv = m_PresentedRenderTargetView.Get();
	m_pImmediateContext->OMSetRenderTargets(1, &rtv, m_gBuffer.depth.m_pDepthStencilView.Get());

	// Setup the viewport
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	m_pImmediateContext->RSSetViewports(1, &vp);

	// Update camera projection
	m_pScene->GetCamera()->SetProjectionMatrix(width, height);

	vector<Microsoft::WRL::ComPtr <ID3D11ShaderResourceView>> renderSRVs = {
	m_gBuffer.depth.rendersrv,
	m_gBuffer.normals.srv,
	m_gBuffer.worldPos.srv,
	m_gBuffer.albedo.srv,
	m_gBuffer.lightAccumulation.srv
	};

	m_imguiRenderer->UpdateSRVs(renderSRVs);
}

// Function to center the mouse in the window
void DX11Renderer::CentreMouseInWindow(HWND hWnd)
{
	// Get the dimensions of the window
	RECT rect;
	GetClientRect(hWnd, &rect);

	// Calculate the center position
	POINT center;
	center.x = (rect.right - rect.left) / 2;
	center.y = (rect.bottom - rect.top) / 2;

	// Convert the client area point to screen coordinates
	ClientToScreen(hWnd, &center);

	// Move the cursor to the center of the screen
	SetCursorPos(center.x, center.y);
}

void DX11Renderer::CreatePostProcessConstantBuffer()
{
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(PostProcessConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	Microsoft::WRL::ComPtr <ID3D11Buffer>* buf_out = &m_postProcessConstantBuffer;
	HRESULT hr = m_pd3dDevice->CreateBuffer(&bd, nullptr, buf_out->GetAddressOf());
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"Failed to create post-process constant buffer.", L"Error", MB_OK);
	}

	m_pImmediateContext->UpdateSubresource(m_postProcessConstantBuffer.Get(), 0, nullptr, &m_postProcessCBData, 0, 0);

	m_pImmediateContext->PSSetConstantBuffers(2, 1, &m_postProcessConstantBuffer);


}

void DX11Renderer::DrawFullScreenQuad(FullScreenQuadOptions options)
{
	m_pImmediateContext->IASetInputLayout(g_pQuadLayout.Get());
	m_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	UINT stride = sizeof(SCREEN_VERTEX);
	UINT offset = 0;
	ID3D11Buffer* vb = g_pScreenQuadVB.Get();
	m_pImmediateContext->IASetVertexBuffers(0, 1, &vb, &stride, &offset);

	m_pImmediateContext->VSSetShader(g_pQuadVS.Get(), nullptr, 0);
	ID3D11SamplerState* ss = m_textureSampler.Get();
	m_pImmediateContext->PSSetSamplers(0, 1, &ss);


	if (options == FullScreenQuadOptions::LightingQuad)
	{
		m_pImmediateContext->PSSetShader(m_LightAccumulationWrite.Get(), nullptr, 0);
		m_pImmediateContext->PSSetShaderResources(2, 1, m_gBuffer.normals.srv.GetAddressOf());

		m_pImmediateContext->PSSetShaderResources(4, 1, m_gBuffer.worldPos.srv.GetAddressOf());

		m_pImmediateContext->Draw(4, 0);

		// unbind
		ID3D11ShaderResourceView* nullSRV = nullptr;
		m_pImmediateContext->PSSetShaderResources(2, 1, &nullSRV);
		m_pImmediateContext->PSSetShaderResources(4, 1, &nullSRV);
	}
	else if (options == FullScreenQuadOptions::FullDeferredQuad)
	{
		m_pImmediateContext->PSSetShader(g_pQuadPS.Get(), nullptr, 0);
		m_pImmediateContext->PSSetShaderResources(2, 1, m_gBuffer.normals.srv.GetAddressOf());
		m_pImmediateContext->PSSetShaderResources(3, 1, m_gBuffer.depth.srv.GetAddressOf());
		m_pImmediateContext->PSSetShaderResources(4, 1, m_gBuffer.worldPos.srv.GetAddressOf());
		m_pImmediateContext->PSSetShaderResources(5, 1, m_gBuffer.albedo.srv.GetAddressOf());
		m_pImmediateContext->PSSetShaderResources(6, 1, m_gBuffer.lightAccumulation.srv.GetAddressOf());
		m_pImmediateContext->PSSetShaderResources(7, 1, m_gBuffer.depth.rendersrv.GetAddressOf());

		m_pImmediateContext->Draw(4, 0);

		// unbind
		ID3D11ShaderResourceView* nullSRV = nullptr;
		m_pImmediateContext->PSSetShaderResources(2, 1, &nullSRV);
		m_pImmediateContext->PSSetShaderResources(3, 1, &nullSRV);
		m_pImmediateContext->PSSetShaderResources(4, 1, &nullSRV);
		m_pImmediateContext->PSSetShaderResources(5, 1, &nullSRV);
		m_pImmediateContext->PSSetShaderResources(6, 1, &nullSRV);
		m_pImmediateContext->PSSetShaderResources(7, 1, &nullSRV);
	}
	else if (options == FullScreenQuadOptions::ForwardRenderingQuad)
	{
		m_pImmediateContext->PSSetShader(m_ForwardRendering.Get(), nullptr, 0);

		m_pImmediateContext->PSSetShaderResources(8, 1, g_SceneShaderResourceView.GetAddressOf());


		m_pImmediateContext->Draw(4, 0);

		ID3D11ShaderResourceView* nullSRV = nullptr;
		m_pImmediateContext->PSSetShaderResources(8, 1, &nullSRV);

	}
}

void DX11Renderer::DrawDepthScreenQuad()
{
	m_pImmediateContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	m_pImmediateContext->PSSetShader(m_visualiseDepth.Get(), nullptr, 0);

	m_pImmediateContext->PSSetShaderResources(3, 1, m_gBuffer.depth.srv.GetAddressOf());

	m_pImmediateContext->OMSetRenderTargets(1, m_gBuffer.depth.rtv.GetAddressOf(), nullptr);

	m_pImmediateContext->PSSetShaderResources(0, 1, g_SceneShaderResourceView.GetAddressOf());
	m_pImmediateContext->IASetInputLayout(g_pQuadLayout.Get());
	m_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	UINT stride = sizeof(SCREEN_VERTEX);
	UINT offset = 0;
	ID3D11Buffer* vb = g_pScreenQuadVB.Get();
	m_pImmediateContext->IASetVertexBuffers(0, 1, &vb, &stride, &offset);

	m_pImmediateContext->VSSetShader(g_pQuadVS.Get(), nullptr, 0);
	ID3D11SamplerState* ss = m_textureSampler.Get();
	m_pImmediateContext->PSSetSamplers(0, 1, &ss);

	m_pImmediateContext->Draw(4, 0);

	ID3D11ShaderResourceView* nullSRV[] = { nullptr };
	m_pImmediateContext->PSSetShaderResources(3, 1, nullSRV);
}

void DX11Renderer::SetSceneDrawData()
{
	m_pImmediateContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	m_pImmediateContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
	m_pImmediateContext->IASetInputLayout(m_pVertexLayout.Get());
}

void DX11Renderer::Update(const float deltaTime)
{
	UpdateKeyInputs();
	m_imguiRenderer->ResetAllWindowsPositions();

	// ----- FPS calculation -----
	static float timer = 0;
	timer += deltaTime;
	static unsigned int frameCounter = 0;
	frameCounter++;
	static unsigned int FPS = 0;
	if (timer > 1.0f)
	{
		timer -= 1.0f;
		FPS = frameCounter;
		frameCounter = 0;
	}

	float clearColor[4] = { 0.f, 0.f, 0.f, 1.f };

	m_pImmediateContext->UpdateSubresource(m_postProcessConstantBuffer.Get(), 0, nullptr, &m_postProcessCBData, 0, 0);

	if (m_imguiRenderer->m_deferredRendering)
	{
		//FIRST GEOMETRY PASS TO FILL SMALLER G-BUFFER (DEPTH, NORMALS, WORLD POS)
		//  Geometry Pass
		ID3D11RenderTargetView* rtvs[] =
		{
			m_gBuffer.normals.rtv.Get(),
			 m_gBuffer.worldPos.rtv.Get(),
			m_gBuffer.depth.rtv.Get()
		};

		m_pImmediateContext->OMSetRenderTargets(3, rtvs, m_gBuffer.depth.m_pDepthStencilView.Get());
		m_pImmediateContext->ClearRenderTargetView(m_gBuffer.normals.rtv.Get(), clearColor);
		m_pImmediateContext->ClearRenderTargetView(m_gBuffer.worldPos.rtv.Get(), clearColor);
		m_pImmediateContext->ClearRenderTargetView(m_gBuffer.depth.rtv.Get(), clearColor);
		m_pImmediateContext->ClearDepthStencilView(m_gBuffer.depth.m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

		m_pScene->Update(deltaTime);
		m_pImmediateContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
		m_pImmediateContext->PSSetShader(m_WriteGBuffer.Get(), nullptr, 0); // Normal Write // Depth Write // World Pos Write
		m_pImmediateContext->IASetInputLayout(m_pVertexLayout.Get());;
		m_pScene->Draw();

		m_pImmediateContext->OMSetRenderTargets(0, nullptr, nullptr);
		//////

		DrawDepthScreenQuad();

		//////////////

		//SECOND PASS TO FILL FULL SCREEN RENDER TARGET WITH LIGHTING
		m_pImmediateContext->OMSetRenderTargets(1, m_gBuffer.lightAccumulation.rtv.GetAddressOf(), nullptr);
		m_pImmediateContext->ClearRenderTargetView(m_gBuffer.lightAccumulation.rtv.Get(), clearColor);

		DrawFullScreenQuad(FullScreenQuadOptions::LightingQuad);
		//////////////
		m_pImmediateContext->OMSetRenderTargets(0, nullptr, nullptr);

		// THIRD PASS TO DRAW FULL SCREEN QUAD AND SAMPLE FROM G-BUFFER TO APPLY LIGHTING
		// Full Screen Quad / Post Processing Pass / Sampling Pass

		m_pImmediateContext->OMSetRenderTargets(1, m_gBuffer.albedo.rtv.GetAddressOf(), m_gBuffer.depth.m_pDepthStencilView.Get());
		m_pImmediateContext->ClearRenderTargetView(m_gBuffer.albedo.rtv.Get(), clearColor);
		m_pImmediateContext->ClearDepthStencilView(m_gBuffer.depth.m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

		m_pImmediateContext->IASetInputLayout(m_pVertexLayout.Get());;
		m_pImmediateContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);

		m_pImmediateContext->PSSetShader(m_WriteAlbedo.Get(), nullptr, 0);

		m_pScene->Draw();

		m_pImmediateContext->OMSetRenderTargets(0, nullptr, nullptr);

		m_pImmediateContext->OMSetRenderTargets(1, m_PresentedRenderTargetView.GetAddressOf(), nullptr);
		DrawFullScreenQuad(FullScreenQuadOptions::FullDeferredQuad);
	}
	else
	{
		m_pImmediateContext->OMSetRenderTargets(1, g_SceneRenderTargetView.GetAddressOf(), m_gBuffer.depth.m_pDepthStencilView.Get());
		m_pImmediateContext->ClearRenderTargetView(g_SceneRenderTargetView.Get(), clearColor);
		m_pImmediateContext->ClearDepthStencilView(m_gBuffer.depth.m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
		m_pScene->Update(deltaTime);
		SetSceneDrawData();
		m_pScene->Draw();
		m_pImmediateContext->OMSetRenderTargets(0, nullptr, nullptr);
		m_pImmediateContext->OMSetRenderTargets(1, m_PresentedRenderTargetView.GetAddressOf(), nullptr);
		DrawFullScreenQuad(FullScreenQuadOptions::ForwardRenderingQuad);

	}
	// ImGui Pass
	m_imguiRenderer->ImGuiDrawAllWindows(FPS, m_totalTime, m_pScene, m_pImmediateContext.Get());

	if (m_imguiRenderer->VSyncEnabled)
	{
		m_pSwapChain->Present(1, 0);
	}
	else
	{
		m_pSwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
	}
}