#version 450
#extension GL_KHR_vulkan_glsl: enable

layout(location = 0) flat out vec3 fragColor;

// Геометрические параметры
const float left   = -0.9;
const float right  =  0.9;
const float top    =  0.9;
const float bottom = -0.9;
const float margin = 0.1;

const float innerTop    = top - margin;
const float innerBottom = bottom + margin;
const float innerHeight = innerTop - innerBottom;
const float stripeHeight = innerHeight / 3.0;

// Вершины для triangle strip (3 прямоугольника подряд)
vec2 positions[12] = vec2[](
    // Белая полоса
    vec2(left + margin, innerTop),                         // 0 top-left
    vec2(right - margin, innerTop),                        // 1 top-right
    vec2(left + margin, innerTop - stripeHeight),          // 2 bottom-left
    vec2(right - margin, innerTop - stripeHeight),         // 3 bottom-right

    // Синяя полоса
    vec2(left + margin, innerTop - stripeHeight),          // 4 top-left (same as 2)
    vec2(right - margin, innerTop - stripeHeight),         // 5 top-right (same as 3)
    vec2(left + margin, innerTop - 2.0 * stripeHeight),    // 6 bottom-left
    vec2(right - margin, innerTop - 2.0 * stripeHeight),   // 7 bottom-right

    // Красная полоса
    vec2(left + margin, innerTop - 2.0 * stripeHeight),    // 8 top-left (same as 6)
    vec2(right - margin, innerTop - 2.0 * stripeHeight),   // 9 top-right (same as 7)
    vec2(left + margin, innerBottom),                      // 10 bottom-left
    vec2(right - margin, innerBottom)                     // 11 bottom-right
);

vec3 colors[12] = vec3[](
    // Красная
    vec3(1.0, 0.0, 0.0),
    vec3(1.0, 0.0, 0.0),
    vec3(1.0, 0.0, 0.0),
    vec3(1.0, 0.0, 0.0),

    // Синяя
    vec3(0.0, 0.0, 1.0),
    vec3(0.0, 0.0, 1.0),
    vec3(0.0, 0.0, 1.0),
    vec3(0.0, 0.0, 1.0),

    // Белая
    vec3(1.0, 1.0, 1.0),
    vec3(1.0, 1.0, 1.0),
    vec3(1.0, 1.0, 1.0),
    vec3(1.0, 1.0, 1.0)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
}
