struct VSOutput {
    float4 pos : SV_Position;
    float2 uv;
};

static const float2 kPositions[3] = {
    float2( 0.0, -0.5),
    float2( 0.5,  0.5),
    float2(-0.5,  0.5)
};

static const float2 kUVs[3] = {
    float2(0.5, 1.0),
    float2(1.0, 0.0),
    float2(0.0, 0.0)
};

struct UBO {
    float4x4 view;
    float4x4 projection;
};

[[vk::binding(0,0)]]
ConstantBuffer<UBO> ubo;

[[vk::binding(1,0)]]
Sampler2D texture;

[shader("vertex")]
VSOutput vertexMain(uint index : SV_VulkanVertexID) {
    VSOutput output;
    output.pos = mul(ubo.projection, mul(ubo.view, float4(kPositions[index], 0.0, 1.0)));
    output.uv = (kUVs[index] - float2(0.5, 0.5)) * 4 + float2(0.5, 0.5);
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input) : SV_Target {
    return texture.Sample(input.uv);
}
