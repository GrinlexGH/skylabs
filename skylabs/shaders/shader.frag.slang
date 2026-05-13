import main;

struct PushConstants {
    uint textureIndex;
};

[[vk::push_constant]]
PushConstants pc;

[[vk::binding(1)]]
Sampler2D allTextures[];

[shader("fragment")]
float4 main(VOut input) : SV_Target {
    float4 texColor = allTextures[NonUniformResourceIndex(pc.textureIndex)].Sample(input.uv);
    return float4(texColor.rgb, 1.0);
}
