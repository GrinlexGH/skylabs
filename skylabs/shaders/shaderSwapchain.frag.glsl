#version 450

layout(binding = 0) uniform sampler2D uLowRes;

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

void main()
{
    // семплим низкое разрешение
    vec4 c = texture(uLowRes, vUV);

    // Можно добавить nearest-neighbor для пикселей:
    //   vec2 res = vec2(16.0, 16.0);
    //   vec2 uv2 = floor(vUV * res) / res;
    //   c = texture(uLowRes, uv2);

    outColor = c;
}
