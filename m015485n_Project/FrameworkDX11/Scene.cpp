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
	Cube* go = new Cube(XMFLOAT3(0, 0, 0), std::pair(XMFLOAT3(0, 0, 0), 0.0f), 1);
	HRESULT hr = go->initMesh(m_pd3dDevice.Get(), m_pImmediateContext.Get());
	m_vecDrawables.push_back(go);

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
	for (auto& i : m_lightProperties.Lights)
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
		XMFLOAT4 LightPosition(m_pCamera->GetPosition().x, m_pCamera->GetPosition().y, m_pCamera->GetPosition().z, 1);
		light.Position = LightPosition;

		m_lightProperties.EyePosition = LightPosition;
		i = light;
	}
}

void Scene::updateLightProperties(unsigned int index, const Light& light)
{
	m_lightProperties.Lights[index] = light;
}

void Scene::update(const float deltaTime)
{
	ConstantBuffer cb1;
	cb1.mView = XMMatrixTranspose(getCamera()->GetViewMatrix());
	cb1.mProjection = XMMatrixTranspose(getCamera()->GetProjectionMatrix());
	cb1.vOutputColor = XMFLOAT4(0, 0, 0, 0);

	Cube* cube = m_vecDrawables[0];

	m_pImmediateContext->UpdateSubresource(m_pLightConstantBuffer.Get(), 0, nullptr, &m_lightProperties, 0, 0);
	ID3D11Buffer* buf = m_pLightConstantBuffer.Get();
	m_pImmediateContext->PSSetConstantBuffers(2, 1, &buf);

	// for the cube we rotate using delta time as a rotation value
	static float rotation = 0;
	rotation += deltaTime;

	for (unsigned int i = 0; i < 1; i++)
	{
		cube->setRotate(XMFLOAT3(0, 1, 0), rotation);

		cube->update(deltaTime, m_pImmediateContext.Get());

		// get the game object world transform
		XMMATRIX cubeTransformMatrix = XMLoadFloat4x4(cube->getTransform());

		// store world and the view / projection in a constant buffer for the vertex shader to use
		cb1.mWorld = XMMatrixTranspose(cubeTransformMatrix);
		m_pImmediateContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb1, 0, 0);

		// Render a cube
		ID3D11Buffer* cb = m_pConstantBuffer.Get();
		m_pImmediateContext->VSSetConstantBuffers(0, 1, &cb);

		ID3D11Buffer* materialCB = cube->getMaterialConstantBuffer();
		m_pImmediateContext->PSSetConstantBuffers(1, 1, &materialCB);

		cube->draw(m_pImmediateContext.Get());
	}
}