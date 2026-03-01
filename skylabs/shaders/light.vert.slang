struct UniformBufferObject {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
    float3 offset;
};

[[vk::binding(0)]]
ConstantBuffer<UniformBufferObject> ubo;

struct VertexInput {
    [[vk::location(0)]] float3 position : POSOTION;
    [[vk::location(1)]] float2 texCoord : TEXCOORD;
};

[shader("vertex")]
float4 main(VertexInput input, uint vertexIndex : SV_VertexID) : SV_Position {
    float3 pos = input.position;
    if (vertexIndex >= 4) {
        pos += ubo.offset;
    }

    return mul(ubo.proj, mul(ubo.view, mul(ubo.model, float4(pos, 1.0))));
}
