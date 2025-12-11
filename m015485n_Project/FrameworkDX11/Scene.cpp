#include "Scene.h"

HRESULT Scene::init(HWND hwnd, const Microsoft::WRL::ComPtr<ID3D11Device>& device, const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context)
{
	m_pd3dDevice = device;
	m_pImmediateContext = context;

	RECT rc;
	GetClientRect(hwnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	// CREATE A SIMPLE game object
	Cube* go = new Cube(XMFLOAT3(0, 0, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1), "Cube 1");
	HRESULT hr = go->initMesh(m_pd3dDevice.Get(), m_pImmediateContext.Get());
	m_vecDrawables.push_back(go);

	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"Failed to init mesh in game object.", L"Error", MB_OK);
		return hr;
	}

	// CREATE A SIMPLE game object
	Cube* go2 = new Cube(XMFLOAT3(0, 0, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1), "Cube 2");
	 hr = go2->initMesh(m_pd3dDevice.Get(), m_pImmediateContext.Get());
	m_vecDrawables.push_back(go2);

	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"Failed to init mesh in game object.", L"Error", MB_OK);
		return hr;
	}

	m_pCamera = new Camera(XMFLOAT3(0, 0, 4), XMFLOAT3(0, 0, -1), XMFLOAT3(0.0f, 1.0f, 0.0f), width, height);

	// Create the constant buffer
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = m_pd3dDevice->CreateBuffer(&bd, nullptr, &m_pConstantBuffer);
	if (FAILED(hr))
		return hr;

	setupLightProperties();

	// Create the light constant buffer
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(LightPropertiesConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = m_pd3dDevice->CreateBuffer(&bd, nullptr, &m_pLightConstantBuffer);
	if (FAILED(hr))
		return hr;

	return S_OK;
}

void Scene::cleanUp()
{
	for (Cube* obj : m_vecDrawables)
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
		if (i == 1)
		{
			light.Enabled = static_cast<int>(false);
		}
		else
		{
			light.Enabled = static_cast<int>(true);
		}
		light.LightType = PointLight;
		light.Color = XMFLOAT4(1, 1, 1, 1);
		light.SpotAngle = XMConvertToRadians(45.0f);
		light.ConstantAttenuation = 1.0f;
		light.LinearAttenuation = 1;
		light.QuadraticAttenuation = 1;

		// set up the light
		XMFLOAT4 LightPosition(0, 0, 1.5f, 1);
		light.Position = LightPosition;

		m_lightProperties.EyePosition = LightPosition;
		m_lightProperties.Lights[i] = light;
	}
}

void Scene::updateLightProperties(unsigned int index, const Light& light)
{
	m_lightProperties.Lights[index] = light;
}

void Scene::update(const float deltaTime)
{
	m_pImmediateContext->UpdateSubresource(m_pLightConstantBuffer.Get(), 0, nullptr, &m_lightProperties, 0, 0);
	ID3D11Buffer* buf = m_pLightConstantBuffer.Get();
	m_pImmediateContext->PSSetConstantBuffers(2, 1, &buf);

	// for the cube we rotate using delta time as a rotation value
	static float rotation = 0;
	rotation += deltaTime;

	for (unsigned int i = 0; i < m_vecDrawables.size(); i++)
	{
		m_vecDrawables[i]->update(deltaTime, m_pImmediateContext.Get());

		m_vecDrawables[i]->draw(m_pImmediateContext.Get(), getCamera(), m_pConstantBuffer.Get());
	}
}