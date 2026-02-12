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
Texture2D NormalBuffer : register(t2);
Texture2D DepthBuffer : register(t3);
Texture2D WorldPosBuffer : register(t4);
Texture2D AlbedoBuffer : register(t5);
Texture2D LightAccBuffer : register(t6);
Texture2D DepthTex : register(t7);
Texture2D ForwardRenderedTexture : register(t8);
SamplerState samLinear : register(s0);

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

cbuffer PostProcessProperties : register(b2)
{
    float4 PostProcessColor;
    float Brightness;
    uint GrayScaleMode;
    uint BlurMode;
    int BlurSteps;
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

cbuffer LightProperties : register(b9)
{
    float4 EyePosition; // 16 bytes
//----------------------------------- (16 byte boundary)
    float4 GlobalAmbient; // 16 bytes
//----------------------------------- (16 byte boundary)
    uint LightCount;
    float3 _Padding; // align to 16 bytes
};

StructuredBuffer<Light> Lights : register(t9); // Put the lights in a structured buffer so I can make them at runtime.

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

struct PS_OUTPUT
{
    float4 Normal : SV_Target0;
    float4 WorldPos : SV_Target1;
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

    for (uint i = 0; i < LightCount; ++i)
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

            float3 pixelToLightTS = VectorToTangentSpace(pixelToLight, TBN_Inv);

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

    for (uint i = 0; i < LightCount; ++i)
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

    output.EyeWorldSpaceVector = vertexToEye;

    return output;


}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

float4 PS(PS_INPUT IN) : SV_TARGET
{
    float4 texColor = float4(1, 1, 1, 1);


    if (Material.UseTexture)
    {
        texColor = txDiffuse.Sample(samLinear, IN.Tex);
        if (texColor.a < 0.1f)
        {
            discard;
        }
    }


    LightingResult lit;

    if (Material.UseNormalMap)
    {
        float4 bumpMap = txNormalMap.Sample(samLinear, IN.Tex);
        bumpMap = (bumpMap * 2.0f) - 1.0f;
        bumpMap = float4(normalize(bumpMap.xyz), 1);
        lit = ComputeLightingNormalMap(IN.worldPos, bumpMap.xyz, normalize(IN.EyeTangentVector), IN.TBN_Inv);
    }
    else
    {
        lit = ComputeLightingNoNormalMap(IN.worldPos, normalize(IN.Norm), normalize(IN.EyeWorldSpaceVector));
    }



    float4 emissive = Material.Emissive;
    float4 ambient = Material.Ambient * GlobalAmbient;
    float4 diffuse = Material.Diffuse * lit.Diffuse;
    float4 specular = Material.Specular * lit.Specular;
    float4 finalColor = (emissive + ambient + diffuse + specular) * texColor;



    return finalColor;


}




float4 PSWriteAlbedoBuffer(PS_INPUT IN) : SV_TARGET
{

    float4 texColor;
    if (Material.UseTexture)
    {
        texColor = txDiffuse.Sample(samLinear, IN.Tex);
        
        texColor *= Material.Diffuse;
        
        if (texColor.a < 0.1f)
        {
            discard;
        }
    }
    else
    {
        texColor = Material.Diffuse;
    }


    return texColor;
}


float LinearizeDepth(float depth, float nearZ, float farZ)
{
    return (nearZ * farZ) / (farZ - depth * (farZ - nearZ));
}

PS_OUTPUT PSWriteGBuffer(PS_INPUT IN)
{
    PS_OUTPUT output;

    float3 N_ws;

    if (Material.UseNormalMap)
    {
        float3 bumpTS = txNormalMap.Sample(samLinear, IN.Tex).xyz * 2.0f - 1.0f;

        N_ws = normalize(mul(bumpTS, transpose(IN.TBN_Inv)));
    }
    else
    {
        N_ws = normalize(IN.Norm);
    }



    output.Normal = float4(N_ws * 0.5f + 0.5f, 1.0f);
    output.WorldPos = IN.worldPos;


    return output;
}




float4 PSTextureUnLit(PS_INPUT IN) : SV_TARGET
{
    float4 finalColor;
    if (Material.UseTexture)
    {
        float4 texColor = txDiffuse.Sample(samLinear, IN.Tex);
        finalColor = texColor;
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

struct QuadVS_Input
{
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD0;
};

struct QuadVS_Output
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
};

float4 QuadPSForwardRendered(QuadVS_Output IN) : SV_TARGET
{
    float4 scene = ForwardRenderedTexture.Sample(samLinear, IN.Tex);
    
    if (BlurMode == 1.0f)
    {
        float3 blur = float3(0, 0, 0);
        float tex_width, tex_height;
        ForwardRenderedTexture.GetDimensions(tex_width, tex_height);
        float2 uv_scale = 1.0f / float2(tex_width, tex_height);
	 
        float scalar = 1;
        float scalar_sum = 0;
        for (int i = -BlurSteps + 1; i < BlurSteps; i++)
        {
            float scalar_tmp = scalar;
            for (int j = -BlurSteps + 1; j < BlurSteps; j++)
            {
                blur += ForwardRenderedTexture.Sample(samLinear, IN.Tex + float2(i * uv_scale.x, j * uv_scale.y)).rgb * scalar_tmp;
                scalar_sum += scalar_tmp;
                if (j <= 0)
                    scalar_tmp *= 2.0f;
                else
                    scalar_tmp /= 2.0f;
            }
            if (i <= 0)
                scalar *= 2.0f;
            else
                scalar /= 2.0f;
        }
        float actual_radius = (BlurSteps * 2) + 1;
	 
        blur /= scalar_sum;
   
        scene.rgb = blur;
    }
    
    scene *= PostProcessColor;
    
    scene *= Brightness;
    
    if (GrayScaleMode == 1.0f)
    {
        scene.rgb = dot(scene.rgb, float3(0.3, 0.59, 0.11));
    }
    
    return scene;
}

QuadVS_Output QuadVS(QuadVS_Input Input)
{
    // no mvp transform - model coordinates already in projection space (-1 to 1)
    QuadVS_Output Output = (QuadVS_Output) 0;
    Output.Pos = Input.Pos;
    Output.Tex = Input.Tex;
    return Output;
}



float4 ComputeScene(float2 uv)
{

    float4 light = LightAccBuffer.Sample(samLinear, uv);

    float4 albedo = AlbedoBuffer.Sample(samLinear, uv);
    float4 scene = albedo * light;
    
    scene *= PostProcessColor;
    
    scene *= Brightness;
    
    return scene;
}



float4 QuadPS(QuadVS_Output IN) : SV_TARGET
{
    float2 uv = IN.Tex;
    
    float4 scene = ComputeScene(uv);
    if (BlurMode == 1.0f)
    {
        float3 blur = float3(0, 0, 0);
        float tex_width, tex_height;
        AlbedoBuffer.GetDimensions(tex_width, tex_height);
        float2 uv_scale = 1.0f / float2(tex_width, tex_height);
	 
        float scalar = 1;
        float scalar_sum = 0;
        for (int i = -BlurSteps + 1; i < BlurSteps; i++)
        {
            float scalar_tmp = scalar;
            for (int j = -BlurSteps + 1; j < BlurSteps; j++)
            {
                blur += ComputeScene(uv + float2(i * uv_scale.x, j * uv_scale.y)).rgb * scalar_tmp;
                scalar_sum += scalar_tmp;
                if (j <= 0)
                    scalar_tmp *= 2.0f;
                else
                    scalar_tmp /= 2.0f;
            }
            if (i <= 0)
                scalar *= 2.0f;
            else
                scalar /= 2.0f;
        }
        
        float actual_radius = (BlurSteps * 2) + 1;
	 
        blur /= scalar_sum;
        
        scene.rgb = blur;
    }
    
    if (GrayScaleMode == 1.0f)
    {
        scene.rgb = dot(scene.rgb, float3(0.3, 0.59, 0.11));

    } 
    
    return scene;
}

float4 PS_VisualiseDepth(QuadVS_Output IN) : SV_Target
{
    float depth = DepthBuffer.Sample(samLinear, IN.Tex).r;
    depth = LinearizeDepth(depth, 0.1f, 100.0f) / 100.0f;
    return float4(depth, depth, depth, 1);
}


float4 PS_DeferredLighting(QuadVS_Output IN) : SV_TARGET
{
    float3 N = NormalBuffer.Sample(samLinear, IN.Tex).xyz * 2.0f - 1.0f;
    float3 worldPos = WorldPosBuffer.Sample(samLinear, IN.Tex).xyz;

    float3 diffuseLighting = float3(0.03f, 0.03f, 0.03f);
    float3 ambient = GlobalAmbient.rgb * Material.Ambient.rgb;
    float3 specularLighting = float3(0, 0, 0);

    float3 pixelToEyeVector = normalize(EyePosition.xyz - worldPos);

    for (uint i = 0; i < LightCount; ++i)
    {
        if (!Lights[i].Enabled)
            continue;

        float3 lightDir;
        float attenuation = 1.0f;

        if (Lights[i].LightType == DIRECTIONAL_LIGHT)
        {
            lightDir = normalize(-Lights[i].Direction.xyz);
        }
        else
        {
            float3 toLight = Lights[i].Position.xyz - worldPos;
            float distance = length(toLight);
            lightDir = normalize(toLight);

            attenuation = 1.0f / (Lights[i].ConstantAttenuation + Lights[i].LinearAttenuation * distance + Lights[i].QuadraticAttenuation * distance * distance);

            if (Lights[i].LightType == SPOT_LIGHT)
            {
                float3 spotDir = normalize(-Lights[i].Direction.xyz);
                float spotFactor = dot(lightDir, spotDir);
                if (spotFactor < Lights[i].SpotAngle)
                    continue;
                attenuation *= saturate((spotFactor - Lights[i].SpotAngle) / (1.0f - Lights[i].SpotAngle));
            }
        }

        float diff = max(dot(N, lightDir), 0.0f);
        diffuseLighting += Lights[i].Color.rgb * diff * attenuation;

        float lightIntensity = saturate(dot(N, lightDir));
        if (lightIntensity > 0.0f)
        {
            float3 reflection = reflect(-lightDir, N);
            float specular = pow(saturate(dot(reflection, pixelToEyeVector)), Material.SpecularPower);
            specularLighting += Lights[i].Color.rgb * specular * attenuation;
        }
    }

    float3 finalLighting =  ambient + diffuseLighting + (specularLighting * Material.Specular.rgb);

    return float4(finalLighting, 1.0f);
}


