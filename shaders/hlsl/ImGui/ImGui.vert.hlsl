// HLSL version of ImGui.vert

cbuffer ImGuiShaderData : register(b0) {
    matrix projMatrix;
};

struct VSInput {
    float2 position : POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
};

struct PSInput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
};

PSInput main(VSInput input) {
    PSInput output;
    output.uv = input.uv;
    // imgui colors are srgb space
    output.color = float4(pow(input.color.rgb, float3(2.2, 2.2, 2.2)), input.color.a);
    output.position = mul(projMatrix, float4(input.position, 0.0, 1.0));
    return output;
} 