// Texture Maps
Texture2D gDiffuseMap                   : DIFFUSEMAP;
Texture2D gNormalMap                    : NORMALMAP;
Texture2D gSpecularMap                  : SPECULARMAP;
Texture2D gGlossMap                     : GLOSSINESMAP;

// Matrices
float4x4 gWorldViewProjectionMatrix     : PROJ;
float4x4 gWorldMatrix                   : WORLD;
float4x4 gViewInverseMatrix             : VIEW;

// States
SamplerState gSampler                   : SAMPLER_STATE;
RasterizerState gRasterizeState         : RASTERIZER_STATE;
BlendState gBlendSate                   : BLEND_STATE;
DepthStencilState gDepthStencilState    : DEPTHSTENCIL_STATE;

// Constants
float4 gAmbient = { 0.025f, 0.025f, 0.025f, 0.0f };
float3 gLightDirection = { .577f, -.577f, 0.577f };
float gLightIntensity = 7.f;
float gShininess = 25.f;
float gPI = 3.14159265358979323846f;

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
    float4 Position         : SV_POSITION;
    float4 WorldPosition    : WORLD_POSITION;
    float3 Normal           : NORMAL;
    float3 Tangent          : TANGENT;
    float2 TexCoord         : TEXCOORD;
};

//----------------------------------------------
//	Vertex Shader
//----------------------------------------------
VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT) 0;
	
	// Transform the input parameters
    output.Position         = mul(float4(input.Position, 1.f), gWorldViewProjectionMatrix);
    output.WorldPosition    = mul(float4(input.Position, 1.f), gWorldMatrix);
    output.Normal           = mul(normalize(input.Normal), (float3x3) gWorldMatrix);
    output.Tangent          = mul(normalize(input.Tangent), (float3x3) gWorldMatrix);
    output.TexCoord         = input.TexCoord;
    
    return output;
}

//----------------------------------------------
//	Pixel Shader
//----------------------------------------------
float Phong(float ks, float exp, float3 l, float3 v, float3 n)
{
    float3 reflected = reflect(-l, n);
    float cosAngle = saturate(dot(reflected, v));
    float specular = mul(ks, pow(cosAngle, exp));
    
    return specular;
}

float4 LambertDiffuse(float kd, float4 color)
{
    return mul(color, kd) / gPI;
}


float4 PS(VS_OUTPUT input) : SV_TARGET
{
    // Normal sampling
    float3 binormal = normalize(cross(input.Normal, input.Tangent));
    float3x3 tangentSpaceAxis = { input.Tangent, binormal, input.Normal };
    float3 sampledNormal = normalize(mul(mul(gNormalMap.Sample(gSampler, input.TexCoord), 2.f).xyz - float3(1.f, 1.f, 1.f), tangentSpaceAxis));
    
    // Lambert cosine law
    float observedArea = saturate(dot(sampledNormal, -gLightDirection));
    
    // Lambert diffuse
    float4 diffuse = LambertDiffuse(1.f, gDiffuseMap.Sample(gSampler, input.TexCoord));
    
    // Phong Specular shading
    float3 viewDirection = normalize(input.WorldPosition.xyz - gViewInverseMatrix[3].xyz);
    float exponent = mul(gGlossMap.Sample(gSampler, input.TexCoord).r, gShininess);
    float4 phongSpecular = mul(gSpecularMap.Sample(gSampler, input.TexCoord), Phong(1.f, exponent, gLightDirection, viewDirection, sampledNormal));
    
    return mul(mul(diffuse, gLightIntensity) + phongSpecular, observedArea) + gAmbient;
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