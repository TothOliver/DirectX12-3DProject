cbuffer TransformBuffer : register(b0)
{
    float4x4 WorldViewProjection;
}

struct VSInput
{
    float3 position : POSITION;
    float4 color : COLOR;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PSInput VSMain(VSInput input)
{
    PSInput output;

    output.position = mul(WorldViewProjection, float4(input.position, 1.0f));
    output.color = input.color;

    return output;
}