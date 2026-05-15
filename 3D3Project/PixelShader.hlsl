Texture2D myTexture : register(t0);
SamplerState mySampler : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD;
};

float4 PSMain(PSInput input) : SV_TARGET
{
    float4 texColor =
        myTexture.Sample(mySampler, input.texCoord);

    return input.color;
}