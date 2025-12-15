#include "GameObject.h"
#include "DDSTextureLoader.h"

using namespace std;
using namespace DirectX;

GameObject::GameObject(XMFLOAT3 Position, XMFLOAT3 Rotation, XMFLOAT3 Scale, string ObjectName, MeshData meshData, ID3D11Device* m_pd3dDevice, ID3D11DeviceContext* m_pImmediateContext, Microsoft::WRL::ComPtr <ID3D11PixelShader> pixelShader)
{
	SetPosition(Position);
	SetRotate(Rotation);
	SetScale(Scale);
	m_orginalPosition = Position;
	m_orginalRotation = Rotation;
	m_orginalScale = Scale;
	objectName = ObjectName;
	m_pixelShader = pixelShader;
	m_material.Material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_material.Material.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_material.Material.SpecularPower = 128.0f;
	m_originalMaterial = m_material;
	m_meshData = meshData;
	CreateSampler(m_pd3dDevice, m_pImmediateContext);

	CreateMaterialBuffer(m_pd3dDevice, m_pImmediateContext);
}

GameObject::GameObject(XMFLOAT3 Position, XMFLOAT3 Rotation, XMFLOAT3 Scale, string ObjectName, MeshData meshData, ID3D11Device* m_pd3dDevice, ID3D11DeviceContext* m_pImmediateContext, Microsoft::WRL::ComPtr <ID3D11PixelShader> pixelShader, Microsoft::WRL::ComPtr < ID3D11ShaderResourceView> texture)
{
	SetPosition(Position);
	SetRotate(Rotation);
	SetScale(Scale);
	m_orginalPosition = Position;
	m_orginalRotation = Rotation;
	m_orginalScale = Scale;
	objectName = ObjectName;
	m_pixelShader = pixelShader;
	m_textureResourceView = texture;
	m_material.Material.UseTexture = true;
	m_material.Material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_material.Material.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_material.Material.SpecularPower = 128.0f;
	m_originalMaterial = m_material;
	m_meshData = meshData;

	CreateSampler(m_pd3dDevice, m_pImmediateContext);

	CreateMaterialBuffer(m_pd3dDevice, m_pImmediateContext);
}

GameObject::GameObject(XMFLOAT3 Position, XMFLOAT3 Rotation, XMFLOAT3 Scale, string ObjectName, MeshData meshData, ID3D11Device* m_pd3dDevice, ID3D11DeviceContext* m_pImmediateContext, Microsoft::WRL::ComPtr <ID3D11PixelShader> pixelShader, Microsoft::WRL::ComPtr < ID3D11ShaderResourceView> texture, Microsoft::WRL::ComPtr < ID3D11ShaderResourceView> normalMap)
{
	SetPosition(Position);
	SetRotate(Rotation);
	SetScale(Scale);
	m_orginalPosition = Position;
	m_orginalRotation = Rotation;
	m_orginalScale = Scale;
	objectName = ObjectName;
	m_pixelShader = pixelShader;
	m_textureResourceView = texture;
	m_normalMapResourceView = normalMap;
	m_material.Material.UseTexture = true;
	m_material.Material.UseNormalMap = true;
	m_material.Material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_material.Material.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_material.Material.SpecularPower = 128.0f;
	m_originalMaterial = m_material;
	m_meshData = meshData;

	CreateSampler(m_pd3dDevice, m_pImmediateContext);

	CreateMaterialBuffer(m_pd3dDevice, m_pImmediateContext);
}

GameObject::~GameObject()
= default;

void GameObject::CreateSampler(ID3D11Device* m_pd3dDevice, ID3D11DeviceContext* m_pImmediateContext)
{
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_POINT;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	HRESULT hr = m_pd3dDevice->CreateSamplerState(&sampDesc, &m_textureSampler);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"Failed to init sampler in game object.", L"Error", MB_OK);
	}
}

void GameObject::CreateMaterialBuffer(ID3D11Device* m_pd3dDevice, ID3D11DeviceContext* m_pImmediateContext)
{
	D3D11_BUFFER_DESC bd = {};

	// Create the material constant buffer
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(MaterialPropertiesConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;

	Microsoft::WRL::ComPtr <ID3D11Buffer>* buf_out = &m_materialConstantBuffer;
	HRESULT hr = m_pd3dDevice->CreateBuffer(&bd, nullptr, buf_out->GetAddressOf());
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"Failed to init Material Buffer in game object.", L"Error", MB_OK);
	}

	m_pImmediateContext->UpdateSubresource(m_materialConstantBuffer.Get(), 0, nullptr, &m_material, 0, 0);
}