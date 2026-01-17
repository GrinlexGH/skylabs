[[vk::binding(0)]]
Sampler2D uOut;

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
float4 fragmentMain(float4 pos : SV_Position, [[vk::location(0)]] float2 uv) : SV_Target {
    return uOut.Sample(uv);
}
