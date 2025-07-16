#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main() {
    vec2 center = vec2(0.5, 0.5);
    float radius = 0.2;
    float dist = distance(fragTexCoord, center);

    vec4 texColor = texture(texSampler, fragTexCoord);

    if (dist <= radius) {
        float luminance = dot(texColor.rgb, vec3(0.299, 0.587, 0.114));
        outColor = vec4(vec3(luminance), texColor.a);
    } else {
        outColor = texColor;
    }
}
