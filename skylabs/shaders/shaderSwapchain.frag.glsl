#version 450

layout(binding = 0) uniform sampler2D uOut;

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(uOut, vUV);
}
