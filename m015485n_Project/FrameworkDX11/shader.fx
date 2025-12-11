//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float4 vOutputColor;
};

Texture2D txDiffuse : register(t0);
Texture2D txDiffuse2 : register(t1);
SamplerState samLinear : register(s0);

#define MAX_LIGHTS 2
// Light types.
#define DIRECTIONAL_LIGHT 0
#define POINT_LIGHT 1
#define SPOT_LIGHT 2

struct _Material
{
    float4 Emissive; // 16 bytes
//----------------------------------- (16 byte boundary)
    float4 Ambient; // 16 bytes
//------------------------------------(16 byte boundary)
    float4 Diffuse; // 16 bytes
//----------------------------------- (16 byte boundary)
    float4 Specular; // 16 bytes
//----------------------------------- (16 byte boundary)
    float SpecularPower; // 4 bytes
    bool UseTexture; // 4 bytes
    float2 Padding; // 8 bytes
//----------------------------------- (16 byte boundary)
}; // Total: // 80 bytes ( 5 * 16 )

cbuffer MaterialProperties : register(b1)
{
    _Material Material;
};

struct Light
{
    float4 Position; // 16 bytes
//----------------------------------- (16 byte boundary)
    float4 Direction; // 16 bytes
//----------------------------------- (16 byte boundary)
    float4 Color; // 16 bytes
//----------------------------------- (16 byte boundary)
    float SpotAngle; // 4 bytes
    float ConstantAttenuation; // 4 bytes
    float LinearAttenuation; // 4 bytes
    float QuadraticAttenuation; // 4 bytes
//----------------------------------- (16 byte boundary)
    int LightType; // 4 bytes
    bool Enabled; // 4 bytes
    int2 Padding; // 8 bytes
//----------------------------------- (16 byte boundary)
}; // Total: // 80 bytes (5 * 16)

cbuffer LightProperties : register(b2)
{
    float4 EyePosition; // 16 bytes
//----------------------------------- (16 byte boundary)
    float4 GlobalAmbient; // 16 bytes
//----------------------------------- (16 byte boundary)
    Light Lights[MAX_LIGHTS]; // 80 * 8 = 640 bytes
};

//--------------------------------------------------------------------------------------
struct VS_INPUT
{
    float4 Pos : POSITION;
    float3 Norm : NORMAL;
    float2 Tex : TEXCOORD0;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float4 worldPos : TEXCOORD1;
    float3 Norm : NORMAL;
    float2 Tex : TEXCOORD0;
};

float4 DoDiffuse(Light light, float3 L, float3 N)
{
    float NdotL = max(0, dot(N, L));
    return light.Color * NdotL;
}

float4 DoSpecular(Light lightObject, float3 pixelToEyeVectorNormalised, float3 pixelToLightVectorNormalised, float3 Normal)
{
    float lightIntensity = saturate(dot(Normal, pixelToLightVectorNormalised));
    float specular = 0;
    if (lightIntensity > 0.0f)
    {
// note the reflection equation requires the light to pixel vector - hence we reverse it
        float3 reflection = reflect(-pixelToLightVectorNormalised, Normal);
        float l = length(pixelToLightVectorNormalised);
        float d = dot(reflection, pixelToEyeVectorNormalised);
        d = d * l;
        d = saturate(d);
        d = pow(d, Material.SpecularPower);
        specular = d; // 128 = specular power Material.SpecularPower
    }

    return lightObject.Color * specular;


}

float DoAttenuation(Light light, float d)
{
    return 1.0f / (light.ConstantAttenuation + light.LinearAttenuation * d + light.QuadraticAttenuation * d * d);
}

struct LightingResult
{
    float4 Diffuse;
    float4 Specular;
};

LightingResult DoPointLight(Light light, float3 pixelToLightVectorNormalised, float3 pixelToEyeVectorNormalised, float distanceFromPixelToLight, float3 N)
{
    LightingResult result;

    float attenuation = DoAttenuation(light, distanceFromPixelToLight);

    result.Diffuse = DoDiffuse(light, pixelToLightVectorNormalised, N) * attenuation;
    result.Specular = DoSpecular(light, pixelToEyeVectorNormalised, pixelToLightVectorNormalised, N) * attenuation;

    return result;


}

LightingResult ComputeLighting(float4 worldPos, float3 N)
{
    LightingResult totalResult;
    totalResult.Diffuse = float4(0, 0, 0, 0);
    totalResult.Specular = float4(0, 0, 0, 0);

[unroll]
    for (int i = 0; i < MAX_LIGHTS; ++i)
    {
        LightingResult result;
        result.Diffuse = float4(0, 0, 0, 0);
        result.Specular = float4(0, 0, 0, 0);
        
        float4 pixelToLightVectorNormalised = normalize(Lights[i].Position - worldPos);
        float4 pixelToEyeVectorNormalised = normalize(EyePosition - worldPos);
        float distanceFromPixelToLight = length(worldPos - Lights[i].Position);

        if (!Lights[i].Enabled)
            continue;

        result = DoPointLight(Lights[i], pixelToLightVectorNormalised.xyz, pixelToEyeVectorNormalised.xyz, distanceFromPixelToLight, N);

        totalResult.Diffuse += result.Diffuse;
        totalResult.Specular += result.Specular;
    }

    totalResult.Diffuse = saturate(totalResult.Diffuse);
    totalResult.Specular = saturate(totalResult.Specular);

    return totalResult;


}

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;
    output.Pos = mul(input.Pos, World);
    output.worldPos = output.Pos;
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);

// multiply the normal by the world transform (to go from model space to world space)
    output.Norm = mul(float4(input.Norm, 0), World).xyz;

    output.Tex = input.Tex;

    return output;


}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

float4 PS(PS_INPUT IN) : SV_TARGET
{
    
  

    LightingResult lit = ComputeLighting(IN.worldPos, normalize(IN.Norm));

    float4 texColor = float4(1, 1, 1, 1);

    float4 emissive = Material.Emissive;
    float4 ambient = Material.Ambient * GlobalAmbient;
    float4 diffuse = Material.Diffuse * lit.Diffuse;
    float4 specular = Material.Specular * lit.Specular;

    if (Material.UseTexture)
    {

        if (IN.worldPos.x < 0)
        {
            texColor = txDiffuse2.Sample(samLinear, IN.Tex);
        }
        else
        {
            texColor = txDiffuse.Sample(samLinear, IN.Tex);
        }
    }

    float4 finalColor = (emissive + ambient + diffuse + specular) * texColor;

    return finalColor;


}

//--------------------------------------------------------------------------------------
// PSSolid - render a solid color
//--------------------------------------------------------------------------------------
float4 PSSolid(PS_INPUT input) : SV_Target
{
    return vOutputColor;
}