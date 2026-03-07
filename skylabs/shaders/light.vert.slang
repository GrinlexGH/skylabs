struct UniformBufferObject {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};

[[vk::binding(0)]]
ConstantBuffer<UniformBufferObject> ubo;

struct VertexInput {
    [[vk::location(0)]] float3 position : POSITION;
    [[vk::location(1)]] float2 texCoord : TEXCOORD;
};

[shader("vertex")]
float4 main(VertexInput input, uint vertexIndex : SV_VertexID) : SV_Position {
    float4 worldPos = float4(input.position, 1.0);

    worldPos.xyz += input.texCoord.xyx * 0.00000000000001f;

    if (vertexIndex >= 4) {
        worldPos = mul(ubo.model, worldPos);
    }

    return mul(ubo.proj, mul(ubo.view, worldPos));
}
