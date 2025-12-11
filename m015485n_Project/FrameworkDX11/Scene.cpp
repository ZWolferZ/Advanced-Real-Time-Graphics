#include "Scene.h"

HRESULT Scene::init(HWND hwnd, const Microsoft::WRL::ComPtr<ID3D11Device>& device, const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context)
{
	m_pd3dDevice = device;
	m_pImmediateContext = context;

	CreateGameObjects();

	RECT rc;
	GetClientRect(hwnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	m_pCamera = new Camera(XMFLOAT3(0, 0, 4), XMFLOAT3(0, 0, -1), XMFLOAT3(0.0f, 1.0f, 0.0f), width, height);

	// Create the constant buffer
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	HRESULT	hr = m_pd3dDevice->CreateBuffer(&bd, nullptr, &m_pConstantBuffer);
	if (FAILED(hr))
		return hr;

	setupLightProperties();

	return S_OK;
}

void Scene::CreateGameObjects()
{
	// CREATE A SIMPLE game object
	GameObject* go = new GameObject(XMFLOAT3(-2, 0, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1), "Cube 1", m_vecDrawables, m_pd3dDevice.Get(), m_pImmediateContext.Get(), m_pixelShaders[0]);

	// CREATE A SIMPLE game object
	GameObject* go2 = new GameObject(XMFLOAT3(2, 0, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1), "Cube 2", m_vecDrawables, m_pd3dDevice.Get(), m_pImmediateContext.Get(), m_pixelShaders[1]);
}

void Scene::cleanUp()
{
	for (GameObject* obj : m_vecDrawables)
	{
		obj->cleanup();
		delete obj;
	}

	m_vecDrawables.clear();

	delete m_pCamera;
}

void Scene::setupLightProperties()
{
	for (unsigned int i = 0; i < MAX_LIGHTS; i++)
	{
		Light light;
		light.Enabled = static_cast<int>(true);
		light.LightType = PointLight;
		light.Color = XMFLOAT4(1, 1, 1, 1);
		light.SpotAngle = XMConvertToRadians(45.0f);
		light.ConstantAttenuation = 1.0f;
		light.LinearAttenuation = 1;
		light.QuadraticAttenuation = 1;

		// set up the light
		XMFLOAT4 LightPosition(0, 0, 1.5f, 1);

		if (i == 1)
		{
			LightPosition = { 0,0,-1.5f,1 };
		}

		light.Position = LightPosition;
		m_lightProperties.EyePosition = LightPosition;
		m_lightProperties.Lights[i] = light;
	}

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
}

void Scene::UpdateLightProperties(unsigned int index, const Light& light)
{
	m_lightProperties.Lights[index] = light;
}

void Scene::UpdateLightBuffer()
{
	m_pImmediateContext->UpdateSubresource(m_pLightConstantBuffer.Get(), 0, nullptr, &m_lightProperties, 0, 0);
	ID3D11Buffer* buf = m_pLightConstantBuffer.Get();
	m_pImmediateContext->PSSetConstantBuffers(2, 1, &buf);
}

void Scene::update(const float deltaTime)
{
	UpdateLightBuffer();

	for (unsigned int i = 0; i < m_vecDrawables.size(); i++)
	{
		m_vecDrawables[i]->update(deltaTime, m_pImmediateContext.Get());

		m_vecDrawables[i]->draw(m_pImmediateContext.Get(), getCamera(), m_pConstantBuffer.Get());
	}
}