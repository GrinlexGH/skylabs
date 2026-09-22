#pragma once
#include <cstdint>
#include <vector>

#include <VkBootstrap.h>
#include <glm/ext/matrix_float4x4.hpp>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

#include "skylabs/base/filesystem.hpp"
#include "skylabs/base/window.hpp"
#include "skylabs/core/render/render_object.hpp"
#include "skylabs/core/render/renderer.hpp"
#include "skylabs/core/render/vertex.hpp"
#include "skylabs/core/render/vulkan/command_recording/command_buffer_allocator.hpp"
#include "skylabs/core/render/vulkan/context/context.hpp"
#include "skylabs/core/render/vulkan/in_flight.hpp"
#include "skylabs/core/render/vulkan/main_pass.hpp"
#include "skylabs/core/render/vulkan/pipeline/descriptor_allocator.hpp"
#include "skylabs/core/render/vulkan/pipeline/descriptor_layout_cache.hpp"
#include "skylabs/core/render/vulkan/pipeline/pipeline_layout_cache.hpp"
#include "skylabs/core/render/vulkan/post_process_pass.hpp"
#include "skylabs/core/render/vulkan/render_object.hpp"
#include "skylabs/core/render/vulkan/resources/image.hpp"
#include "skylabs/core/render/vulkan/swapchain.hpp"
#include "vulkan/os_connector.hpp"

namespace sk::render::vulkan {
class Renderer final : public IRenderer {
public:
    explicit Renderer(const IWindow* window, const sk::vulkan::IOSConnector* osConnector,
                      const filesystem::Filesystem& filesystem);
    Renderer(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer& operator=(Renderer&&) = delete;
    ~Renderer() override;

    void Draw(glm::mat4 viewMat, float fov, float deltaTime) override;

    std::uint32_t UploadMesh(const std::vector<Vertex>& vertices,
                             const std::vector<std::uint16_t>& indices);

    sk::RenderObject& GetObjectData(std::uint32_t id);
    const sk::RenderObject& GetObjectData(std::uint32_t id) const;
    RenderObject UploadGameObject(std::uint32_t meshId, const glm::mat4& matrix, std::uint16_t colorID);

    void OnDeviceLost();
    void OnPossiblyWindowSizeChange();

private:
    static constexpr auto kFramesInFlightCount = 3;
    static constexpr auto kGeometryPoolSize = static_cast<vk::DeviceSize>(128 * 1024 * 1024);

    void RecreateSwapchain();
    void ResizeTextures();
    void LoadTextures();
    void LoadModels();
    void LoadObjects();

    void UpdateMVP(const glm::mat4& view, float fov);

    const filesystem::Filesystem* m_filesystem = nullptr;

    Context m_context { nullptr };

    Swapchain m_swapchain { nullptr };
    InFlightContext m_inFlightContext;

    PipelineLayoutCache m_pipelineLayoutCache { nullptr };
    DescriptorLayoutCache m_descriptorLayoutCache { nullptr };
    DescriptorAllocator m_descriptorAllocator { nullptr };

    CommandBufferAllocator m_commandBufferAllocator { nullptr };
    InFlight<CommandBuffer> m_graphicsCmd { nullptr };

    InFlight<bool> m_firstUse { nullptr };
    InFlight<vk::raii::Fence> m_fence { nullptr };
    InFlight<vk::raii::Semaphore> m_imageAvailableSemaphore { nullptr };

    std::vector<vk::raii::Semaphore> m_renderFinishedSemaphores;

    Buffer m_stagingBuffer { nullptr };

    Buffer m_vertexBuffer { nullptr };
    Buffer m_indexBuffer { nullptr };

    std::vector<Image> m_meshTextures;
    std::vector<SubMesh> m_meshes;

    MainPass m_mainPass { nullptr };
    PostProcessPass m_postProcessPass { nullptr };

    std::vector<sk::RenderObject> m_objects;
};
}
