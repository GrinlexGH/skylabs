[[vk::binding(0)]]
Sampler2D uOut;

struct VOut {
    float4 pos : SV_Position;
    [[vk::location(0)]] float2 uv;
};

[shader("fragment")]
float4 main(VOut input) : SV_Target {
    return uOut.Sample(input.uv);
}
