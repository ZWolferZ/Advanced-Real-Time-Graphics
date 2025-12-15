#include "Scene.h"

#include <iostream>
#include <unordered_map>

#include "DDSTextureLoader.h"
#include "WaveFrontReader.h"

HRESULT Scene::Init(HWND hwnd, const Microsoft::WRL::ComPtr<ID3D11Device>& device, const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context)
{
	m_pd3dDevice = device;
	m_pImmediateContext = context;
	LoadTextures();
	LoadModels();
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

void Scene::LoadModels()
{
	MeshData cube = InitCubeMesh(m_pd3dDevice.Get(), m_pImmediateContext.Get());

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
	// CREATE A SIMPLE game object
	GameObject* go = new GameObject(XMFLOAT3(0, 0, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1), "Cube 1", GetModelData("Cube"), m_pd3dDevice.Get(), m_pImmediateContext.Get(), GetPixelShader("Texture Pixel Shader"), GetTexture(m_textureMap, "stone.dds"), GetTexture(m_normalMapTextureMap, "conenormal.dds"));
	// CREATE A SIMPLE game object
	GameObject* go2 = new GameObject(XMFLOAT3(7.6, -1.3, -7.1), XMFLOAT3(0, -31, 0), XMFLOAT3(2, 2, 2), "Asha", GetModelData("asha.obj"), m_pd3dDevice.Get(), m_pImmediateContext.Get(), GetPixelShader("Texture Pixel Shader"), GetTexture(m_textureMap, "AshaTex.dds"));

	GameObject* go3 = new GameObject(XMFLOAT3(-8, -1.4, -8.4), XMFLOAT3(0, -47, 0), XMFLOAT3(10, 10, 10), "Bunny", GetModelData("bunny.obj"), m_pd3dDevice.Get(), m_pImmediateContext.Get(), GetPixelShader("Texture Pixel Shader"), GetTexture(m_textureMap, "BunnyTex.dds"));

	// CREATE A SIMPLE game object
	GameObject* go4 = new GameObject(XMFLOAT3(0, -1.5, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(10, 0.1, 10), "Floor 1", GetModelData("Cube"), m_pd3dDevice.Get(), m_pImmediateContext.Get(), GetPixelShader("Texture Pixel Shader"), GetTexture(m_textureMap, "Pathway.dds"), GetTexture(m_normalMapTextureMap, "PathwayNormal.dds"));

	GameObject* go5 = new GameObject(XMFLOAT3(0, 0, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(-50, -50, -50), "Skybox", GetModelData("Cube"), m_pd3dDevice.Get(), m_pImmediateContext.Get(), GetPixelShader("Texture UnLit Pixel Shader"), GetTexture(m_textureMap, "Stars.dds"));

	go->m_autoRotateX = true;
	go->m_autoRotateY = true;

	go5->m_autoRotateX = true;
	go5->m_autoRotateY = true;
	go5->m_autoRotationSpeed = 1.5f;

	m_vecDrawables.push_back(go);
	m_vecDrawables.push_back(go2);
	m_vecDrawables.push_back(go3);
	m_vecDrawables.push_back(go4);
	m_vecDrawables.push_back(go5);
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

MeshData Scene::InitCubeMesh(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pContext)
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
	//for (unsigned int i = 0; i < MAX_LIGHTS; i++)
	//{
	//	Light light;
	//	light.Enabled = static_cast<int>(true);
	//	light.LightType = PointLight;
	//	light.Color = XMFLOAT4(0.0f, 0.765f, 1, 1);
	//	light.SpotAngle = XMConvertToRadians(45.0f);
	//	light.ConstantAttenuation = 1.0f;
	//	light.LinearAttenuation = 1.0f;
	//	light.QuadraticAttenuation = 1;

	//	// set up the light
	//	XMFLOAT4 LightPosition(0, 0, 1.2f, 1);

	//	if (i == 1)
	//	{
	//		light.Color = { 1.00f,0.00f,0.559f,1.0f };
	//		LightPosition = { 3,0,1.5f,1 };
	//	}

	//	light.Position = LightPosition;
	//	m_lightProperties.Lights[i] = light;
	//}

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
	m_pImmediateContext->PSSetConstantBuffers(2, 1, &cb);

	ID3D11ShaderResourceView* srv = m_lightSRV.Get();
	m_pImmediateContext->PSSetShaderResources(2, 1, &srv);
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
	//static bool moveLightRight = true;

	//if (m_lights[4].Position.x >= 5.0f)
	//{
	//	moveLightRight = false;
	//}
	//else if (m_lights[4].Position.x <= -5.0f)
	//{
	//	moveLightRight = true;
	//}

	//if (moveLightRight)
	//{
	//	m_lights[4].Position.x += 2 * deltaTime;
	//}
	//else
	//{
	//	m_lights[4].Position.x -= 2 * deltaTime;
	//}

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
		m_vecDrawables[i]->Update(deltaTime, m_pImmediateContext.Get());
	}
}

void Scene::Draw()
{
	for (unsigned int i = 0; i < m_vecDrawables.size(); i++)
	{
		m_vecDrawables[i]->Draw(m_pImmediateContext.Get(), GetCamera(), m_pConstantBuffer.Get());
	}
}