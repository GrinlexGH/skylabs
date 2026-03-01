import main;

[[vk::binding(1)]]
Sampler2D texSampler;

[[vk::binding(2)]]
Sampler2D shadowSampler;

[shader("fragment")]
float4 main(VertexToFragment input) : SV_Target {
    float4 texColor = texSampler.Sample(input.fragTexCoord);
    float3 projCoords = input.shadowCoord.xyz / input.shadowCoord.w;
    float2 uv = projCoords.xy * 0.5 + 0.5;

    const float ambient = 0.1;

    if (uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1 || projCoords.z < 0) {
        return float4(texColor.rgb * ambient, 1.0); // Делаем область темной
    }

    float shadow = 0.0;
    const float2 texelSize = 1.0 / 2048.0;
    const float bias = 0.0005;

    [unroll]
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            shadow += shadowSampler.SampleCmp(uv + float2(x, y) * texelSize, projCoords.z + bias);
        }
    }
    shadow /= 9.0;

    return float4(texColor.rgb * max(shadow, ambient), 1.0);
}
