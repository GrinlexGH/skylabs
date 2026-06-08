import main;

struct PushConstants {
    uint32_t textureIndex;
    float4x4 model;
};

[[vk::push_constant]]
PushConstants pc;

[shader("fragment")]
float4 main(VOut input) : SV_Target {;
    return float4(1.0, 1.0, 1.0, 1.0);
}
