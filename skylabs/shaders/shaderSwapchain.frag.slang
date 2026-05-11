[[vk::binding(0)]]
Sampler2D uOut;

struct VOut {
    float4 pos : SV_Position;
    [[vk::location(0)]] float2 uv;
};

static const float m1 = 2610.0 / 4096.0 / 4.0;
static const float m2 = 2523.0 / 4096.0 * 128.0;
static const float c1 = 3424.0 / 4096.0;
static const float c2 = 2413.0 / 4096.0 * 32.0;
static const float c3 = 2392.0 / 4096.0 * 32.0;

float3 LinearToPQ(float3 color, float maxNits) {
    float3 L = color * (maxNits / 10000.0);
    float3 Lp = pow(abs(L), m1);
    float3 num = c1 + c2 * Lp;
    float3 den = 1.0 + c3 * Lp;
    return pow(num / den, m2);
}

[shader("fragment")]
float4 main(VOut input) : SV_Target {
    float4 texColor = uOut.Sample(input.uv);
    float3 linearRGB = texColor.rgb;
    float sdrPaperWhite = 200.0;
    float3 hdrColor = LinearToPQ(linearRGB, sdrPaperWhite);
    return float4(hdrColor, 1.0);
}
