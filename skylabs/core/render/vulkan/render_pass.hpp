#pragma once
#include <skylabs/core/render/vulkan/in_flight.hpp>
#include <skylabs/core/render/vulkan/pipeline/pipeline_layout_cache.hpp>
#include <skylabs/core/render/vulkan/pipeline/descriptor_layout_cache.hpp>
#include <skylabs/core/render/vulkan/pipeline/descriptor_allocator.hpp>

namespace Vulkan {
struct CreationTools
{
    const CContext& m_context;
    const CInFlightContext& m_inFlightContext;
    CPipelineLayoutCache& m_pipelineLayoutCache;
    CDescriptorLayoutCache& m_descriptorLayoutCache;
    CDescriptorAllocator& m_descriptorAllocator;
};
}
