struct UniformBufferObject {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
    float3 offset;
};

[[vk::binding(0)]]
ConstantBuffer<UniformBufferObject> ubo;

[[vk::binding(1)]]
Sampler2D texSampler;

struct VertexInput {
    [[vk::location(0)]] float3 position : POSOTION;
    [[vk::location(1)]] float3 color : COLOR;
    [[vk::location(2)]] float2 texCoord : TEXCOORD;
};

struct VertexToFragment {
    float4 outPosition : SV_Position;
    [[vk::location(0)]] float3 fragColor;
    [[vk::location(1)]] float2 fragTexCoord;
};

[shader("vertex")]
VertexToFragment vertMain(VertexInput input, uint vertexIndex : SV_VertexID) {
    VertexToFragment output;

    float4x4 pvm = mul(ubo.proj, mul(ubo.view, ubo.model));

    float3 pos = input.position;
    if (vertexIndex >= 4) {
        pos += ubo.offset;
    }

    output.outPosition = mul(pvm, float4(pos, 1.0));
    output.fragColor = input.color;
    output.fragTexCoord = input.texCoord;

    return output;
}

[shader("fragment")]
float4 fragMain(VertexToFragment input) : SV_Target {
    const float Z = 3.0;
    const float n = 2.0;
    const float pi = 3.14159265359;

    float2 uv = input.fragTexCoord * 2.0 - 1.0;
    float3 p = float3(uv * 5.0, ubo.offset.z);

    float r = length(p);
    float rho = 2.0 * Z * r / n;

    float R = (1.0 / (2.0 * sqrt(2.0))) * (2.0 - rho) * pow(Z, 1.5) * exp(-rho / 2.0);
    float Y = sqrt(1.0 / (4.0 * pi));
    float psi = R * Y;
    float intensity = psi * psi;

    float minIntensity = 6.0741607e-08;
    float maxIntensity = 1.0742959;
    intensity = saturate((intensity - minIntensity) / (maxIntensity - minIntensity));

    float3 color = lerp(float3(0.0, 0.0, 1.0), float3(1.0), intensity);

    float4 texColor = texSampler.Sample(input.fragTexCoord);
    return float4(texColor.rgb, 1.0);
}
