struct PushConstants {
    uint32_t textureIndex;
    float4x4 model;
};

[[vk::push_constant]]
PushConstants pc;

struct UniformBufferObject {
    float4x4 view;
    float4x4 proj;
};

struct VIn {
    [[vk::location(0)]] float3 position;
    [[vk::location(1)]] float2 texCoord;
    [[vk::location(2)]] float3 normal;
};

struct VOut {
    float4 outPosition : SV_Position;
    [[vk::location(0)]] float2 uv;
    [[vk::location(1)]] float3 normal;
};

[shader("vertex")]
VOut vertexMain(VIn input, [[vk::binding(0)]] ConstantBuffer<UniformBufferObject> ubo) {
    VOut output;

    output.outPosition = mul(ubo.proj, mul(ubo.view, mul(pc.model, float4(input.position, 1.0))));
    output.uv = input.texCoord;
    output.normal = mul((float3x3)pc.model, input.normal);

    return output;
}

[shader("fragment")]
float4 fragmentMain(VOut input) : SV_Target0 {
    float3 N = normalize(input.normal);
    float3 lightDir = normalize(float3(0.3, 1.0, 0.3));
    float diffuse = max(dot(N, lightDir), 0.0);
    float ambient = 0.2;
    float lighting = ambient + diffuse * 0.8;
    float3 baseColor = float3(0.1, 0.1, 0.1);
    if (pc.textureIndex == 1) {
        baseColor = float3(0.0, 1.0, 1.0);
    } else if (pc.textureIndex == 2) {
        baseColor = float3(1.0, 0.0, 1.0);
    } else if (pc.textureIndex == 3) {
        baseColor = float3(1.0, 1.0, 1.0);
    }
    return float4(baseColor * lighting, 1.0);
}
