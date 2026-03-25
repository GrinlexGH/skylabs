[shader("fragment")]
float4 main([[vk::location(0)]] float4 color : COLOR, float2 pointCoord : SV_PointCoord) : SV_Target {
    float dist = distance(pointCoord, float2(0.5, 0.5));

    float alpha = smoothstep(0.5, 0.4, dist);
    return float4(color.rgb, color.a * alpha);
}