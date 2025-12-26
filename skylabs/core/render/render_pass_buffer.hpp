#pragma once
#include <cstddef>
#include <cstdint>

struct CRPBufferDescription
{

};

struct CRPBuffer
{
    std::size_t m_size = 0;
    std::uint32_t m_handle = 0xFFFFFFFF;
};
