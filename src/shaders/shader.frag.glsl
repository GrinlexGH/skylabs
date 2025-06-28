#version 450

layout(location = 0) flat in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    if (gl_PointCoord[0] + gl_PointCoord[1] > 0.5) {
        outColor = vec4(fragColor, 1.0);
    } else { outColor = vec4(1.0, 1.0, 1.0, 1.0); }
        outColor = vec4(fragColor, 1.0);
}
