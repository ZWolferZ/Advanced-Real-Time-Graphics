Texture2D txDiffuse : register(t0);
Texture2D DepthTex : register(t1);
Texture2D AlbedoTex : register(t2);
SamplerState samPoint : register(s0);


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

QuadVS_Output QuadVS(QuadVS_Input Input)
{
    // no mvp transform - model coordinates already in projection space (-1 to 1)
    QuadVS_Output Output = (QuadVS_Output) 0;
    Output.Pos = Input.Pos;
    Output.Tex = Input.Tex;
    return Output;
}

// Source - https://stackoverflow.com/q/51108596
// Posted by l1994743
// Retrieved 2026-02-05, License - CC BY-SA 4.0

float linearize_depth(float d, float zNear, float zFar)
{
    float z_n = 2.0 * d - 1.0;
    return 2.0 * zNear * zFar / (zFar + zNear - z_n * (zFar - zNear));
}

float4 QuadPS_Albedo(QuadVS_Output Input) : SV_TARGET
{
    float4 col = AlbedoTex.Sample(samPoint, Input.Tex);
    return col;
}

float4 QuadPS_Depth(QuadVS_Output Input) : SV_TARGET
{
    float d = DepthTex.Sample(samPoint, Input.Tex);
    d = linearize_depth(d, 0.1, 100.0) / 100.0;
    return float4(d, d, d, 1);
}

float4 QuadPS(QuadVS_Output Input) : SV_TARGET
{
    float d = DepthTex.Sample(samPoint, Input.Tex);

    d = linearize_depth(d, 0.1, 100.0) / 100.0;

   // return float4(d, d, d, 1);

    float4 col = txDiffuse.Sample(samPoint, Input.Tex);

    return col;
}

