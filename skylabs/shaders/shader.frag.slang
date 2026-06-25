import main;

struct PushConstants {
    uint32_t textureIndex;
    float4x4 model;
};

[[vk::push_constant]]
PushConstants pc;

[shader("fragment")]
float4 main(VOut input) : SV_Target {
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
