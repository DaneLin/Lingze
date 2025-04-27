// HLSL version of ImGui.frag

Texture2D fontTexture : register(t0);
SamplerState fontSampler : register(s0);

struct PSInput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
};

float4 main(PSInput input) : SV_TARGET {
    float4 sampled = fontTexture.Sample(fontSampler, input.uv);
    return float4(input.color.rgb, input.color.a * sampled.r);
} 