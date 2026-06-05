import main;

struct UniformBufferObject {
    float4x4 view;
    float4x4 proj;
};

struct PushConstants {
    uint32_t textureIndex;
    float4x4 model;
};

[[vk::binding(0)]]
ConstantBuffer<UniformBufferObject> ubo;

[[vk::push_constant]]
PushConstants pc;

struct VIn {
    [[vk::location(0)]] float3 position;
    [[vk::location(1)]] float2 texCoord;
};

[shader("vertex")]
VOut main(VIn input, uint vertexIndex : SV_VertexID) {
    VOut output;

    float4 worldPos = float4(input.position, 1.0);
    if (vertexIndex >= 4) {
        worldPos = mul(pc.model, worldPos);
    }

    output.outPosition = mul(ubo.proj, mul(ubo.view, worldPos));
    output.uv = input.texCoord;

    return output;
}
