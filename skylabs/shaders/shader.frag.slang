struct UniformBufferObject {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};

[[vk::binding(0)]]
ConstantBuffer<UniformBufferObject> ubo;

[[vk::binding(1)]]
Sampler2D texSampler;

struct VOut {
    float4 outPosition : SV_Position;
    [[vk::location(0)]] float2 uv;
};

[shader("fragment")]
float4 main(VOut input) : SV_Target {
    float4 texColor = texSampler.Sample(input.uv);
    return float4(texColor.rgb, 1.0);
}
