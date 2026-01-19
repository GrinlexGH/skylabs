[[vk::binding(0)]]
Sampler2D uOut;

[shader("fragment")]
float4 main(float4 pos : SV_Position, [[vk::location(0)]] float2 uv) : SV_Target {
    return uOut.Sample(uv);
}
