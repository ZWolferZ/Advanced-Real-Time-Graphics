#include "IRenderable.h"

IRenderable::IRenderable()
{
	m_vertexBuffer = nullptr;
	m_indexBuffer = nullptr;
	m_textureResourceView = nullptr;
	m_textureSampler = nullptr;

	// Initialize the world matrix
	XMStoreFloat4x4(&m_world, XMMatrixIdentity());
}

IRenderable::~IRenderable()
{
	cleanup();
}

void IRenderable::update(const float deltaTime, ID3D11DeviceContext* pContext)
{
	// Don't overflow the rotation
	if (m_rotation.x > 360.0f) m_rotation.x = 0.0f;

	if (m_rotation.x < -360.0f) m_rotation.x = 0.0f;

	if (m_rotation.y > 360.0f) m_rotation.y = 0.0f;

	if (m_rotation.y < -360.0f) m_rotation.y = 0.0f;

	if (m_rotation.z > 360.0f) m_rotation.z = 0.0f;

	if (m_rotation.z < -360.0f) m_rotation.z = 0.0f;

	if (m_autoRotateX)
	{
		m_rotation.x += m_autoRotationSpeed * deltaTime;
	}

	if (m_autoRotateY)
	{
		m_rotation.y += m_autoRotationSpeed * deltaTime;
	}

	if (m_autoRotateZ)
	{
		m_rotation.z += m_autoRotationSpeed * deltaTime;
	}

	XMMATRIX translate = XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
	XMMATRIX scale = XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);
	XMMATRIX rotation = XMMatrixRotationX(XMConvertToRadians(m_rotation.x)) * XMMatrixRotationY(XMConvertToRadians(m_rotation.y)) * XMMatrixRotationZ(XMConvertToRadians(m_rotation.z));
	XMMATRIX world = scale * rotation * translate;
	XMStoreFloat4x4(&m_world, world);
}

void IRenderable::draw(ID3D11DeviceContext* pContext, Camera* camera, ID3D11Buffer* m_pConstantBuffer)
{
	pContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);

	ConstantBuffer cb;
	cb.mView = XMMatrixTranspose(camera->GetViewMatrix());
	cb.mProjection = XMMatrixTranspose(camera->GetProjectionMatrix());
	cb.vOutputColor = XMFLOAT4(0, 0, 0, 0);

	// store world and the view / projection in a constant buffer for the vertex shader to use
	cb.mWorld = XMMatrixTranspose(XMLoadFloat4x4(getTransform()));
	pContext->UpdateSubresource(m_pConstantBuffer, 0, nullptr, &cb, 0, 0);

	// Render a cube
	pContext->VSSetConstantBuffers(0, 1, &m_pConstantBuffer);

	ID3D11Buffer* materialCB = getMaterialConstantBuffer();
	pContext->PSSetConstantBuffers(1, 1, &materialCB);

	// Set vertex buffer
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	ID3D11Buffer* vbuf = m_vertexBuffer.Get();
	pContext->IASetVertexBuffers(0, 1, &vbuf, &stride, &offset);

	// Set index buffer
	pContext->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

	// Set primitive topology
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set the texture and sampler
	if (m_textureResourceView != nullptr)
	{
		ID3D11ShaderResourceView* srv = m_textureResourceView.Get();
		pContext->PSSetShaderResources(0, 1, &srv);
		ID3D11SamplerState* ss = m_textureSampler.Get();
		pContext->PSSetSamplers(0, 1, &ss);
	}

	// draw
	pContext->DrawIndexed(m_vertexCount, 0, 0);
}

void IRenderable::cleanup()
{
	// we are using com pointers so no release() necessary
}