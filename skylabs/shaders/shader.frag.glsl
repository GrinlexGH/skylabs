#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 offset;
} ubo;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main() {
    float Z = 3.0;
    float n = 2.0;
    float pi = radians(180);

    vec2 uv = fragTexCoord * 2.0 - 1.0;
    float x = uv.x * 5.0;
    float y = uv.y * 5.0;
    float z = ubo.offset.z;

    float r = sqrt(x*x + y*y + z*z);

    float rho = 2.0 * Z * r / n;
    float R = (1 / (2 * sqrt(2))) * (2 - rho) * pow(Z, 3 / 2) * exp(-rho / 2);
    float Y = 1 * pow(1 / (4 * pi), 1 / 2);
    float psi = R * Y;
    float intensity = psi * psi;

    float minIntensity = 6.0741607e-08;
    float maxIntensity = 1.0742959;

    intensity = clamp((intensity - minIntensity) / (maxIntensity - minIntensity), 0.0, 1.0);

    vec3 color = mix(vec3(0.0, 0.0, 1.0), vec3(1.0), intensity);
    float alpha = clamp(intensity, 0, 1);

    vec4 texColor = texture(texSampler, fragTexCoord);
    outColor = vec4(texColor.rgb, 1);
}
