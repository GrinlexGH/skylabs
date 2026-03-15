#pragma once

namespace Vulkan {
class CPipelineLayoutCache
{
public:
    explicit CPipelineLayoutCache(std::nullptr_t) {}
    CPipelineLayoutCache(const CPipelineLayoutCache&) = delete;
    CPipelineLayoutCache(CPipelineLayoutCache&&) noexcept = default;
    CPipelineLayoutCache& operator=(const CPipelineLayoutCache&) = delete;
    CPipelineLayoutCache& operator=(CPipelineLayoutCache&&) noexcept = default;
    ~CPipelineLayoutCache() = default;

};
}
