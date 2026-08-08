struct VOut {
    float4 pos : SV_Position;
    [[vk::location(0)]] float2 uv;
};

[shader("vertex")]
VOut vertexMain(uint vertexIndex : SV_VertexID) {
    VOut output;

    const float2 positions[3] = {
        float2(-1.0, -1.0),
        float2( 3.0, -1.0),
        float2(-1.0,  3.0)
    };

    output.pos = float4(positions[vertexIndex], 0.0, 1.0);
    output.uv = output.pos.xy * 0.5 + 0.5;

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
    }
    else if (dist <= radius + 1.0) {
        return float4(0.0, 0.0, 0.0, 1.0);
    }

    return sceneColor;
}
