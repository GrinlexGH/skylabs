[[vk::binding(0)]]
Sampler2D uOut;

[[vk::binding(1)]]
Sampler2D uCompute;

[shader("fragment")]
float4 main(float4 pos: SV_Position, [[vk::location(0)]] float2 uv) : SV_Target {
    float4 colorMain = uOut.Sample(uv);
    float4 colorCompute = uCompute.Sample(uv);

    float3 finalRGB = colorCompute.rgb * colorCompute.a + colorMain.rgb * (1.0 - colorCompute.a);
    
    // Альфа итогового кадра обычно 1.0 (непрозрачный экран)
    return float4(finalRGB, 1.0);
}
