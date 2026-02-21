import main;

[[vk::binding(1)]]
Sampler2D texSampler;

[shader("fragment")]
float4 main(VertexToFragment input) : SV_Target {
    float4 texColor = texSampler.Sample(input.fragTexCoord);
    return float4(texColor.rgb, 1.0);
}
