struct PushConstants {
    uint textureIndex;
};

[[vk::push_constant]]
PushConstants pc;

[[vk::binding(1)]]
Sampler2D allTextures[];

struct VOut {
    float4 outPosition : SV_Position;
    [[vk::location(0)]] float2 uv : TEXCOORD0;
};

[shader("fragment")]
float4 main(VOut input) : SV_Target {
    float4 texColor = allTextures[NonUniformResourceIndex(pc.textureIndex)].Sample(input.uv);
    return float4(texColor.rgb, 1.0);
}
