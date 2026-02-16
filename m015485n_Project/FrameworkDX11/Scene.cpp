#include "Scene.h"

HRESULT Scene::Init(HWND hwnd, const Microsoft::WRL::ComPtr<ID3D11Device>& device, const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context)
{
	m_pd3dDevice = device;
	m_pImmediateContext = context;

	std::thread DDSThread(&Scene::LoadDDSs, this);
	std::thread ModelThread(&Scene::LoadModels, this);

	DDSThread.join();
	ModelThread.join();
	CreateGameObjects();

	RECT rc;
	GetClientRect(hwnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	m_pCamera = new Camera(XMFLOAT3(0, 3, 4.5), XMFLOAT3(0, -0.65, -1), XMFLOAT3(0.0f, 1.0f, 0.0f), width, height);

	// Create the constant buffer
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	HRESULT	hr = m_pd3dDevice->CreateBuffer(&bd, nullptr, &m_pConstantBuffer);
	if (FAILED(hr))
		return hr;

	SetupLightProperties();

	return S_OK;
}

void Scene::LoadTextures()
{
	for (const auto& entry : filesystem::directory_iterator(L"resources\\Textures"))
	{
		if (!entry.is_regular_file()) continue;

		if (entry.path().extension() != L".dds") continue;

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureResourceView;

		HRESULT hr = CreateDDSTextureFromFile(m_pd3dDevice.Get(), entry.path().wstring().c_str(), nullptr, textureResourceView.GetAddressOf());

		if (FAILED(hr))
		{
			MessageBox(nullptr, L"Failed to load texture", L"Error", MB_OK);
			continue;
		}

		m_textureMap.push_back({ entry.path().filename().string(), textureResourceView });
	}
}

void Scene::LoadNormalMaps()
{
	for (const auto& entry : filesystem::directory_iterator(L"resources\\NormalMaps"))
	{
		if (!entry.is_regular_file()) continue;

		if (entry.path().extension() != L".dds") continue;

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normalMapResourceView;

		HRESULT hr = CreateDDSTextureFromFile(m_pd3dDevice.Get(), entry.path().wstring().c_str(), nullptr, normalMapResourceView.GetAddressOf());

		if (FAILED(hr))
		{
			MessageBox(nullptr, L"Failed to load normal map texture", L"Error", MB_OK);
			continue;
		}

		m_normalMapTextureMap.push_back({ entry.path().filename().string(), normalMapResourceView });
	}
}

void Scene::LoadDDSs()
{
	std::thread texThread(&Scene::LoadTextures, this);
	std::thread normThread(&Scene::LoadNormalMaps, this);

	texThread.join();
	normThread.join();
}

void Scene::LoadModels()
{
	MeshData cube = InitCubeMesh(m_pd3dDevice.Get());

	m_models.push_back({ "Cube", cube });

	for (const auto& entry : filesystem::directory_iterator(L"resources\\Models"))
	{
		if (!entry.is_regular_file()) continue;

		if (entry.path().extension() != L".obj") continue;

		MeshData obj = LoadOBJMesh(m_pd3dDevice.Get(), entry.path().string());

		m_models.push_back({ entry.path().filename().string(), obj });
	}
}

void Scene::CreateGameObjects()
{
	GameObject* NormalMapCube1 = new GameObject(XMFLOAT3(2.0f, 0, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1), "Normal Mapped Cube 1", GetModelData("Cube"), m_pd3dDevice.Get(), m_pImmediateContext.Get(), GetPixelShader("Texture Pixel Shader"), GetTexture(m_textureMap, "stone.dds"), "stone.dds", GetTexture(m_normalMapTextureMap, "conenormal.dds"));

	GameObject* NormalMapCube2 = new GameObject(XMFLOAT3(-2.0f, 0, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1), "Normal Mapped Cube 2", GetModelData("Cube"), m_pd3dDevice.Get(), m_pImmediateContext.Get(), GetPixelShader("Texture Pixel Shader"), GetTexture(m_textureMap, "Crate_COLOR.dds"), "Crate_COLOR.dds", GetTexture(m_normalMapTextureMap, "Crate_NRM.dds"));

	GameObject* NormalMapCube3 = new GameObject(XMFLOAT3(7.7, 0, -7.7), XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1), "Normal Mapped Cube 3", GetModelData("Cube"), m_pd3dDevice.Get(), m_pImmediateContext.Get(), GetPixelShader("Texture Pixel Shader"), GetTexture(m_textureMap, "Glass.dds"), "Glass.dds", GetTexture(m_normalMapTextureMap, "CircleMap.dds"));

	GameObject* NormalMapCube4 = new GameObject(XMFLOAT3(-7.7, 0, -7.7), XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1), "Normal Mapped Cube 4", GetModelData("Cube"), m_pd3dDevice.Get(), m_pImmediateContext.Get(), GetPixelShader("Texture Pixel Shader"), GetTexture(m_textureMap, "MetalTex.dds"), "MetalTex.dds", GetTexture(m_normalMapTextureMap, "Metal.dds"));

	GameObject* Obj1 = new GameObject(XMFLOAT3(7.6 + 200, -1.3, -7.1), XMFLOAT3(0, -31, 0), XMFLOAT3(2, 2, 2), "Asha", GetModelData("asha.obj"), m_pd3dDevice.Get(), m_pImmediateContext.Get(), GetPixelShader("Texture Pixel Shader"), GetTexture(m_textureMap, "AshaTex.dds"), "AshaTex.dds");

	GameObject* Obj2 = new GameObject(XMFLOAT3(-8 + 200, -1.4, -8.4), XMFLOAT3(0, -47, 0), XMFLOAT3(10, 10, 10), "Bunny", GetModelData("bunny.obj"), m_pd3dDevice.Get(), m_pImmediateContext.Get(), GetPixelShader("Texture Pixel Shader"), GetTexture(m_textureMap, "BunnyTex.dds"), "BunnyTex.dds");

	GameObject* Floor1 = new GameObject(XMFLOAT3(0, -1.5, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(10, 0.1, 10), "Floor 1", GetModelData("Cube"), m_pd3dDevice.Get(), m_pImmediateContext.Get(), GetPixelShader("Texture Pixel Shader"), GetTexture(m_textureMap, "Depth View Texture"), "Depth View Texture");

	GameObject* Floor2 = new GameObject(XMFLOAT3(200, -1.5, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(10, 0.1, 10), "Floor 2", GetModelData("Cube"), m_pd3dDevice.Get(), m_pImmediateContext.Get(), GetPixelShader("Texture Pixel Shader"), GetTexture(m_textureMap, "Bunny.dds"), "Bunny.dds");

	GameObject* Floor3 = new GameObject(XMFLOAT3(-200, -1.5, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(10, 0.1, 10), "Floor 3", GetModelData("Cube"), m_pd3dDevice.Get(), m_pImmediateContext.Get(), GetPixelShader("Texture Pixel Shader"), GetTexture(m_textureMap, "Glass.dds"), "Glass.dds");

	GameObject* messageboxNormalMapping = new GameObject(XMFLOAT3(0.0f, 2, 0), XMFLOAT3(0, 180, 0), XMFLOAT3(1, 1, 1), "Normal Mapping Message", GetModelData("SpeechBubble.obj"), m_pd3dDevice.Get(), m_pImmediateContext.Get(), GetPixelShader("Texture Pixel Shader"), "resources\\Audio\\Normal Mapping Message.wav");

	GameObject* messageboxPostProcessing = new GameObject(XMFLOAT3(200, 2, 0), XMFLOAT3(0, 180, 0), XMFLOAT3(1, 1, 1), "Post Processing Message", GetModelData("SpeechBubble.obj"), m_pd3dDevice.Get(), m_pImmediateContext.Get(), GetPixelShader("Texture Pixel Shader"), "resources\\Audio\\Post Processing Message.wav");

	GameObject* messageboxDeferrredLighting = new GameObject(XMFLOAT3(-195, 3.8, 0), XMFLOAT3(0, 180, 0), XMFLOAT3(1, 1, 1), "Deferred Lighting Message", GetModelData("SpeechBubble.obj"), m_pd3dDevice.Get(), m_pImmediateContext.Get(), GetPixelShader("Texture Pixel Shader"), "resources\\Audio\\Deferred Lighting Message.wav");

	GameObject* CassetteTV = new GameObject(XMFLOAT3(200, -1.3, -7.7), XMFLOAT3(-90, 0, 0), XMFLOAT3(.1, .1, .1), "CassetteTV", GetModelData("TV.obj"), m_pd3dDevice.Get(), m_pImmediateContext.Get(), GetPixelShader("Texture Pixel Shader"), GetTexture(m_textureMap, "TV.dds"), "TV.dds");

	GameObject* CassetteImage = new GameObject(XMFLOAT3(200, 2.643, -7.623), XMFLOAT3(0, 0, 0), XMFLOAT3(4.4, 2.7, 0.04), "CassetteImage", GetModelData("Cube"), m_pd3dDevice.Get(), m_pImmediateContext.Get(), GetPixelShader("Texture Pixel Shader"), GetTexture(m_textureMap, "Cassette.dds"), "Cassette.dds");

	GameObject* skybox = new GameObject(XMFLOAT3(0, 0, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(50, 50, 50), "Skybox", GetModelData("skybox.obj"), m_pd3dDevice.Get(), m_pImmediateContext.Get(), GetPixelShader("Texture UnLit Pixel Shader"), GetTexture(m_textureMap, "Stars.dds"), "Stars.dds");

	GameObject* DepthTV = new GameObject(XMFLOAT3(-193, -1.3, -7.7), XMFLOAT3(-90, -31, 0), XMFLOAT3(.05, .05, .05), "DepthTV", GetModelData("TV.obj"), m_pd3dDevice.Get(), m_pImmediateContext.Get(), GetPixelShader("Texture Pixel Shader"), GetTexture(m_textureMap, "TV.dds"), "TV.dds");

	GameObject* DepthImage = new GameObject(XMFLOAT3(-192.965, 0.766, -7.665), XMFLOAT3(0, -31, 0), XMFLOAT3(2.181, 1.35, 0.04), "DepthImage", GetModelData("Cube"), m_pd3dDevice.Get(), m_pImmediateContext.Get(), GetPixelShader("Texture Pixel Shader"), GetTexture(m_textureMap, "Depth View Texture"), "Depth View Texture");

	GameObject* WorldNormalTV = new GameObject(XMFLOAT3(-197.9, -1.3, -9.1), XMFLOAT3(-90, 0, 0), XMFLOAT3(.05, .05, .05), "WorldNormalTV", GetModelData("TV.obj"), m_pd3dDevice.Get(), m_pImmediateContext.Get(), GetPixelShader("Texture Pixel Shader"), GetTexture(m_textureMap, "TV.dds"), "TV.dds");

	GameObject* WorldNormalImage = new GameObject(XMFLOAT3(-197.883, 0.766, -9.088), XMFLOAT3(0, 0, 0), XMFLOAT3(2.181, 1.35, 0.04), "WorldNormalImage", GetModelData("Cube"), m_pd3dDevice.Get(), m_pImmediateContext.Get(), GetPixelShader("Texture Pixel Shader"), GetTexture(m_textureMap, "Normal View Texture"), "Normal View Texture");

	GameObject* WorldPositionTV = new GameObject(XMFLOAT3(-203.2, -1.3, -9.1), XMFLOAT3(-90, 0, 0), XMFLOAT3(.05, .05, .05), "WorldPositionTV", GetModelData("TV.obj"), m_pd3dDevice.Get(), m_pImmediateContext.Get(), GetPixelShader("Texture Pixel Shader"), GetTexture(m_textureMap, "TV.dds"), "TV.dds");

	GameObject* WorldPositionImage = new GameObject(XMFLOAT3(-203.198, 0.766, -9.088), XMFLOAT3(0, 0, 0), XMFLOAT3(2.181, 1.35, 0.04), "WorldPositionImage", GetModelData("Cube"), m_pd3dDevice.Get(), m_pImmediateContext.Get(), GetPixelShader("Texture Pixel Shader"), GetTexture(m_textureMap, "World Pos View Texture"), "World Pos View Texture");

	GameObject* LightAccTV = new GameObject(XMFLOAT3(-208.2, -1.3, -7.7), XMFLOAT3(-90, 31, 0), XMFLOAT3(.05, .05, .05), "LightAccTV", GetModelData("TV.obj"), m_pd3dDevice.Get(), m_pImmediateContext.Get(), GetPixelShader("Texture Pixel Shader"), GetTexture(m_textureMap, "TV.dds"), "TV.dds");

	GameObject* LightAccImage = new GameObject(XMFLOAT3(-208.195, 0.766, -7.6), XMFLOAT3(0, 31, 0), XMFLOAT3(2.181, 1.35, 0.04), "LightAccImage", GetModelData("Cube"), m_pd3dDevice.Get(), m_pImmediateContext.Get(), GetPixelShader("Texture Pixel Shader"), GetTexture(m_textureMap, "Light Accumulation View Texture"), "Light Accumulation View Texture");

	NormalMapCube1->m_autoRotateX = true;
	NormalMapCube1->m_autoRotateY = true;
	NormalMapCube2->m_autoRotateX = true;
	NormalMapCube2->m_autoRotateY = true;
	NormalMapCube3->m_autoRotateX = true;
	NormalMapCube3->m_autoRotateY = true;
	NormalMapCube4->m_autoRotateX = true;
	NormalMapCube4->m_autoRotateY = true;
	skybox->m_autoRotateX = true;
	skybox->m_autoRotateY = true;
	skybox->m_autoRotationSpeed = 1.5f;

	m_vecDrawables.push_back(NormalMapCube1);
	m_vecDrawables.push_back(NormalMapCube2);
	m_vecDrawables.push_back(NormalMapCube3);
	m_vecDrawables.push_back(NormalMapCube4);

	m_vecDrawables.push_back(Obj1);
	m_vecDrawables.push_back(Obj2);
	m_vecDrawables.push_back(Floor1);
	m_vecDrawables.push_back(Floor2);
	m_vecDrawables.push_back(Floor3);
	m_vecDrawables.push_back(messageboxNormalMapping);
	m_vecDrawables.push_back(messageboxPostProcessing);
	m_vecDrawables.push_back(messageboxDeferrredLighting);
	m_vecDrawables.push_back(CassetteTV);
	m_vecDrawables.push_back(CassetteImage);
	m_vecDrawables.push_back(DepthImage);
	m_vecDrawables.push_back(DepthTV);
	m_vecDrawables.push_back(WorldNormalTV);
	m_vecDrawables.push_back(WorldNormalImage);

	m_vecDrawables.push_back(WorldPositionTV);
	m_vecDrawables.push_back(WorldPositionImage);
	m_vecDrawables.push_back(LightAccTV);
	m_vecDrawables.push_back(LightAccImage);

	m_vecDrawables.push_back(skybox); // Last thing to be pushed back must be skybox
}

void Scene::CleanUp()
{
	for (GameObject* obj : m_vecDrawables)
	{
		obj->Cleanup();
		delete obj;
	}

	m_vecDrawables.clear();

	delete m_pCamera;
}

MeshData Scene::GetModelData(const string& modelToFind)
{
	for (auto& modelPair : m_models)
	{
		if (modelPair.first == modelToFind)
		{
			return modelPair.second;
		}
	}
	return m_models[0].second;
}

MeshData Scene::InitCubeMesh(ID3D11Device* pd3dDevice)
{
	MeshData meshData;

	// Create index buffer
	WORD indices[] =
	{
		0,1,2,
		3,4,5,

		6,7,8,
		9,10,11,

		12,13,14,
		15,16,17,

		18,19,20,
		21,22,23,

		24,25,26,
		27,28,29,

		30,31,32,
		33,34,35
	};

	meshData.VertexCount = 36;
	// Create vertex buffer
	SimpleVertex vertices[] =
	{
		// top
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) }, // 3 // 0
		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) }, // 1 // 1
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) }, // 0 // 2

		{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) }, // 2 // 3
		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) }, // 1 // 4
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) }, // 3 // 5

		// bottom
		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) }, // 6 // 6
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) }, // 4 // 7
		{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) }, // 5 // 8

		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) }, // 7 // 9
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) }, // 4 // 10
		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) }, // 6 // 11

		// left
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) }, // 11 // 12
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) }, // 9 // 13
		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) }, // 8 // 14

		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) }, // 10 // 15
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) }, // 9 // 16
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) }, // 11 // 17

		// right
		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) }, // 14 // 18
		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) }, // 12 // 19
		{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) }, // 13 // 20

		{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) }, // 15 // 21
		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) }, // 12 // 22
		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) }, // 14 // 23

		// front
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) }, // 19 // 24
		{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) , XMFLOAT2(1.0f, 1.0f) }, // 17 // 25
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 1.0f) }, // 16 // 26

		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) }, // 18 // 27
		{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) , XMFLOAT2(1.0f, 1.0f) }, // 17 // 28
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) }, // 19 // 29

		// back
		{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) }, // 22
		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) }, // 20
		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) }, // 21

		{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 0.0f) }, // 23
		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) }, // 20
		{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) }, // 22
	};

	CalculateModelVectorsNoSharedVertices(vertices, 36);

	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 36;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData = {};
	InitData.pSysMem = vertices;
	Microsoft::WRL::ComPtr <ID3D11Buffer>* vbuf = &meshData.VertexBuffer;
	HRESULT hr = pd3dDevice->CreateBuffer(&bd, &InitData, vbuf->GetAddressOf());
	if (FAILED(hr))
	{
		MessageBox(nullptr, L"Failed to init Cube VBuffer in Cube Mesh Loading", L"Error", MB_OK);
	}
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(WORD) * 36;        // 36 vertices needed for 12 triangles in a triangle list
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData.pSysMem = indices;
	Microsoft::WRL::ComPtr <ID3D11Buffer>* ibuf = &meshData.IndexBuffer;
	hr = pd3dDevice->CreateBuffer(&bd, &InitData, ibuf->GetAddressOf());
	if (FAILED(hr))
	{
		MessageBox(nullptr, L"Failed to init Cube IBuffer in Cube Mesh Loading", L"Error", MB_OK);
	}

	meshData.VBStride = sizeof(SimpleVertex);
	meshData.VBOffset = 0;

	return meshData;
}

MeshData Scene::LoadOBJMesh(ID3D11Device* pd3dDevice, const std::string& filename)
{
	MeshData meshData;

	std::wstring wFilename(filename.begin(), filename.end());

	DX::WaveFrontReader<WORD> objReader;
	HRESULT hr = objReader.Load(wFilename.c_str());
	if (FAILED(hr))
	{
		MessageBox(nullptr, L"Failed to load OBJ file", L"Error", MB_OK);
		return meshData;
	}

	size_t vertexCount = objReader.vertices.size();
	size_t indexCount = objReader.indices.size();
	meshData.VertexCount = static_cast<UINT>(indexCount);

	std::vector<SimpleVertex> vertices(vertexCount);
	for (size_t i = 0; i < vertexCount; ++i)
	{
		const auto& v = objReader.vertices[i];
		vertices[i].Pos = v.position;
		vertices[i].Normal = v.normal;
		vertices[i].TexCoord.x = v.textureCoordinate.x;
		vertices[i].TexCoord.y = 1.0f - v.textureCoordinate.y;
	}

	std::vector<WORD> indices(indexCount);
	for (size_t i = 0; i < indexCount; ++i)
		indices[i] = objReader.indices[i];

	CalculateModelVectorsSharedVertices(vertices, indices);

	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = static_cast<UINT>(sizeof(SimpleVertex) * vertexCount);
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData = {};
	InitData.pSysMem = vertices.data();

	hr = pd3dDevice->CreateBuffer(&bd, &InitData, meshData.VertexBuffer.GetAddressOf());
	if (FAILED(hr))
	{
		MessageBox(nullptr, L"Failed to create vertex buffer for OBJ mesh", L"Error", MB_OK);
		return meshData;
	}

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = static_cast<UINT>(sizeof(WORD) * indexCount);
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData.pSysMem = indices.data();

	hr = pd3dDevice->CreateBuffer(&bd, &InitData, meshData.IndexBuffer.GetAddressOf());
	if (FAILED(hr))
	{
		MessageBox(nullptr, L"Failed to create index buffer for OBJ mesh", L"Error", MB_OK);
		return meshData;
	}

	meshData.VBStride = sizeof(SimpleVertex);
	meshData.VBOffset = 0;

	return meshData;
}

Microsoft::WRL::ComPtr<ID3D11PixelShader>& Scene::GetPixelShader(const string& shaderToFind)
{
	for (auto& shaderPair : m_pixelShadersMap)
	{
		if (shaderPair.first == shaderToFind)
		{
			return shaderPair.second;
		}
	}
	return m_pixelShadersMap[0].second;
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& Scene::GetTexture(vector<std::pair<string, Microsoft::WRL::ComPtr < ID3D11ShaderResourceView>>>& mapToCheck, const string& textureToFind)
{
	for (auto& texturePair : mapToCheck)
	{
		if (texturePair.first == textureToFind)
		{
			return texturePair.second;
		}
	}

	return m_textureMap[0].second;
}

void Scene::SetupLightProperties()
{
	{
		Light light0;
		light0.Enabled = static_cast<int>(true);
		light0.LightType = SpotLight;
		light0.Color = XMFLOAT4(0.0f, 0.765f, 1, 1);
		light0.SpotAngle = XMConvertToRadians(46.0f);
		light0.ConstantAttenuation = .1f;
		light0.LinearAttenuation = .1f;
		light0.QuadraticAttenuation = 0.1f;
		light0.Direction = { 0.0f,-1.0f,-0.65f,1 };
		light0.Position = { 7.7f, 4.0f,-4.2f,1 };
		m_lights.push_back(light0);
	}

	{
		Light light1;
		light1.Enabled = static_cast<int>(true);
		light1.LightType = SpotLight;
		light1.Color = { 1.00f,0.00f,0.559f,1.0f };
		light1.SpotAngle = XMConvertToRadians(45.0f);
		light1.ConstantAttenuation = .1f;
		light1.LinearAttenuation = .1f;
		light1.QuadraticAttenuation = 0.1f;
		light1.Direction = { 0.0f,-1.0f,-0.65f,1 };
		light1.Position = { -7.6f, 4.0f,-4.2f,1 };
		m_lights.push_back(light1);
	}
	{
		Light light2;
		light2.Enabled = static_cast<int>(true);
		light2.LightType = PointLight;
		light2.Color = XMFLOAT4(0.0f, 0.765f, 1, 1);
		light2.SpotAngle = XMConvertToRadians(45.0f);
		light2.ConstantAttenuation = 1.0f;
		light2.LinearAttenuation = 1.0f;
		light2.QuadraticAttenuation = 1;
		light2.Position = { -7.5f, 0.0f,7.0f,1 };
		m_lights.push_back(light2);
	}

	{
		Light light3;
		light3.Enabled = static_cast<int>(true);
		light3.LightType = PointLight;
		light3.Color = { 1.00f,0.00f,0.559f,1.0f };
		light3.SpotAngle = XMConvertToRadians(45.0f);
		light3.ConstantAttenuation = 1.0f;
		light3.LinearAttenuation = 1.0f;
		light3.QuadraticAttenuation = 1;
		light3.Position = { 7.5f, 0.0f,7.0f,1 };
		m_lights.push_back(light3);
	}

	{
		Light light4;
		light4.Enabled = static_cast<int>(true);
		light4.LightType = SpotLight;
		light4.Color = { 1.00f,1.00f,1.00f,1.0f };
		light4.SpotAngle = XMConvertToRadians(45);
		light4.ConstantAttenuation = .1f;
		light4.LinearAttenuation = .1f;
		light4.QuadraticAttenuation = .1;
		light4.Position = { 0, 4.0f,0,1 };
		light4.Direction = { 0,-1,0,0 };
		m_lights.push_back(light4);
	}

	{
		Light light5;
		light5.Enabled = static_cast<int>(true);
		light5.LightType = PointLight;
		light5.Color = { 1.00f,0.559,0,1.0f };
		light5.SpotAngle = XMConvertToRadians(45);
		light5.ConstantAttenuation = .1f;
		light5.LinearAttenuation = .1f;
		light5.QuadraticAttenuation = .1;
		light5.Position = { 200, 2,0,1 };
		light5.Direction = { 0,-1,0,0 };
		m_lights.push_back(light5);
	}

	{
		Light light6;
		light6.Enabled = static_cast<int>(true);
		light6.LightType = PointLight;
		light6.Color = { 1.00f,0.559,0,1.0f };
		light6.SpotAngle = XMConvertToRadians(45);
		light6.ConstantAttenuation = .1f;
		light6.LinearAttenuation = .1f;
		light6.QuadraticAttenuation = .1;
		light6.Position = { 206, 4,-5.4,1 };
		light6.Direction = { 0,-1,0,0 };
		m_lights.push_back(light6);
	}

	{
		Light light7;
		light7.Enabled = static_cast<int>(true);
		light7.LightType = PointLight;
		light7.Color = { 1.00f,0.559,0,1.0f };
		light7.SpotAngle = XMConvertToRadians(45);
		light7.ConstantAttenuation = .1f;
		light7.LinearAttenuation = .1f;
		light7.QuadraticAttenuation = .1;
		light7.Position = { 193, 3.2,-5.4,1 };
		light7.Direction = { 0,-1,0,0 };
		m_lights.push_back(light7);
	}

	{
		Light light8;
		light8.Enabled = static_cast<int>(true);
		light8.LightType = SpotLight;
		light8.Color = { 1.00f,1,1,1.0f };
		light8.SpotAngle = XMConvertToRadians(0);
		light8.ConstantAttenuation = .1f;
		light8.LinearAttenuation = .1f;
		light8.QuadraticAttenuation = .1;
		light8.Position = { -195, 2,-4.2,1 };
		light8.Direction = { 0,-1,0,0 };
		m_lights.push_back(light8);
	}

	{
		Light light9;
		light9.Enabled = static_cast<int>(true);
		light9.LightType = SpotLight;
		light9.Color = { 1.00f,1,1,1.0f };
		light9.SpotAngle = XMConvertToRadians(0);
		light9.ConstantAttenuation = .1f;
		light9.LinearAttenuation = .1f;
		light9.QuadraticAttenuation = .1;
		light9.Position = { -200, 2,-4.0,1 };
		light9.Direction = { 0,-1,0,0 };
		m_lights.push_back(light9);
	}

	{
		Light light10;
		light10.Enabled = static_cast<int>(true);
		light10.LightType = SpotLight;
		light10.Color = { 1.00f,1,1,1.0f };
		light10.SpotAngle = XMConvertToRadians(0);
		light10.ConstantAttenuation = .1f;
		light10.LinearAttenuation = .1f;
		light10.QuadraticAttenuation = .1;
		light10.Position = { -204, 2,-4.0,1 };
		light10.Direction = { 0,-1,0,0 };
		m_lights.push_back(light10);
	}

	{
		Light light11;
		light11.Enabled = static_cast<int>(true);
		light11.LightType = SpotLight;
		light11.Color = { 1.00f,1,1,1.0f };
		light11.SpotAngle = XMConvertToRadians(0);
		light11.ConstantAttenuation = .1f;
		light11.LinearAttenuation = .1f;
		light11.QuadraticAttenuation = .1;
		light11.Position = { -200, 2,2,1 };
		light11.Direction = { 0,-1,0,0 };
		m_lights.push_back(light11);
	}

	m_lightProperties.EyePosition = XMFLOAT4(GetCamera()->GetPosition().x, GetCamera()->GetPosition().y, GetCamera()->GetPosition().z, 1);

	D3D11_BUFFER_DESC bd = {};
	// Create the light constant buffer
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(LightPropertiesConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	HRESULT hr = m_pd3dDevice->CreateBuffer(&bd, nullptr, &m_pLightConstantBuffer);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"Failed to create lighting buffer in scene.cpp", L"Error", MB_OK);
	}

	D3D11_BUFFER_DESC sbDesc = {};
	sbDesc.Usage = D3D11_USAGE_DYNAMIC;
	sbDesc.ByteWidth = sizeof(Light) * 128; // max you expect
	sbDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	sbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	sbDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	sbDesc.StructureByteStride = sizeof(Light);

	hr = m_pd3dDevice->CreateBuffer(&sbDesc, nullptr, &m_lightStructuredBuffer);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = 128;

	hr = m_pd3dDevice->CreateShaderResourceView(
		m_lightStructuredBuffer.Get(),
		&srvDesc,
		&m_lightSRV
	);
}

void Scene::UpdateLightBuffer()
{
	D3D11_MAPPED_SUBRESOURCE mapped;
	m_pImmediateContext->Map(
		m_lightStructuredBuffer.Get(),
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&mapped
	);

	memcpy(mapped.pData, m_lights.data(), sizeof(Light) * m_lights.size());

	m_pImmediateContext->Unmap(m_lightStructuredBuffer.Get(), 0);

	LightPropertiesConstantBuffer globals = {};
	globals.EyePosition = XMFLOAT4(
		GetCamera()->GetPosition().x,
		GetCamera()->GetPosition().y,
		GetCamera()->GetPosition().z,
		1.0f
	);
	//globals.GlobalAmbient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	globals.LightCount = static_cast<UINT>(m_lights.size());

	m_pImmediateContext->UpdateSubresource(
		m_pLightConstantBuffer.Get(),
		0,
		nullptr,
		&globals,
		0,
		0
	);

	// Bind to PS
	ID3D11Buffer* cb = m_pLightConstantBuffer.Get();
	m_pImmediateContext->PSSetConstantBuffers(9, 1, &cb);

	ID3D11ShaderResourceView* srv = m_lightSRV.Get();
	m_pImmediateContext->PSSetShaderResources(9, 1, &srv);
}

void Scene::AddLight()
{
	Light light;
	light.Enabled = static_cast<int>(true);
	light.LightType = PointLight;
	light.Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1);
	light.SpotAngle = XMConvertToRadians(45.0f);
	light.ConstantAttenuation = 1.0f;
	light.LinearAttenuation = 1.0f;
	light.QuadraticAttenuation = 1;
	light.Position = { 0, 0,0,1 };
	m_lights.push_back(light);
}

void Scene::Update(const float deltaTime)
{
	static bool moveLightRight = true;

	if (m_lights[4].Position.x >= 5.0f)
	{
		moveLightRight = false;
	}
	else if (m_lights[4].Position.x <= -5.0f)
	{
		moveLightRight = true;
	}

	if (moveLightRight)
	{
		m_lights[4].Position.x += 2 * deltaTime;
	}
	else
	{
		m_lights[4].Position.x -= 2 * deltaTime;
	}

	UpdateLightBuffer();

	if (m_playCameraSplineAnimation)
	{
		m_pCamera->CameraSplineAnimation(deltaTime, m_controlPoints, m_totalSplineAnimation);
	}

	for (unsigned int i = 0; i < m_vecDrawables.size(); i++)
	{
		if (m_vecDrawables[i]->GetObjectName() == "Skybox")
		{
			m_vecDrawables[i]->SetPosition(GetCamera()->GetPosition());
		}
		m_vecDrawables[i]->Update(deltaTime);
	}
}

void Scene::Draw(int renderpass, bool deferredRendering, bool showDialogBoxes)
{
	for (unsigned int i = 0; i < m_vecDrawables.size(); i++)
	{
		if (m_vecDrawables[i]->GetObjectName() == "Skybox")
		{
			m_vecDrawables[i]->Draw(m_pImmediateContext.Get(), GetCamera(), m_pConstantBuffer.Get(), true, renderpass);
		}
		else
		{
			if ((m_vecDrawables[i]->m_isDialog && !deferredRendering) || (m_vecDrawables[i]->m_isDialog && !showDialogBoxes))
			{
				continue;
			}
			m_vecDrawables[i]->Draw(m_pImmediateContext.Get(), GetCamera(), m_pConstantBuffer.Get(), false, renderpass);
		}
	}
}