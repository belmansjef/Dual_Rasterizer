// Texture Maps
Texture2D gDiffuseMap               : DIFFUSEMAP;

// Matrices
float4x4 gWorldViewProjectionMatrix : PROJ;
float4x4 gWorldMatrix               : WORLD;
float4x4 gViewInverseMatrix         : VIEW;

// States
SamplerState gSampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

RasterizerState gRasterizeState
{
    CullMode = None;
    FrontCounterClockwise = false;
};

BlendState gBlendSate
{
    BlendEnable[0] = true;
    SrcBlend = src_alpha;
    DestBlend = inv_src_alpha;
    BlendOp = add;
    SrcBlendAlpha = zero;
    DestBlendAlpha = zero;
    BlendOpAlpha = add;
    RenderTargetWriteMask[0] = 0x0F;
};

DepthStencilState gDepthStencilState
{
    DepthEnable = true;
    DepthWriteMask = zero;
    DepthFunc = less;
    StencilEnable = false;
};

//----------------------------------------------
//	Input/Output Structs
//----------------------------------------------
struct VS_INPUT
{
    float3 Position : POSITION;
    float3 Color    : COLOR;
    float3 Normal   : NORMAL;
    float3 Tangent  : TANGENT;
    float2 TexCoord : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
};

//----------------------------------------------
//	Vertex Shader
//----------------------------------------------
VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT) 0;
	
	// Transform the input parameters
    output.Position         = mul(float4(input.Position, 1.f), gWorldViewProjectionMatrix);
    output.TexCoord         = input.TexCoord;
    
    return output;
}

//----------------------------------------------
//	Pixel Shader
//----------------------------------------------
float4 PS(VS_OUTPUT input) : SV_TARGET
{
    return gDiffuseMap.Sample(gSampler, input.TexCoord);
}

//----------------------------------------------
//	Technique
//----------------------------------------------
technique11 DefaultTechnique
{
    pass P0
    {
        SetRasterizerState(gRasterizeState);
        SetDepthStencilState(gDepthStencilState, 0);
        SetBlendState(gBlendSate, float4(0.f, 0.f, 0.f, 0.f), 0xFFFFFFFF);
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS()));
    }
}