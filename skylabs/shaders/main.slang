module main;

public struct VOut {
    public float4 outPosition : SV_Position;
    public [[vk::location(0)]] float2 uv;
};
