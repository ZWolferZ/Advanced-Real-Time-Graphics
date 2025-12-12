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
Texture2D txNormalMap : register(t1);
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
    bool UseNormalMap; // 4 bytes
    float Padding; // 8 bytes
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
    float3 Tangent : TANGENT;
    float3 BiNormal : BINORMAL;

};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float4 worldPos : TEXCOORD1;
    float3 Norm : NORMAL;
    float2 Tex : TEXCOORD0;
    float3 LightTangentVector : LightTangentVector;
    float3 EyeTangentVector : EyeTangentVector;
    float3x3 TBN : MATRIX;
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

LightingResult ComputeLighting(float4 worldPos, float3 N, float3 pixelToLightVectorNormalised, float3 pixelToEyeVectorNormalised)
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

    output.Tex = input.Tex;

    
    // multiply the normal by the world transform (to go from model space to world space)
    output.Norm = mul(float4(input.Norm, 0), World).xyz;

    float3 vertexToEye = EyePosition.xyz - output.worldPos.xyz;
    float3 vertexToLight;
   
    [unroll]
    for (int i = 0; i < MAX_LIGHTS; ++i)
    {
        vertexToLight =+ Lights[i].Position.xyz - output.worldPos.xyz;
    }
    
    vertexToLight = saturate(vertexToLight);
    

    float3 T = normalize(mul(input.Tangent, (float3x3)World));
    float3 B = normalize(mul(input.BiNormal, (float3x3) World));
    float3 N = normalize(mul(input.Norm, (float3x3) World));
    
    float3x3 TBN = float3x3(T, B, N);
    output.TBN = transpose(TBN);
    
    output.EyeTangentVector = mul(output.TBN,vertexToEye);
    output.LightTangentVector = mul(output.TBN,vertexToLight);
    
    return output;


}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

float4 PS(PS_INPUT IN) : SV_TARGET
{
    LightingResult lit;
    if (Material.UseNormalMap)
    {
        float4 bumpMap = txNormalMap.Sample(samLinear, IN.Tex);
        bumpMap = (bumpMap * 2.0f) - 1.0f;
        bumpMap = float4(normalize(bumpMap.xyz), 1);
        lit = ComputeLighting(IN.worldPos, bumpMap.xyz, normalize(IN.LightTangentVector), normalize(IN.EyeTangentVector));
    }
    else
    {
        lit = ComputeLighting(IN.worldPos, normalize(IN.Norm), normalize(IN.LightTangentVector), normalize(IN.EyeTangentVector));
    }
    

    float4 texColor = float4(1, 1, 1, 1);

    float4 emissive = Material.Emissive;
    float4 ambient = Material.Ambient * GlobalAmbient;
    float4 diffuse = Material.Diffuse * lit.Diffuse;
    float4 specular = Material.Specular * lit.Specular;

    float4 finalColor = emissive + ambient + diffuse + specular;
    if (Material.UseTexture)
    {
    	texColor = txDiffuse.Sample(samLinear, IN.Tex);
        finalColor *= texColor;
    }

    
    return finalColor;


}

//--------------------------------------------------------------------------------------
// PSSolid - render a solid color
//--------------------------------------------------------------------------------------
float4 PSSolid(PS_INPUT input) : SV_Target
{
    float4 vOutputColor = {1,1,0,1};

    return vOutputColor;
}