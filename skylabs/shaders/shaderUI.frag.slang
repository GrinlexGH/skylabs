[[vk::binding(0)]] Sampler2D uOut;
[[vk::binding(1)]] Sampler2D uText;

struct PushConstants {
    float2 screenRes;
    float2 textRes;
};
[[vk::push_constant]] PushConstants pc;

struct VOut {
    float4 pos : SV_Position;
    [[vk::location(0)]] float2 uv;
};

[shader("fragment")]
float4 main(VOut input) : SV_Target {
    float4 sceneColor = uOut.Sample(input.uv);
    float2 pixelPos = input.uv * pc.screenRes;

    if (pixelPos.x < pc.textRes.x && pixelPos.y < pc.textRes.y) {
        float2 textUV = pixelPos / pc.textRes;
        float4 rawColor = uText.Sample(textUV);
        float3 finalRGB = lerp(sceneColor.rgb, rawColor.bgr, rawColor.a);
        return float4(finalRGB, 1.0);
    }

    return sceneColor;
}
