import main;

struct VertexInput {
    [[vk::location(0)]] float3 position : POSOTION;
    [[vk::location(1)]] float2 texCoord : TEXCOORD;
};

[shader("vertex")]
VertexToFragment main(VertexInput input, uint vertexIndex : SV_VertexID) {
    VertexToFragment output;

    float4 worldPos = float4(input.position, 1.0);
    if (vertexIndex >= 4) {
        worldPos = mul(ubo.model, worldPos);
    }

    output.outPosition = mul(ubo.proj, mul(ubo.view, worldPos));
    output.fragTexCoord = input.texCoord;

    float4 shadowPos = mul(ubo.lightproj, mul(ubo.lightview, worldPos));

    output.shadowCoord = shadowPos;

    return output;
}
