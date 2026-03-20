#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>
#include <skylabs/core/render/vulkan/command_buffer.hpp>

namespace Vulkan {
struct CommandBuffersCount
{
    std::uint32_t m_primaryCount = 0;
    std::uint32_t m_secondaryCount = 0;
};

class CCommandBufferSet
{
public:
    explicit CCommandBufferSet(std::nullptr_t) {}
    explicit CCommandBufferSet(const CContext& context, std::uint32_t familyIndex, CommandBuffersCount buffersCount = {});
    CCommandBufferSet(const CCommandBufferSet&) = delete;
    CCommandBufferSet(CCommandBufferSet&&) noexcept = default;
    CCommandBufferSet& operator=(const CCommandBufferSet&) = delete;
    CCommandBufferSet& operator=(CCommandBufferSet&&) noexcept = default;
    ~CCommandBufferSet() = default;

    const std::vector<CCommandBuffer>& PrimaryBuffers() const { return m_primaryBuffersWrap; }
    const std::vector<CCommandBuffer>& SecondaryBuffers() const { return m_secondaryBuffersWrap; }

    void Realloc(CommandBuffersCount newBufferCount);

private:
    const CContext* m_context = nullptr;
    vk::raii::CommandPool m_pool { nullptr };
    vk::raii::CommandBuffers m_primaryBuffers { nullptr };
    vk::raii::CommandBuffers m_secondaryBuffers { nullptr };
    std::vector<CCommandBuffer> m_primaryBuffersWrap {};
    std::vector<CCommandBuffer> m_secondaryBuffersWrap {};
};
}
