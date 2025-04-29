struct VertexOutput
{
    float4 position : SV_Position;
};

[outputtopology("triangle")]
[numthreads(1, 1, 1)]
void main(out indices uint3 triangles[1], out vertices VertexOutput vertices[3], uint3 DispatchThreadID : SV_DispatchThreadID)
{
    SetMeshOutputCounts(3, 1); 
    vertices[0].position = float4(0.5, -0.5, 0, 1);
    vertices[1].position = float4(0.5, 0.5, 0, 1);
    vertices[2].position = float4(-0.5, 0.5, 0, 1);    
    triangles[0] = uint3(0, 1, 2);
}