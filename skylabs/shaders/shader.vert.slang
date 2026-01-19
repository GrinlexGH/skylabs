import main;

struct VertexInput {
    [[vk::location(0)]] float3 position : POSOTION;
    [[vk::location(1)]] float3 color : COLOR;
    [[vk::location(2)]] float2 texCoord : TEXCOORD;
};

[shader("vertex")]
VertexToFragment main(VertexInput input, uint vertexIndex : SV_VertexID) {
    VertexToFragment output;

    float4x4 pvm = mul(ubo.proj, mul(ubo.view, ubo.model));

    float3 pos = input.position;
    if (vertexIndex >= 4) {
        pos += ubo.offset;
    }

    output.outPosition = mul(pvm, float4(pos, 1.0));
    output.fragColor = input.color;
    output.fragTexCoord = input.texCoord;

    return output;
}
