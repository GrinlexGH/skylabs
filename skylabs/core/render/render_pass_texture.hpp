#pragma once
#include <cstdint>
#include <string>

enum class CRPTextureUsage : std::uint8_t
{
    eColor,
    eDepth,
};

struct CRPTextureDescription
{
    CRPTextureUsage m_usage = CRPTextureUsage::eColor;
    std::uint32_t m_width = 0;
    std::uint32_t m_height = 0;
    std::string m_materialDescription {};
};

struct CRPTexture
{
    std::uint32_t m_handle = 0xFFFFFFFF;
};
