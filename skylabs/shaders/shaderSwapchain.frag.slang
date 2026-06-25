[[vk::binding(0)]]
Sampler2D uOut;

struct VOut {
    float4 pos : SV_Position;
    [[vk::location(0)]] float2 uv;
};

[shader("fragment")]
float4 main(VOut input) : SV_Target {
    float4 sceneColor = uOut.Sample(input.uv);

    uint width, height;
    uOut.GetDimensions(width, height);

    float2 pixelPos = input.uv * float2(width, height);
    float2 centerPixel = float2(width, height) * 0.5;

    float dist = distance(pixelPos, centerPixel);

    float radius = 3.0;

    if (dist <= radius) {
        return float4(0.0, 1.0, 0.0, 1.0);
    }
    else if (dist <= radius + 1.0) {
        return float4(0.0, 0.0, 0.0, 1.0);
    }

    return sceneColor;
}
