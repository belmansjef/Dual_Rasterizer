#include "pch.h"
#include "Mesh.h"

namespace dae
{
#pragma warning ( push )
#pragma warning( disable : 26495) // Uninit bla bla
	dae::Mesh::Mesh(ID3D11Device* pDevice, const std::vector<Vertex_In>& vertices, const std::vector<uint32_t>& indices)
		: m_IsEnabled(true)
		, m_VerticesIn(vertices)
		, m_Indices(indices)
	{

		// Create vertex buffer
		D3D11_BUFFER_DESC buffer_desc{};
		buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;
		buffer_desc.ByteWidth = sizeof(Vertex_In) * static_cast<uint32_t>(vertices.size());
		buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		buffer_desc.CPUAccessFlags = 0;
		buffer_desc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA init_data{};
		init_data.pSysMem = vertices.data();

		HRESULT result = pDevice->CreateBuffer(&buffer_desc, &init_data, &m_pVertexBuffer);
		if (FAILED(result))
			return;

		// Create index buffer
		m_NumIndices = static_cast<uint32_t>(indices.size());
		buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;
		buffer_desc.ByteWidth = sizeof(uint32_t) * m_NumIndices;
		buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		buffer_desc.CPUAccessFlags = 0;
		buffer_desc.MiscFlags = 0;

		init_data.pSysMem = indices.data();
		result = pDevice->CreateBuffer(&buffer_desc, &init_data, &m_pIndexBuffer);
		if (FAILED(result))
			return;
	}

	Mesh::Mesh(const Mesh& other) noexcept
	{
		m_pVertexBuffer = nullptr;
		m_pIndexBuffer = nullptr;

		m_IsEnabled = other.m_IsEnabled;
		m_PrimitiveTopology = other.m_PrimitiveTopology;
		m_CullMode = other.m_CullMode;

		m_pDiffuseMap = other.m_pDiffuseMap;
		m_pNormalMap = other.m_pNormalMap;
		m_pSpecularMap = other.m_pSpecularMap;
		m_pGlossMap = other.m_pGlossMap;

		m_WorldMatrix = other.m_WorldMatrix;
		m_TranslationMatrix = other.m_TranslationMatrix;
		m_RotationMatrix = other.m_RotationMatrix;

		m_Indices = other.m_Indices;
		m_VerticesIn = other.m_VerticesIn;
		m_VerticesOut = other.m_VerticesOut;

		m_pEffect = other.m_pEffect;
	}
#pragma warning ( pop )

	dae::Mesh::~Mesh()
	{
		if(m_pIndexBuffer) m_pIndexBuffer->Release();
		if(m_pVertexBuffer) m_pVertexBuffer->Release();
	}

	void dae::Mesh::Update(const Matrix& viewMatrix, const Matrix& projMatrix)
	{
		m_pEffect->SetWorldViewProjectionMatrix(m_WorldMatrix * viewMatrix * projMatrix);
		m_pEffect->SetWorldMatrix(m_WorldMatrix);
		m_pEffect->SetViewInverseMatrix(Matrix::Inverse(viewMatrix));
	}

	void dae::Mesh::Render(ID3D11DeviceContext* pDeviceContext)
	{
		// 1. Set primitive topology
		pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// 2. Set input layout
		pDeviceContext->IASetInputLayout(m_pEffect->GetInputLayout());

		// 3. Set VertexBuffer
		constexpr UINT stride = sizeof(Vertex_In);
		constexpr UINT offset = 0;
		pDeviceContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);

		// 4. Set IndexBuffer
		pDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

		// 5. Draw
		D3DX11_TECHNIQUE_DESC tech_desc{};
		m_pEffect->GetTechnique()->GetDesc(&tech_desc);
		for (UINT p = 0; p < tech_desc.Passes; p++)
		{
			m_pEffect->GetTechnique()->GetPassByIndex(p)->Apply(0, pDeviceContext);
			pDeviceContext->DrawIndexed(m_NumIndices, 0, 0);
		}
	}
}