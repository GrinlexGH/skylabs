import main;

struct VertexInput {
    [[vk::location(0)]] float3 position : POSOTION;
    [[vk::location(1)]] float2 texCoord : TEXCOORD;
};

[shader("vertex")]
VertexToFragment main(VertexInput input, uint vertexIndex : SV_VertexID) {
    VertexToFragment output;

    float3 pos = input.position;
    if (vertexIndex >= 4) {
        pos += ubo.offset;
    }

    float4 worldPos = mul(ubo.model, float4(pos, 1.0));

    // Обычный рендеринг камеры
    output.outPosition = mul(ubo.proj, mul(ubo.view, worldPos));
    output.fragTexCoord = input.texCoord;

    // Вычисляем координаты для теней
    float4 shadowPos = mul(ubo.lightproj, mul(ubo.lightview, worldPos));;

    output.shadowCoord = shadowPos;

    return output;
}
