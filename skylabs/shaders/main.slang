struct UniformBufferObject {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
    float3 offset;
};

struct VertexToFragment {
    float4 outPosition : SV_Position;
    [[vk::location(0)]] float3 fragColor;
    [[vk::location(1)]] float2 fragTexCoord;
};

[[vk::binding(0)]]
ConstantBuffer<UniformBufferObject> ubo;
