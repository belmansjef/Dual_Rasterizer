#pragma once
#include "DataTypes.h"

namespace dae
{
	class Texture;

	class Mesh
	{
	public:
		Mesh() = default;
		Mesh(ID3D11Device* pDevice, const std::vector<Vertex_In>& vertices, const std::vector<uint32_t>& indices);

		~Mesh();

		void Update(const Matrix& viewMatrix, const Matrix& projMatrix);
		void Render(ID3D11DeviceContext* pDeviceContext);

		// Setters
		void SetIsEnabled(bool value)					{ m_IsEnabled = value; }
		void SetCullMode(CullMode cullMode)				{ m_CullMode = cullMode; }
		void SetEffect(std::shared_ptr<Effect> pEffect) { m_pEffect = pEffect; }
		void Translate(const Vector3& translation)		{ m_WorldMatrix = Matrix::CreateTranslation(translation) * m_WorldMatrix; }
		void RotateY(float yaw)							{ m_WorldMatrix = Matrix::CreateRotationY(yaw) * m_WorldMatrix; }

		void SetDiffuseMap(std::shared_ptr<Texture> pDiffuse) { m_pDiffuseMap = pDiffuse; }
		void SetNormalMap(std::shared_ptr<Texture> pNormal) { m_pNormalMap = pNormal; }
		void SetSpecularMap(std::shared_ptr<Texture> pSpecular) { m_pSpecularMap = pSpecular; }
		void SetGlossMap(std::shared_ptr<Texture> pGloss) { m_pGlossMap = pGloss; }

		void SetVerticesOut(std::vector<Vertex_Out> verticesOut) { m_VerticesOut = verticesOut; }
		void SetIndices(std::vector<uint32_t> indices) { m_Indices = indices; }

		// Getters
		bool IsEnabled() const { return m_IsEnabled; }
		PrimitiveTopology GetPrimitiveTopology() const { return m_PrimitiveTopology; }
		CullMode GetCullMode() const { return m_CullMode; }

		std::shared_ptr<Texture> GetDiffuseMap() const { return m_pDiffuseMap; }
		std::shared_ptr<Texture> GetNormalMap() const { return m_pNormalMap; }
		std::shared_ptr<Texture> GetSpecularMap() const { return m_pSpecularMap; }
		std::shared_ptr<Texture> GetGlossMap() const { return m_pGlossMap; }

		Matrix GetWorldMatrix() const { return m_WorldMatrix; }

		std::vector<uint32_t> GetIndices() const { return m_Indices; }
		std::vector<Vertex_In> GetVerticesIn() const { return m_VerticesIn; }
		std::vector<Vertex_Out>& GetVerticesOut() { return m_VerticesOut; }

		std::shared_ptr<Effect> GetEffect() const { return m_pEffect; }

	private:
		// Common
		bool m_IsEnabled;
		PrimitiveTopology m_PrimitiveTopology;
		CullMode m_CullMode;

		// Textures
		std::shared_ptr<Texture> m_pDiffuseMap;
		std::shared_ptr<Texture> m_pNormalMap;
		std::shared_ptr<Texture> m_pSpecularMap;
		std::shared_ptr<Texture> m_pGlossMap;

		// Matrices
		Matrix m_WorldMatrix;
		Matrix m_TranslationMatrix;
		Matrix m_RotationMatrix;

		// Software
		std::vector<uint32_t> m_Indices;
		std::vector<Vertex_In> m_VerticesIn;
		std::vector<Vertex_Out> m_VerticesOut;

		// DirectX
		ID3D11Buffer* m_pVertexBuffer;
		ID3D11Buffer* m_pIndexBuffer;
		uint32_t m_NumIndices;

		std::shared_ptr<Effect> m_pEffect;
	};
}
