// HLSL version of Simple.frag

struct PSInput {
    float4 position : SV_POSITION;
    float3 color : COLOR0;
};

float4 main(PSInput input) : SV_TARGET {
    return float4(input.color, 1.0);
} 