struct VOut {
    float4 pos : SV_Position;
    [[vk::location(0)]] float3 color : COLOR0;
};

static const float2 kPositions[3] = {
    float2( 0.0, -0.5),
    float2( 0.5,  0.5),
    float2(-0.5,  0.5)
};

static const float3 kColors[3] = {
    float3(1.0, 0.0, 0.0),
    float3(0.0, 1.0, 0.0),
    float3(0.0, 0.0, 1.0)
};

struct ViewProjection {
    float4x4 view;
    float4x4 projection;
};

[shader("vertex")]
VOut vertexMain(uint vertexIndex : SV_VertexID, ConstantBuffer<ViewProjection> viewProjection) {
    VOut output;
    output.pos =
        mul(viewProjection.projection,
            mul(viewProjection.view, float4(kPositions[vertexIndex], 0.0, 1.0)));
    output.color = kColors[vertexIndex];
    return output;
}

[shader("fragment")]
float4 fragmentMain(VOut input) : SV_Target {
    return float4(input.color, 1.0);
}
