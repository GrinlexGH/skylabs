#pragma once
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <array>

enum class VertexFormat : std::uint8_t {
    Float32,
    Float32x2,
    Float32x3,
    Float32x4,
};

struct CVertexAttribute
{
    VertexFormat m_format;
    std::uint32_t m_offset;
};

struct CVertex
{
    glm::vec3 m_position;
    glm::vec2 m_texCoord;
    glm::vec3 m_normal;

    static constexpr std::array<CVertexAttribute, 3> GetAttributes() {
        return {
            CVertexAttribute { .m_format = VertexFormat::Float32x3, .m_offset = offsetof(CVertex, m_position) },
            CVertexAttribute { .m_format = VertexFormat::Float32x2, .m_offset = offsetof(CVertex, m_texCoord) },
            CVertexAttribute { .m_format = VertexFormat::Float32x3, .m_offset = offsetof(CVertex, m_normal) },
        };
    }

    friend constexpr bool operator==(const CVertex& lhs, const CVertex& rhs) {
        return lhs.m_position == rhs.m_position && lhs.m_texCoord == rhs.m_texCoord && lhs.m_normal == rhs.m_normal;
    }
};

struct CParticle
{
    glm::vec2 m_pos;
    glm::vec2 m_vel;
    glm::vec4 m_col;

    static constexpr std::array<CVertexAttribute, 2> GetAttributes() {
        return {
            CVertexAttribute { .m_format = VertexFormat::Float32x2, .m_offset = offsetof(CParticle, m_pos) },
            CVertexAttribute { .m_format = VertexFormat::Float32x4, .m_offset = offsetof(CParticle, m_col) },
        };
    }
};
