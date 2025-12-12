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
		if (m_normalMapResourceView != nullptr)
		{
			ID3D11ShaderResourceView* nrv = m_normalMapResourceView.Get();
			pContext->PSSetShaderResources(1, 1, &nrv);
		}

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

void IRenderable::CalculateModelVectors(SimpleVertex* vertices, int vertexCount)
{
	int faceCount, i, index;
	SimpleVertex vertex1, vertex2, vertex3;
	XMFLOAT3 tangent, binormal, normal;


	// Calculate the number of faces in the model.
	faceCount = vertexCount / 3;

	// Initialize the index to the model data.
	index = 0;

	// Go through all the faces and calculate the the tangent, binormal, and normal vectors.
	for (i = 0; i < faceCount; i++)
	{
		// Get the three vertices for this face from the model.
		vertex1.Pos.x = vertices[index].Pos.x;
		vertex1.Pos.y = vertices[index].Pos.y;
		vertex1.Pos.z = vertices[index].Pos.z;
		vertex1.TexCoord.x = vertices[index].TexCoord.x;
		vertex1.TexCoord.y = vertices[index].TexCoord.y;
		vertex1.Normal.x = vertices[index].Normal.x;
		vertex1.Normal.y = vertices[index].Normal.y;
		vertex1.Normal.z = vertices[index].Normal.z;
		index++;

		vertex2.Pos.x = vertices[index].Pos.x;
		vertex2.Pos.y = vertices[index].Pos.y;
		vertex2.Pos.z = vertices[index].Pos.z;
		vertex2.TexCoord.x = vertices[index].TexCoord.x;
		vertex2.TexCoord.y = vertices[index].TexCoord.y;
		vertex2.Normal.x = vertices[index].Normal.x;
		vertex2.Normal.y = vertices[index].Normal.y;
		vertex2.Normal.z = vertices[index].Normal.z;
		index++;

		vertex3.Pos.x = vertices[index].Pos.x;
		vertex3.Pos.y = vertices[index].Pos.y;
		vertex3.Pos.z = vertices[index].Pos.z;
		vertex3.TexCoord.x = vertices[index].TexCoord.x;
		vertex3.TexCoord.y = vertices[index].TexCoord.y;
		vertex3.Normal.x = vertices[index].Normal.x;
		vertex3.Normal.y = vertices[index].Normal.y;
		vertex3.Normal.z = vertices[index].Normal.z;
		index++;

		// Calculate the tangent and binormal of that face.
		CalculateTangentBinormal(vertex1, vertex2, vertex3, normal, tangent, binormal);

		// Store the normal, tangent, and binormal for this face back in the model structure.
		vertices[index - 1].Normal.x = normal.x;
		vertices[index - 1].Normal.y = normal.y;
		vertices[index - 1].Normal.z = normal.z;
		vertices[index - 1].Tangent.x = tangent.x;
		vertices[index - 1].Tangent.y = tangent.y;
		vertices[index - 1].Tangent.z = tangent.z;
		vertices[index - 1].BiNormal.x = binormal.x;
		vertices[index - 1].BiNormal.y = binormal.y;
		vertices[index - 1].BiNormal.z = binormal.z;

		vertices[index - 2].Normal.x = normal.x;
		vertices[index - 2].Normal.y = normal.y;
		vertices[index - 2].Normal.z = normal.z;
		vertices[index - 2].Tangent.x = tangent.x;
		vertices[index - 2].Tangent.y = tangent.y;
		vertices[index - 2].Tangent.z = tangent.z;
		vertices[index - 2].BiNormal.x = binormal.x;
		vertices[index - 2].BiNormal.y = binormal.y;
		vertices[index - 2].BiNormal.z = binormal.z;

		vertices[index - 3].Normal.x = normal.x;
		vertices[index - 3].Normal.y = normal.y;
		vertices[index - 3].Normal.z = normal.z;
		vertices[index - 3].Tangent.x = tangent.x;
		vertices[index - 3].Tangent.y = tangent.y;
		vertices[index - 3].Tangent.z = tangent.z;
		vertices[index - 3].BiNormal.x = binormal.x;
		vertices[index - 3].BiNormal.y = binormal.y;
		vertices[index - 3].BiNormal.z = binormal.z;
	}

}

void IRenderable::CalculateTangentBinormal(SimpleVertex v0, SimpleVertex v1, SimpleVertex v2, XMFLOAT3& normal, XMFLOAT3& tangent, XMFLOAT3& binormal)
{
	XMVECTOR vv0 = XMLoadFloat3(&v0.Pos);
	XMVECTOR vv1 = XMLoadFloat3(&v1.Pos);
	XMVECTOR vv2 = XMLoadFloat3(&v2.Pos);

	XMVECTOR e0 = vv1 - vv0;
	XMVECTOR e1 = vv2 - vv0;

	XMVECTOR e01cross = XMVector3Cross(e0, e1);
	XMFLOAT3 normalOut;
	XMStoreFloat3(&normalOut, e01cross);
	normal = normalOut;

	//let P = v1 - v0
	XMVECTOR P = vv1 - vv0;
	//let Q = v2 - v0
	XMVECTOR Q = vv2 - vv0;
	float s1 = v1.TexCoord.x - v0.TexCoord.x;
	float t1 = v1.TexCoord.y - v0.TexCoord.y;
	float s2 = v2.TexCoord.x - v0.TexCoord.x;
	float t2 = v2.TexCoord.y - v0.TexCoord.y;

	float tmp = 0.0f;
	if (fabsf(s1 * t2 - s2 * t1) <= 0.0001f)
	{
		tmp = 1.0f;
	}
	else
	{
		tmp = 1.0f / (s1 * t2 - s2 * t1);
	}

	XMFLOAT3 PF3, QF3;
	XMStoreFloat3(&PF3, P);
	XMStoreFloat3(&QF3, Q);

	tangent.x = (t2 * PF3.x - t1 * QF3.x);
	tangent.y = (t2 * PF3.y - t1 * QF3.y);
	tangent.z = (t2 * PF3.z - t1 * QF3.z);

	tangent.x = tangent.x * tmp;
	tangent.y = tangent.y * tmp;
	tangent.z = tangent.z * tmp;

	XMVECTOR vn = XMLoadFloat3(&normal);
	XMVECTOR vt = XMLoadFloat3(&tangent);
	XMVECTOR vb = XMVector3Cross(vt, vn);

	vn = XMVector3Normalize(vn);
	vt = XMVector3Normalize(vt);
	vb = XMVector3Normalize(vb);

	XMStoreFloat3(&normal, vn);
	XMStoreFloat3(&tangent, vt);
	XMStoreFloat3(&binormal, vb);

}
