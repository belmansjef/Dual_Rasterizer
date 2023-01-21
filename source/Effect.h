#pragma once

namespace dae
{
	class Texture;
	enum class FilteringMode;
	enum class CullMode;

	enum class EffectType
	{
		Diffuse,
		Transparent
	};

	class Effect
	{
	public:
		Effect(ID3D11Device* pDevice, const std::wstring& assetFile);
		virtual ~Effect();

		Effect(const Effect& other) = delete;
		Effect& operator=(const Effect& other) = delete;
		Effect(Effect&& other) = delete;
		Effect& operator=(Effect&& other) = delete;

		ID3DX11Effect* LoadEffect(ID3D11Device* pDevice, const std::wstring& assetFile);
		ID3D11InputLayout* LoadInputLayout(ID3D11Device* pDevice, ID3DX11EffectTechnique* pTechnique);

		// Setters
		void SetTextureFiltering(const FilteringMode& mode);
		void SetCullMode(const CullMode& mode);

		void SetWorldMatrix(const Matrix& worldMatrix);
		void SetViewInverseMatrix(const Matrix& inverseViewMatrix);
		void SetWorldViewProjectionMatrix(const Matrix& wvp);

		// Getters
		ID3DX11Effect* GetEffect() const { return m_pEffect; }
		ID3DX11EffectTechnique* GetTechnique() const { return m_pTechnique; }
		ID3D11InputLayout* GetInputLayout() const { return m_pInputLayout; }
		EffectType GetEffectType() const { return m_EffectType; }

	protected:
		// Effect variables
		ID3DX11Effect* m_pEffect{};
		ID3DX11EffectTechnique* m_pTechnique{};
		ID3D11InputLayout* m_pInputLayout{};
		EffectType m_EffectType{};

		// Matrices
		ID3DX11EffectMatrixVariable* m_pWorldMat{};
		ID3DX11EffectMatrixVariable* m_pViewMat{};
		ID3DX11EffectMatrixVariable* m_pProjMat{};

		// Rasterizer
		ID3DX11EffectRasterizerVariable* m_pRasterizer{};
		ID3D11RasterizerState* m_pFrontFaceCulling{};
		ID3D11RasterizerState* m_pBackFaceCulling{};
		ID3D11RasterizerState* m_pNoCulling{};

		// Samplers
		ID3DX11EffectSamplerVariable* m_pSampler{};
		ID3D11SamplerState* m_pPoint{};
		ID3D11SamplerState* m_pLinear{};
		ID3D11SamplerState* m_pAnisotropic{};
	};

	class EffectPosTex final : public Effect
	{
	public:
		EffectPosTex(ID3D11Device* pDevice, const std::wstring& assetFile);
		virtual ~EffectPosTex();

		void SetDiffuseMap(Texture* pDiffuseTexture);
		void SetNormalMap(Texture* pNormalTexture);
		void SetSpecularMap(Texture* pSpecularTexture);
		void SetGlossMap(Texture* pGlossTexture);

	private:
		// Texture maps
		ID3DX11EffectShaderResourceVariable* m_pDiffuseMap{};
		ID3DX11EffectShaderResourceVariable* m_pNormalMap{};
		ID3DX11EffectShaderResourceVariable* m_pSpecularMap{};
		ID3DX11EffectShaderResourceVariable* m_pGlossMap{};
	};

	class EffectTransparent final : public Effect
	{
	public:
		EffectTransparent(ID3D11Device* pDevice, const std::wstring& assetFile);
		virtual ~EffectTransparent();

		void SetDiffuseMap(Texture* pDiffuseTexture);

	private:
		// Texture maps
		ID3DX11EffectShaderResourceVariable* m_pDiffuseMap{};
	};
}