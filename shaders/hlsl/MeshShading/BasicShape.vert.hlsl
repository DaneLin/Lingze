// HLSL version of MeshShading/BasicShape.vert

cbuffer UboData : register(b0) {
    matrix view;
    matrix projection;
};

cbuffer DrawCallData : register(b1) {
    matrix model;
};

struct VSInput {
    float3 position : POSITION;
    float3 color : COLOR0;
    float2 uv : TEXCOORD0;
};

struct PSInput {
    float4 position : SV_POSITION;
    float3 fragColor : COLOR0;
    float2 uv : TEXCOORD0;
};

PSInput main(VSInput input) {
    PSInput output;
    float4 pos = float4(input.position, 1.0);
    output.position = mul(mul(mul(projection, view), model), pos);
    output.fragColor = input.color;
    output.uv = input.uv;
    return output;
}
