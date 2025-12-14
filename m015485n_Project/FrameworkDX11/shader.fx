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

#define MAX_LIGHTS 5
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
    float3 EyeWorldSpaceVector : EyeWorldSpaceVector;
    float3 EyeTangentVector : EyeTangentVector;
    float3x3 TBN_Inv : MATRIX;
};

float3 VectorToTangentSpace(float3 vectorV, float3x3 TBN_Inv)
{
    return normalize(mul(vectorV, TBN_Inv));
}

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

LightingResult DoDirectionalLightNoNormalMap(Light light,float3 pixelToEyeVectorNormalised,float3 N)
{
    LightingResult result;

    float3 L = normalize(-light.Direction.xyz);

    result.Diffuse = DoDiffuse(light, L, N);
    result.Specular = DoSpecular(light, pixelToEyeVectorNormalised, L, N);

    return result;
}

LightingResult DoDirectionalLightNormalMap(Light light, float3 pixelToEyeVectorNormalised, float3 N,float3x3 TBN_Inv)
{
    LightingResult result;

    float3 L_TS = VectorToTangentSpace(-light.Direction.xyz, TBN_Inv);
    result.Diffuse = DoDiffuse(light, L_TS, N);
    result.Specular = DoSpecular(light, pixelToEyeVectorNormalised, L_TS, N);


    return result;
}

LightingResult DoSpotLightNoNormalMap(Light light,float3 pixelToLightVector,float3 pixelToEyeVectorNormalised,float distanceFromPixelToLight,float3 N)
{
    LightingResult result;
    result.Diffuse = float4(0, 0, 0, 0);
    result.Specular = float4(0, 0, 0, 0);

    float3 L = normalize(pixelToLightVector);

    float3 spotDir = normalize(-light.Direction.xyz);

    float spotFactor = dot(L, spotDir);

    if (spotFactor > light.SpotAngle)
    {
        float attenuation = DoAttenuation(light, distanceFromPixelToLight);
        float spotIntensity = saturate((spotFactor - light.SpotAngle) / (1.0f - light.SpotAngle));

        result.Diffuse =DoDiffuse(light, L, N) * attenuation * spotIntensity;

        result.Specular =DoSpecular(light, pixelToEyeVectorNormalised, L, N) * attenuation * spotIntensity;
    }

    return result;
}

LightingResult DoSpotLightNormalMap(Light light, float3 pixelToLightVector, float3 pixelToEyeVectorNormalised, float distanceFromPixelToLight, float3 N, float3x3 TBN_Inv)
{
    LightingResult result;
    result.Diffuse = float4(0, 0, 0, 0);
    result.Specular = float4(0, 0, 0, 0);

    float3 L = normalize(pixelToLightVector);

    float3 spotDirTS = VectorToTangentSpace(-light.Direction.xyz, TBN_Inv);
    float spotFactor = dot(L, spotDirTS);


    if (spotFactor > light.SpotAngle)
    {
        float attenuation = DoAttenuation(light, distanceFromPixelToLight);
        float spotIntensity = saturate((spotFactor - light.SpotAngle) / (1.0f - light.SpotAngle));

        result.Diffuse = DoDiffuse(light, L, N) * attenuation * spotIntensity;

        result.Specular = DoSpecular(light, pixelToEyeVectorNormalised, L, N) * attenuation * spotIntensity;
    }

    return result;
}



LightingResult ComputeLightingNormalMap(float4 worldPos,float3 N,float3 pixelToEyeVectorNormalised,float3x3 TBN_Inv)
{
    LightingResult totalResult;
    totalResult.Diffuse = float4(0, 0, 0, 0);
    totalResult.Specular = float4(0, 0, 0, 0);

    [unroll]
    for (int i = 0; i < MAX_LIGHTS; ++i)
    {
        if (!Lights[i].Enabled)
            continue;

        LightingResult result;
        result.Diffuse = float4(0, 0, 0, 0);
        result.Specular = float4(0, 0, 0, 0);

        if (Lights[i].LightType == DIRECTIONAL_LIGHT)
        {
            result = DoDirectionalLightNormalMap(Lights[i],pixelToEyeVectorNormalised,N,TBN_Inv);
        }
        else
        {
            float3 pixelToLight = Lights[i].Position.xyz - worldPos.xyz;
            float distanceToLight = length(pixelToLight);

            float3 pixelToLightTS =VectorToTangentSpace(pixelToLight, TBN_Inv);

            if (Lights[i].LightType == POINT_LIGHT)
            {
                result = DoPointLight(Lights[i],normalize(pixelToLightTS),pixelToEyeVectorNormalised,distanceToLight,N);
            }
            else if (Lights[i].LightType == SPOT_LIGHT)
            {
                result = DoSpotLightNormalMap(Lights[i],pixelToLightTS,pixelToEyeVectorNormalised,distanceToLight,N,TBN_Inv);
            }
        }

        totalResult.Diffuse += result.Diffuse;
        totalResult.Specular += result.Specular;
    }

    totalResult.Diffuse = saturate(totalResult.Diffuse);
    totalResult.Specular = saturate(totalResult.Specular);

    return totalResult;
}

LightingResult ComputeLightingNoNormalMap(float4 worldPos, float3 N, float3 pixelToEyeVectorNormalised)
{
    LightingResult totalResult;
    totalResult.Diffuse = float4(0, 0, 0, 0);
    totalResult.Specular = float4(0, 0, 0, 0);

    [unroll]
    for (int i = 0; i < MAX_LIGHTS; ++i)
    {
        if (!Lights[i].Enabled)
            continue;

        LightingResult result;
        result.Diffuse = float4(0, 0, 0, 0);
        result.Specular = float4(0, 0, 0, 0);

        if (Lights[i].LightType == DIRECTIONAL_LIGHT)
        {
            result = DoDirectionalLightNoNormalMap(Lights[i], pixelToEyeVectorNormalised, N);
        }
        else
        {
            float3 pixelToLight = Lights[i].Position.xyz - worldPos.xyz;
            float distanceToLight = length(pixelToLight);


            if (Lights[i].LightType == POINT_LIGHT)
            {
                result = DoPointLight(Lights[i], normalize(pixelToLight), pixelToEyeVectorNormalised, distanceToLight, N);
            }
            else if (Lights[i].LightType == SPOT_LIGHT)
            {
                result = DoSpotLightNoNormalMap(Lights[i], pixelToLight, pixelToEyeVectorNormalised, distanceToLight, N);
            }
        }

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





    float3 T = normalize(mul(input.Tangent, (float3x3)World));
    float3 B = normalize(mul(input.BiNormal, (float3x3) World));
    float3 N = normalize(mul(input.Norm, (float3x3) World));

    float3x3 TBN = float3x3(T, B, N);
    float3x3 TBN_Inv = transpose(TBN);

    output.TBN_Inv = TBN_Inv;


    output.EyeTangentVector = VectorToTangentSpace(vertexToEye, TBN_Inv);


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
        lit = ComputeLightingNormalMap(IN.worldPos, bumpMap.xyz,  normalize(IN.EyeTangentVector),IN.TBN_Inv);
    }
    else
    {
        lit = ComputeLightingNoNormalMap(IN.worldPos, normalize(IN.Norm),  normalize(IN.EyeWorldSpaceVector));
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
float4 PSTextureUnLit(PS_INPUT IN) : SV_TARGET
{

    float4 texColor = float4(1, 1, 1, 1);



    float4 finalColor = Material.Ambient;
    if (Material.UseTexture)
    {
        texColor = txDiffuse.Sample(samLinear, IN.Tex);
        finalColor *= texColor;
    }
    else
    {
        finalColor = float4(0.2, 0.2, 0.2, 1.0f);
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