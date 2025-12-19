#include "IRenderable.h"

IRenderable::IRenderable()
{
	m_meshData.VertexBuffer = nullptr;
	m_meshData.IndexBuffer = nullptr;

	m_textureResourceView = nullptr;
	m_textureSampler = nullptr;

	// Initialize the world matrix
	XMStoreFloat4x4(&m_world, XMMatrixIdentity());
}

IRenderable::~IRenderable()
{
	Cleanup();
}

void IRenderable::Update(const float deltaTime, ID3D11DeviceContext* pContext)
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

void IRenderable::Draw(ID3D11DeviceContext* pContext, Camera* camera, ID3D11Buffer* m_pConstantBuffer)
{
	pContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);

	ConstantBuffer cb;
	cb.mView = XMMatrixTranspose(camera->GetViewMatrix());
	cb.mProjection = XMMatrixTranspose(camera->GetProjectionMatrix());
	cb.vOutputColor = XMFLOAT4(0, 0, 0, 0);

	// store world and the view / projection in a constant buffer for the vertex shader to use
	cb.mWorld = XMMatrixTranspose(XMLoadFloat4x4(GetTransform()));
	pContext->UpdateSubresource(m_pConstantBuffer, 0, nullptr, &cb, 0, 0);

	// Render a cube
	pContext->VSSetConstantBuffers(0, 1, &m_pConstantBuffer);

	ID3D11Buffer* materialCB = GetMaterialConstantBuffer();
	pContext->PSSetConstantBuffers(1, 1, &materialCB);

	// Set vertex buffer
	ID3D11Buffer* vbuf = m_meshData.VertexBuffer.Get();
	pContext->IASetVertexBuffers(0, 1, &vbuf, &m_meshData.VBStride, &m_meshData.VBOffset);

	// Set index buffer
	pContext->IASetIndexBuffer(m_meshData.IndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

	// Set primitive topology
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set the texture and sampler
	//if (m_textureResourceView != nullptr)

	{
		ID3D11ShaderResourceView* srv = m_textureResourceView.Get();
		pContext->PSSetShaderResources(0, 1, &srv);
		//if (m_normalMapResourceView != nullptr)
		{
			ID3D11ShaderResourceView* nrv = m_normalMapResourceView.Get();
			pContext->PSSetShaderResources(1, 1, &nrv);
		}

		ID3D11SamplerState* ss = m_textureSampler.Get();
		pContext->PSSetSamplers(0, 1, &ss);
	}

	// draw
	pContext->DrawIndexed(m_meshData.VertexCount, 0, 0);

	ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
	pContext->PSSetShaderResources(0, 2, nullSRVs);
}

void IRenderable::Cleanup()
{
	// we are using com pointers so no release() necessary
}

void IRenderable::SetTransform(XMMATRIX newTransform)
{
	XMVECTOR scaleVector, rotationQuatVector, translationVector;

	// This gives the rotation as a quaternion, so I needed to convert it to Euler angles
	XMMatrixDecompose(&scaleVector, &rotationQuatVector, &translationVector, newTransform);

	XMFLOAT3 scale;
	XMFLOAT3 translation;
	XMFLOAT4 rotQuat;
	XMStoreFloat3(&scale, scaleVector);
	XMStoreFloat3(&translation, translationVector);
	XMStoreFloat4(&rotQuat, rotationQuatVector);

	float ysqr = rotQuat.y * rotQuat.y;

	// Roll x-axis
	float t0 = +2.0f * (rotQuat.w * rotQuat.x + rotQuat.y * rotQuat.z);
	float t1 = +1.0f - 2.0f * (rotQuat.x * rotQuat.x + ysqr);
	float roll = atan2f(t0, t1);

	// Pitch y-axis
	float t2 = +2.0f * (rotQuat.w * rotQuat.y - rotQuat.z * rotQuat.x);
	t2 = t2 > 1.0f ? 1.0f : t2;
	t2 = t2 < -1.0f ? -1.0f : t2;
	float pitch = asinf(t2);

	// Yaw z-axis
	float t3 = +2.0f * (rotQuat.w * rotQuat.z + rotQuat.x * rotQuat.y);
	float t4 = +1.0f - 2.0f * (ysqr + rotQuat.z * rotQuat.z);
	float yaw = atan2f(t3, t4);

	XMFLOAT3 rotationF(XMConvertToDegrees(roll), XMConvertToDegrees(pitch), XMConvertToDegrees(yaw));

	// Set components
	SetScale(scale);
	SetRotate(rotationF);
	SetPosition(translation);
}