// HLSL version of BasicShape.frag

struct PSInput {
    float4 position : SV_POSITION;
    float3 fragColor : COLOR0;
    float2 uv : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET {
    return float4(input.fragColor, 1.0);
} 