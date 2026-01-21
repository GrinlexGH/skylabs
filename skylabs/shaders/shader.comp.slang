WTexture2D<float4> outImage;

[shader("compute")]
[numthreads(8, 8, 1)]
void main(uint3 threadId: SV_DispatchThreadID, uint3 groupId: SV_GroupID) {
    uint2 pixel = threadId.xy;
    uint2 size;
    outImage.GetDimensions(size.x, size.y);

    if (pixel.x >= size.x || pixel.y >= size.y)
        return;

    float4 color = float4(groupId.x / 32.0f, groupId.y / 32.0f, 0.0f, 1.0f);

    outImage.Store(pixel, color);
}
