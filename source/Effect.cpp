#include "pch.h"
#include "Effect.h"

#pragma region Effect
dae::Effect::Effect(ID3D11Device* pDevice, const std::wstring& assetFile)
{
    // Load effect
    m_pEffect = LoadEffect(pDevice, assetFile);

    // Get all variables from effect that we need to set in C++
    m_pTechnique = m_pEffect->GetTechniqueByName("DefaultTechnique");
    if (!m_pTechnique->IsValid())
        std::wcout << L"Technique not valid!\n";

    m_pWorldMat = m_pEffect->GetVariableByName("gWorldMatrix")->AsMatrix();
    if (!m_pWorldMat->IsValid())
        std::wcout << L"m_pWorldMatrix is not valid!\n";

    m_pViewMat = m_pEffect->GetVariableByName("gViewInverseMatrix")->AsMatrix();
    if (!m_pViewMat->IsValid())
        std::wcout << L"m_pViewInverseMatrix is not valid!\n";

    m_pProjMat = m_pEffect->GetVariableByName("gWorldViewProjectionMatrix")->AsMatrix();
    if (!m_pProjMat->IsValid())
        std::wcout << L"m_pWorldViewProjectionMatrix is not valid!\n";

    m_pRasterizer = m_pEffect->GetVariableByName("gRasterizeState")->AsRasterizer();
    if (!m_pRasterizer->IsValid())
        std::wcout << L"m_pRasterizer is not valid!\n";

    D3D11_RASTERIZER_DESC rasterizer_desc{};
    rasterizer_desc.FillMode = D3D11_FILL_SOLID;

    rasterizer_desc.CullMode = D3D11_CULL_BACK;
    if(!SUCCEEDED(pDevice->CreateRasterizerState(&rasterizer_desc, &m_pBackFaceCulling))) assert(-1);

    rasterizer_desc.CullMode = D3D11_CULL_FRONT;
    if (!SUCCEEDED(pDevice->CreateRasterizerState(&rasterizer_desc, &m_pFrontFaceCulling))) assert(-1);

    rasterizer_desc.CullMode = D3D11_CULL_NONE;
    if (!SUCCEEDED(pDevice->CreateRasterizerState(&rasterizer_desc, &m_pNoCulling))) assert(-1);

    m_pSampler = m_pEffect->GetVariableByName("gSampler")->AsSampler();
    if (!m_pSampler->IsValid())
        std::wcout << L"m_pSampler is not valid!\n";

    D3D11_SAMPLER_DESC sampler_desc{};
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampler_desc.MipLODBias = 0;
    sampler_desc.MinLOD = 0;
    sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
    sampler_desc.MaxAnisotropy = 16;

    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    if (!SUCCEEDED(pDevice->CreateSamplerState(&sampler_desc, &m_pPoint))) assert(-1);

    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    if (!SUCCEEDED(pDevice->CreateSamplerState(&sampler_desc, &m_pLinear))) assert(-1);

    sampler_desc.Filter = D3D11_FILTER_ANISOTROPIC;
    if (!SUCCEEDED(pDevice->CreateSamplerState(&sampler_desc, &m_pAnisotropic))) assert(-1);

    LoadInputLayout(pDevice, m_pTechnique);
}

dae::Effect::~Effect()
{
    if (m_pInputLayout) m_pInputLayout->Release();
    if (m_pAnisotropic) m_pAnisotropic->Release();
    if (m_pLinear) m_pLinear->Release();
    if (m_pPoint) m_pPoint->Release();
    if (m_pSampler) m_pSampler->Release();
    if (m_pNoCulling) m_pNoCulling->Release();
    if (m_pFrontFaceCulling) m_pFrontFaceCulling->Release();
    if (m_pBackFaceCulling) m_pBackFaceCulling->Release();
    if (m_pRasterizer) m_pRasterizer->Release();
    if (m_pProjMat) m_pProjMat->Release();
    if (m_pViewMat) m_pViewMat->Release();
    if (m_pWorldMat) m_pWorldMat->Release();
    if (m_pTechnique) m_pTechnique->Release();
    if (m_pEffect) m_pEffect->Release();
}

ID3DX11Effect* dae::Effect::LoadEffect(ID3D11Device* pDevice, const std::wstring& assetFile)
{
    HRESULT result;
    ID3D10Blob* pErrorBlob{ nullptr };
    ID3DX11Effect* pEffect{};

    DWORD shader_flags = 0;
#if defined( DEBUG ) || defined( _DEBUG )
    shader_flags |= D3DCOMPILE_DEBUG;
    shader_flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    result = D3DX11CompileEffectFromFile(assetFile.c_str(),
        nullptr,
        nullptr,
        shader_flags,
        0,
        pDevice,
        &pEffect,
        &pErrorBlob
    );

    if (FAILED(result))
    {
        if (pErrorBlob != nullptr)
        {
            const char* pErrors = static_cast<char*>(pErrorBlob->GetBufferPointer());

            std::wstringstream ss;
            for (unsigned int i = 0; i < pErrorBlob->GetBufferSize(); i++)
                ss << pErrors[i];

            OutputDebugStringW(ss.str().c_str());
            pErrorBlob->Release();
            pErrorBlob = nullptr;

            std::wcout << ss.str() << std::endl;
        }
        else
        {
            std::wstringstream ss;
            ss << "EffectLoader: Failed to CreateEffectFromFile!\nPath: " << assetFile;
            std::wcout << ss.str() << std::endl;
            return nullptr;
        }
    }

    return pEffect;
}

ID3D11InputLayout* dae::Effect::LoadInputLayout(ID3D11Device* pDevice, ID3DX11EffectTechnique* pTechnique)
{
    // Create vertex layout
    static constexpr uint32_t num_elements{ 5 };
    D3D11_INPUT_ELEMENT_DESC vertex_desc[num_elements]{};

    vertex_desc[0].SemanticName = "POSITION";
    vertex_desc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    vertex_desc[0].AlignedByteOffset = 0;
    vertex_desc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

    vertex_desc[1].SemanticName = "COLOR";
    vertex_desc[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    vertex_desc[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    vertex_desc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

    vertex_desc[2].SemanticName = "NORMAL";
    vertex_desc[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    vertex_desc[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    vertex_desc[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

    vertex_desc[3].SemanticName = "TANGENT";
    vertex_desc[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    vertex_desc[3].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    vertex_desc[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

    vertex_desc[4].SemanticName = "TEXCOORD";
    vertex_desc[4].Format = DXGI_FORMAT_R32G32_FLOAT;
    vertex_desc[4].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    vertex_desc[4].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

    // Create input layout
    D3DX11_PASS_DESC pass_desc{};
    pTechnique->GetPassByIndex(0)->GetDesc(&pass_desc);

    const HRESULT result = pDevice->CreateInputLayout(
        vertex_desc,
        num_elements,
        pass_desc.pIAInputSignature,
        pass_desc.IAInputSignatureSize,
        &m_pInputLayout);

    if (FAILED(result))
        assert(-1);

    return m_pInputLayout;
}

void dae::Effect::SetTextureFiltering(const FilteringMode& mode)
{
    switch (mode)
    {
    case dae::FilteringMode::Point:
        m_pSampler->SetSampler(0, m_pPoint);
        break;
    case dae::FilteringMode::Linear:
        m_pSampler->SetSampler(0, m_pLinear);
        break;
    case dae::FilteringMode::Anisotropic:
        m_pSampler->SetSampler(0, m_pAnisotropic);
        break;
    }
}

void dae::Effect::SetCullMode(const CullMode& mode)
{
    switch (mode)
    {
    case dae::CullMode::BackFace:
        // pDeviceContext->RSSetState(m_pBackFaceCulling);
        m_pRasterizer->SetRasterizerState(0, m_pBackFaceCulling);
        break;
    case dae::CullMode::FrontFace:
        // pDeviceContext->RSSetState(m_pFrontFaceCulling);
        m_pRasterizer->SetRasterizerState(0, m_pFrontFaceCulling);
        break;
    case dae::CullMode::None:
        // pDeviceContext->RSSetState(m_pNoCulling);
        m_pRasterizer->SetRasterizerState(0, m_pNoCulling);
        break;
    }
}

void dae::Effect::SetWorldMatrix(const Matrix& worldMatrix)
{
    m_pWorldMat->SetMatrix(reinterpret_cast<const float*>(&worldMatrix));
}

void dae::Effect::SetViewInverseMatrix(const Matrix& inverseViewMatrix)
{
    m_pViewMat->SetMatrix(reinterpret_cast<const float*>(&inverseViewMatrix));
}

void dae::Effect::SetWorldViewProjectionMatrix(const Matrix& wvp)
{
    m_pProjMat->SetMatrix(reinterpret_cast<const float*>(&wvp));
}
#pragma endregion // Effect

#pragma region PosTex
dae::EffectPosTex::EffectPosTex(ID3D11Device* pDevice, const std::wstring& assetFile)
    : Effect(pDevice, assetFile)
{
    m_EffectType = EffectType::Diffuse;

    m_pDiffuseMap = m_pEffect->GetVariableByName("gDiffuseMap")->AsShaderResource();
    if (!m_pDiffuseMap->IsValid())
        std::wcout << L"m_pDiffuseMap is not valid!\n";

    m_pNormalMap = m_pEffect->GetVariableByName("gNormalMap")->AsShaderResource();
    if (!m_pNormalMap->IsValid())
        std::wcout << L"m_pNormalMap is not valid!\n";

    m_pSpecularMap = m_pEffect->GetVariableByName("gSpecularMap")->AsShaderResource();
    if (!m_pSpecularMap->IsValid())
        std::wcout << L"m_pSpecularMap is not valid!\n";

    m_pGlossMap = m_pEffect->GetVariableByName("gGlossMap")->AsShaderResource();
    if (!m_pGlossMap->IsValid())
        std::wcout << L"m_pGlossMap is not valid!\n";
}

dae::EffectPosTex::~EffectPosTex()
{
    if (m_pGlossMap) m_pGlossMap->Release();
    if (m_pSpecularMap) m_pSpecularMap->Release();
    if (m_pNormalMap) m_pNormalMap->Release();
    if (m_pDiffuseMap) m_pDiffuseMap->Release();
}

void dae::EffectPosTex::SetDiffuseMap(Texture* pDiffuseTexture)
{
    if (m_pDiffuseMap)
        m_pDiffuseMap->SetResource(pDiffuseTexture->GetSRV());
}

void dae::EffectPosTex::SetNormalMap(Texture* pNormalTexture)
{
    if (m_pNormalMap)
        m_pNormalMap->SetResource(pNormalTexture->GetSRV());
}

void dae::EffectPosTex::SetSpecularMap(Texture* pSpecularTexture)
{
    if (m_pSpecularMap)
        m_pSpecularMap->SetResource(pSpecularTexture->GetSRV());
}

void dae::EffectPosTex::SetGlossMap(Texture* pGlossTexture)
{
    if (m_pGlossMap)
        m_pGlossMap->SetResource(pGlossTexture->GetSRV());
}
#pragma endregion // PosTex

#pragma region Transparent
dae::EffectTransparent::EffectTransparent(ID3D11Device* pDevice, const std::wstring& assetFile)
    : Effect(pDevice, assetFile)
{
    m_EffectType = EffectType::Transparent;

    m_pDiffuseMap = m_pEffect->GetVariableByName("gDiffuseMap")->AsShaderResource();
    if (!m_pDiffuseMap->IsValid())
        std::wcout << L"m_pDiffuseMap is not valid!\n";
}

dae::EffectTransparent::~EffectTransparent()
{
    if (m_pDiffuseMap) m_pDiffuseMap->Release();
}

void dae::EffectTransparent::SetDiffuseMap(Texture* pDiffuseTexture)
{
    if (m_pDiffuseMap)
        m_pDiffuseMap->SetResource(pDiffuseTexture->GetSRV());
}
#pragma endregion // Transparent