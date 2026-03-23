[[vk::binding(0)]]
Sampler2D uOut;

[[vk::binding(1)]]
Sampler2D uCompute;

[shader("fragment")]
float4 main(float4 pos: SV_Position, [[vk::location(0)]] float2 uv) : SV_Target {
    float4 colorMain = uOut.Sample(uv);
    float4 colorCompute = uCompute.Sample(uv);

    float4 result = lerp(colorMain, colorCompute, 0.01);

    return result;
}
