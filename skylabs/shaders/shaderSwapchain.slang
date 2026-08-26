struct VOut {
    float4 pos : SV_Position;
    [[vk::location(0)]] float2 uv : TEXCOORD0;
};

[shader("vertex")]
VOut vertexMain(uint vertexIndex : SV_VertexID) {
    VOut output;
    output.uv = float2((vertexIndex << 1) & 2, vertexIndex & 2);
    output.pos = float4(output.uv * float2(2.0, 2.0) + float2(-1.0, -1.0), 0.0, 1.0);
    return output;
}

[shader("fragment")]
float4 fragmentMain(VOut input, [[vk::binding(0)]] Sampler2D uOut) : SV_Target {
    float4 sceneColor = uOut.Sample(input.uv);

    uint width, height;
    uOut.GetDimensions(width, height);

    float2 pixelPos = input.uv * float2(width, height);
    float2 centerPixel = float2(width, height) * 0.5;
    float dist = distance(pixelPos, centerPixel);
    float radius = 3.0;
    if (dist <= radius) {
        return float4(0.0, 1.0, 0.0, 1.0);
    } else if (dist <= radius + 1.0) {
        return float4(0.0, 0.0, 0.0, 1.0);
    }

    return sceneColor;
}
