struct UniformBufferObject {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
    float4x4 lightproj;
    float4x4 lightview;
};

struct VertexToFragment {
    float4 outPosition : SV_Position;
    [[vk::location(0)]] float2 fragTexCoord;
    [[vk::location(1)]] float4 shadowCoord : TEXCOORD1;
};

[[vk::binding(0)]]
ConstantBuffer<UniformBufferObject> ubo;
