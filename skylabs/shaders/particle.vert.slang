struct VInput {
    [[vk::location(0)]] float2 position : POSITION;
    [[vk::location(1)]] float4 color    : COLOR;
};

struct VOut {
    float4 pos : SV_Position;
    [[vk::location(0)]] float4 color : COLOR;
    float pointSize : SV_PointSize;
};

[shader("vertex")]
VOut main(VInput input) {
    VOut output;

    output.pos = float4(input.position, 0.0, 1.0);
    output.color = input.color;
    output.pointSize = 4.0;

    return output;
}
