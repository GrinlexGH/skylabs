#pragma once
#include <glm/glm.hpp>

enum class VertexFormat : std::uint8_t {
    eFloat32,
    eFloat32x2,
    eFloat32x3,
    eFloat32x4,
};

struct VertexAttribute {
    VertexFormat format;
    std::uint32_t offset;
};

struct Vertex {
    glm::vec3 position;
    glm::vec2 texCoord;
    glm::vec3 normal;

    static constexpr std::array<VertexAttribute, 3> GetAttributes() {
        return {
            VertexAttribute { .format = VertexFormat::eFloat32x3, .offset = offsetof(Vertex, position) },
            VertexAttribute { .format = VertexFormat::eFloat32x2, .offset = offsetof(Vertex, texCoord) },
            VertexAttribute { .format = VertexFormat::eFloat32x3, .offset = offsetof(Vertex, normal) },
        };
    }

    friend constexpr bool operator==(const Vertex& lhs, const Vertex& rhs) {
        return lhs.position == rhs.position && lhs.texCoord == rhs.texCoord && lhs.normal == rhs.normal;
    }
};

struct Particle {
    glm::vec2 pos;
    glm::vec2 vel;
    glm::vec4 col;

    static constexpr std::array<VertexAttribute, 2> GetAttributes() {
        return {
            VertexAttribute { .format = VertexFormat::eFloat32x2, .offset = offsetof(Particle, pos) },
            VertexAttribute { .format = VertexFormat::eFloat32x4, .offset = offsetof(Particle, col) },
        };
    }
};
