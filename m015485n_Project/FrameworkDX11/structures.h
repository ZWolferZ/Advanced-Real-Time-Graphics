#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include <wrl/client.h>

using namespace std;
using namespace DirectX;

//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------

struct SimpleVertex
{
	XMFLOAT3 Pos;
	XMFLOAT3 Normal;
	XMFLOAT2 TexCoord;
	XMFLOAT3 Tangent;
	XMFLOAT3 BiNormal;
};
struct MeshData
{
	Microsoft::WRL::ComPtr<ID3D11Buffer> VertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> IndexBuffer;
	UINT VBStride;
	UINT VBOffset;
	UINT IndexCount;
	UINT VertexCount;
};

inline void CalculateTangentBinormal(SimpleVertex v0, SimpleVertex v1, SimpleVertex v2, XMFLOAT3& normal, XMFLOAT3& tangent, XMFLOAT3& binormal)
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

inline void CalculateModelVectorsNoSharedVertices(SimpleVertex* vertices, int vertexCount)
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

inline void CalculateModelVectorsSharedVertices(vector<SimpleVertex>& vertices, const std::vector<WORD>& indices)
{
	// PLUS EQUAL IN THE FOR LOOP SOMEBODY COOKED HERE
	for (size_t i = 0; i < indices.size(); i += 3)
	{
		SimpleVertex& v0 = vertices[indices[i]];
		SimpleVertex& v1 = vertices[indices[i + 1]];
		SimpleVertex& v2 = vertices[indices[i + 2]];

		XMFLOAT3 edge1 = {
			v1.Pos.x - v0.Pos.x,
			v1.Pos.y - v0.Pos.y,
			v1.Pos.z - v0.Pos.z
		};
		XMFLOAT3 edge2 = {
			v2.Pos.x - v0.Pos.x,
			v2.Pos.y - v0.Pos.y,
			v2.Pos.z - v0.Pos.z
		};

		float deltaU1 = v1.TexCoord.x - v0.TexCoord.x;
		float deltaV1 = v1.TexCoord.y - v0.TexCoord.y;
		float deltaU2 = v2.TexCoord.x - v0.TexCoord.x;
		float deltaV2 = v2.TexCoord.y - v0.TexCoord.y;

		float f = 1.0f / (deltaU1 * deltaV2 - deltaU2 * deltaV1);

		XMFLOAT3 tangent;
		tangent.x = f * (deltaV2 * edge1.x - deltaV1 * edge2.x);
		tangent.y = f * (deltaV2 * edge1.y - deltaV1 * edge2.y);
		tangent.z = f * (deltaV2 * edge1.z - deltaV1 * edge2.z);

		XMFLOAT3 binormal;
		binormal.x = f * (-deltaU2 * edge1.x + deltaU1 * edge2.x);
		binormal.y = f * (-deltaU2 * edge1.y + deltaU1 * edge2.y);
		binormal.z = f * (-deltaU2 * edge1.z + deltaU1 * edge2.z);

		// Accumulate the tangents and binormals for each vertex
		v0.Tangent.x += tangent.x; v0.Tangent.y += tangent.y; v0.Tangent.z += tangent.z;
		v1.Tangent.x += tangent.x; v1.Tangent.y += tangent.y; v1.Tangent.z += tangent.z;
		v2.Tangent.x += tangent.x; v2.Tangent.y += tangent.y; v2.Tangent.z += tangent.z;

		v0.BiNormal.x += binormal.x; v0.BiNormal.y += binormal.y; v0.BiNormal.z += binormal.z;
		v1.BiNormal.x += binormal.x; v1.BiNormal.y += binormal.y; v1.BiNormal.z += binormal.z;
		v2.BiNormal.x += binormal.x; v2.BiNormal.y += binormal.y; v2.BiNormal.z += binormal.z;
	}

	// Normalize Them
	for (auto& v : vertices)
	{
		XMVECTOR t = XMLoadFloat3(&v.Tangent);
		XMVECTOR n = XMLoadFloat3(&v.Normal);

		t = XMVector3Normalize(t - n * XMVector3Dot(n, t));
		XMStoreFloat3(&v.Tangent, t);

		XMVECTOR b = XMVector3Cross(n, t);
		XMStoreFloat3(&v.BiNormal, b);
	}
}

struct ConstantBuffer
{
	XMMATRIX mWorld;
	XMMATRIX mView;
	XMMATRIX mProjection;
	XMFLOAT4 vOutputColor;
};

struct _Material
{
	_Material()
		: Emissive(0.0f, 0.0f, 0.0f, 1.0f)
		, Ambient(0.1f, 0.1f, 0.1f, 1.0f)
		, Diffuse(1.0f, 1.0f, 1.0f, 1.0f)
		, Specular(1.0f, 1.0f, 1.0f, 1.0f)
		, SpecularPower(128.0f)
		, UseTexture(false)
		, UseNormalMap(false)

	{
		Padding = 0;
	}

	DirectX::XMFLOAT4   Emissive;
	//----------------------------------- (16 byte boundary)
	DirectX::XMFLOAT4   Ambient;
	//----------------------------------- (16 byte boundary)
	DirectX::XMFLOAT4   Diffuse;
	//----------------------------------- (16 byte boundary)
	DirectX::XMFLOAT4   Specular;
	//----------------------------------- (16 byte boundary)
	float               SpecularPower;
	// Add some padding complete the 16 byte boundary.
	int                 UseTexture;

	int					UseNormalMap;
	// Add some padding to complete the 16 byte boundary.
	float               Padding;
	//----------------------------------- (16 byte boundary)
}; // Total:                                80 bytes (5 * 16)

struct MaterialPropertiesConstantBuffer
{
	_Material   Material;
};

enum LightType
{
	DirectionalLight = 0,
	PointLight = 1,
	SpotLight = 2
};

struct Light
{
	Light()
		: Position(0.0f, 0.0f, 0.0f, 1.0f)
		, Direction(0.0f, 0.0f, 1.0f, 0.0f)
		, Color(1.0f, 1.0f, 1.0f, 1.0f)
		, SpotAngle(DirectX::XM_PIDIV2)
		, ConstantAttenuation(1.0f)
		, LinearAttenuation(0.0f)
		, QuadraticAttenuation(0.0f)
		, LightType(DirectionalLight)
		, Enabled(0)
	{
	}

	DirectX::XMFLOAT4    Position;
	//----------------------------------- (16 byte boundary)
	DirectX::XMFLOAT4    Direction;
	//----------------------------------- (16 byte boundary)
	DirectX::XMFLOAT4    Color;
	//----------------------------------- (16 byte boundary)
	float       SpotAngle;
	float       ConstantAttenuation;
	float       LinearAttenuation;
	float       QuadraticAttenuation;
	//----------------------------------- (16 byte boundary)
	int         LightType;
	int         Enabled;
	// Add some padding to make this struct size a multiple of 16 bytes.
	int         Padding[2];
	//----------------------------------- (16 byte boundary)
};  // Total:                              80 bytes ( 5 * 16 )

struct LightPropertiesConstantBuffer
{
	LightPropertiesConstantBuffer()
		: EyePosition(0, 0, 0, 1)
		, GlobalAmbient(0.2f, 0.2f, 0.8f, 1.0f)
	{
	}

	DirectX::XMFLOAT4   EyePosition;
	//----------------------------------- (16 byte boundary)
	DirectX::XMFLOAT4   GlobalAmbient;
	//----------------------------------- (16 byte boundary)
	int LightCount;
	DirectX::XMFLOAT3 Padding; // align to 16 bytes
};  // Total:                                  672 bytes (42 * 16)