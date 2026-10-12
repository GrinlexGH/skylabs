struct VSOutput {
    float4 pos : SV_Position;
    float3 color;
};

static const float2 kPositions[3] = {
    float2( 0.0, -0.5),
    float2( 0.5,  0.5),
    float2(-0.5,  0.5)
};

static const float3 kColors[3] = {
    float3(1.0, 0.0, 0.0),
    float3(0.0, 1.0, 0.0),
    float3(0.0, 0.0, 1.0)
};

struct UBO {
    float4x4 view;
    float4x4 projection;
};

[[vk::binding(0,0)]]
ConstantBuffer<UBO> ubo;

[shader("vertex")]
VSOutput vertexMain(uint index : SV_VulkanVertexID) {
    VSOutput output;
    output.pos = mul(ubo.projection, mul(ubo.view, float4(kPositions[index], 0.0, 1.0)));
    output.color = kColors[index];
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input) : SV_Target {
    return float4(input.color, 1.0);
}
