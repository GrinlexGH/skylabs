#include <thread>

#include <glm/glm.hpp>
#include <glm/gtx/type_aligned.hpp>
#include <tracy/Tracy.hpp>

#include "skylabs/engine/logging.hpp"
#include "skylabs/engine/render/vulkan/descriptor_writer.hpp"
#include "skylabs/engine/render/vulkan/graphics_pipeline.hpp"
#include "skylabs/engine/render/vulkan/renderer.hpp"

namespace {
struct ViewProjection {
    glm::aligned_mat4 view { 1 };
    glm::aligned_mat4 projection { 1 };
};

struct PushData {
    std::uint32_t uboOffset;
    std::uint32_t imageOffset;
    std::uint32_t samplerOffset;
};

glm::mat4 ReverseZPerspective(const unsigned int width, const unsigned int height, const float fov = 90,
                              const float nearZ = 0.01f) {
    glm::mat4 proj = glm::mat4(0.0f);
    const float g = 1.0f / std::tan(0.5f * glm::radians(fov));
    proj[0][0] = g / (static_cast<float>(width) / static_cast<float>(height));
    proj[1][1] = -g;
    proj[2][3] = -1.0f;
    proj[3][2] = nearZ;

    return proj;
}
}

namespace sk::render::vulkan {
Renderer::Renderer(const IWindow* const window, const IOSAdapter* const osAdapter,
                   const filesystem::Filesystem& filesystem)
    : m_filesystem(&filesystem) {
    // General context
    m_context = Context { window, osAdapter };

    m_swapchain = Swapchain { m_context.GetDevice(), window, *m_context.GetSurface(), 3,
                              vk::PresentModeKHR::eMailbox };

    m_inFlightContext = InFlightContext { kFramesInFlightCount };

    m_commandBufferAllocator =
        CommandBufferAllocator { *m_context.GetDevice(),
                                 m_context.GetDevice().GraphicsQueue().FamilyIndex() };

    // Frame synchronization
    m_firstUse = InFlight<bool> { m_inFlightContext, true };
    m_fence = InFlight<vk::raii::Fence> { m_inFlightContext, *m_context.GetDevice(),
                                          vk::FenceCreateInfo { vk::FenceCreateFlagBits::eSignaled } };
    m_imageAvailableSemaphore =
        InFlight<vk::raii::Semaphore> { m_inFlightContext, *m_context.GetDevice(),
                                        vk::SemaphoreCreateInfo { } };

    m_commandBuffers = InFlight { m_inFlightContext,
                                  m_commandBufferAllocator.Allocate(vk::CommandBufferLevel::ePrimary,
                                                                    m_inFlightContext.FrameCount()) };

    // Base pipeline
    m_viewProjection = InFlight<Buffer> {
        m_inFlightContext,      m_context.GetDevice(),        *m_context.GetAllocator(),
        sizeof(ViewProjection), MemoryLocation::eHostVisible, vk::BufferUsageFlagBits2::eUniformBuffer
    };

    m_texture = LoadTexture("assets://texture.jpg", "texture");

    const Shader vert(*m_context.GetDevice(), vk::ShaderStageFlagBits::eVertex,
                      filesystem.LoadAsVector32("res://shaders/triangle.vert.spv"));
    const Shader frag(*m_context.GetDevice(), vk::ShaderStageFlagBits::eFragment,
                      filesystem.LoadAsVector32("res://shaders/triangle.frag.spv"));

    if (m_context.GetDevice().Caps().descriptorHeap) {
        // Descriptor heap creation
        const auto heapProps = m_context.GetDevice()
                                   .GetPhysicalDevice()
                                   ->getProperties2<vk::PhysicalDeviceProperties2,
                                                    vk::PhysicalDeviceDescriptorHeapPropertiesEXT>()
                                   .get<vk::PhysicalDeviceDescriptorHeapPropertiesEXT>();

        constexpr std::uint32_t kMaxResourceDescriptors = 64;
        const auto resourceHeapUserSize = kMaxResourceDescriptors * heapProps.bufferDescriptorSize;
        m_resourceHeapReservedOff =
            utils::AlignUp(resourceHeapUserSize, heapProps.resourceHeapAlignment);
        m_resourceHeapReservedSize = heapProps.minResourceHeapReservedRange;
        m_resourceDescriptorHeap = Buffer { m_context.GetDevice(),
                                            *m_context.GetAllocator(),
                                            m_resourceHeapReservedOff + m_resourceHeapReservedSize,
                                            MemoryLocation::eHostVisible,
                                            vk::BufferUsageFlagBits2::eDescriptorHeapEXT,
                                            m_resourceHeapReservedOff };

        constexpr std::uint32_t kMaxSamplerDescriptors = 64;
        const auto sampleHeapUserSize = kMaxSamplerDescriptors * heapProps.samplerDescriptorSize;
        m_samplerHeapReservedOff = utils::AlignUp(sampleHeapUserSize, heapProps.samplerHeapAlignment);
        m_samplerHeapReservedSize = heapProps.minSamplerHeapReservedRange;
        m_samplerDescriptorHeap = Buffer { m_context.GetDevice(),
                                           *m_context.GetAllocator(),
                                           m_samplerHeapReservedOff + m_samplerHeapReservedSize,
                                           MemoryLocation::eHostVisible,
                                           vk::BufferUsageFlagBits2::eDescriptorHeapEXT,
                                           m_samplerHeapReservedOff };

        // Pipeline "layout"
        vk::DescriptorSetAndBindingMappingEXT uboMapping { };
        uboMapping.descriptorSet = 0;
        uboMapping.firstBinding = 0;
        uboMapping.bindingCount = 1;
        uboMapping.resourceMask = vk::SpirvResourceTypeFlagBitsEXT::eUniformBuffer;
        uboMapping.source = vk::DescriptorMappingSourceEXT::eHeapWithPushIndex;
        {
            vk::DescriptorMappingSourcePushIndexEXT p { };
            p.pushOffset = offsetof(PushData, uboOffset);
            p.heapIndexStride = 1;
            p.heapArrayStride = heapProps.bufferDescriptorSize;
            uboMapping.sourceData.pushIndex = p;
        }

        vk::DescriptorSetAndBindingMappingEXT texMapping { };
        texMapping.descriptorSet = 0;
        texMapping.firstBinding = 1;
        texMapping.bindingCount = 1;
        texMapping.resourceMask = vk::SpirvResourceTypeFlagBitsEXT::eCombinedSampledImage;
        texMapping.source = vk::DescriptorMappingSourceEXT::eHeapWithPushIndex;
        {
            vk::DescriptorMappingSourcePushIndexEXT p { };
            p.pushOffset = offsetof(PushData, imageOffset);
            p.heapIndexStride = 1;
            p.heapArrayStride = heapProps.imageDescriptorSize;

            p.samplerPushOffset = offsetof(PushData, samplerOffset);
            p.samplerHeapIndexStride = 1;
            p.samplerHeapArrayStride = heapProps.samplerDescriptorSize;
            texMapping.sourceData.pushIndex = p;
        }

        std::array colorFormats { m_swapchain.SurfaceFormat().format };
        m_pipeline =
            GraphicsPipeline { m_context.GetDevice(),
                               GraphicsPipelineCreateInfo { .layout = nullptr,
                                                            .shaders = { &vert, &frag },
                                                            .vertexBindings = { },
                                                            .heapMappings = { uboMapping, texMapping },
                                                            .renderingInfo = { { }, colorFormats } } };

        // Descriptors allocation and writing
        auto allocate = [](Buffer& heap, const vk::DeviceSize size, const vk::DeviceSize alignment) {
            vma::VirtualAllocationCreateInfo ci;
            ci.size = size;
            ci.alignment = alignment;
            return heap.VirtualBlock().allocate(ci);
        };
        auto hostRange = [](Buffer& heap, const vma::raii::VirtualAllocation& a) {
            const auto info = a.getInfo();
            return vk::HostAddressRangeEXT { static_cast<std::byte*>(heap.Data()) + info.offset,
                                             info.size };
        };

        const auto frameCount = m_inFlightContext.FrameCount();

        std::vector<vk::DeviceAddressRangeEXT> uboRanges;
        std::vector<vk::ResourceDescriptorInfoEXT> resourceInfos;
        std::vector<vk::HostAddressRangeEXT> resourceDsts;
        uboRanges.reserve(frameCount);
        resourceInfos.reserve(frameCount + 1);
        resourceDsts.reserve(frameCount + 1);

        std::vector<vma::raii::VirtualAllocation> uboDescriptors;
        uboDescriptors.reserve(frameCount);

        for (const auto i : utils::Range(frameCount)) {
            uboRanges.emplace_back(m_viewProjection[i].Address(), sizeof(ViewProjection));

            vk::ResourceDescriptorInfoEXT info { };
            info.type = vk::DescriptorType::eUniformBuffer;
            info.data.pAddressRange = &uboRanges.back();
            resourceInfos.push_back(info);

            uboDescriptors.push_back(allocate(m_resourceDescriptorHeap, heapProps.bufferDescriptorSize,
                                              heapProps.bufferDescriptorAlignment));
            resourceDsts.push_back(hostRange(m_resourceDescriptorHeap, uboDescriptors.back()));
        }

        const vk::ImageViewCreateInfo viewCI {
            { }, *m_texture, vk::ImageViewType::e2D, m_texture.Format(), { }, m_texture.FullRange()
        };
        const vk::ImageDescriptorInfoEXT image { &viewCI, vk::ImageLayout::eShaderReadOnlyOptimal };

        vk::ResourceDescriptorInfoEXT imageInfo { };
        imageInfo.type = vk::DescriptorType::eSampledImage;
        imageInfo.data.pImage = &image;
        resourceInfos.push_back(imageInfo);

        m_imageDescriptor = allocate(m_resourceDescriptorHeap, heapProps.imageDescriptorSize,
                                     heapProps.imageDescriptorAlignment);
        resourceDsts.push_back(hostRange(m_resourceDescriptorHeap, m_imageDescriptor));

        m_context.GetDevice()->writeResourceDescriptorsEXT(resourceInfos, resourceDsts);

        // --- sampler: один вызов ---
        vk::SamplerCreateInfo samplerCreateInfo { };
        samplerCreateInfo.maxLod = vk::LodClampNone;
        samplerCreateInfo.magFilter = vk::Filter::eLinear;
        samplerCreateInfo.minFilter = vk::Filter::eLinear;
        samplerCreateInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
        samplerCreateInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
        samplerCreateInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
        samplerCreateInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;

        m_samplerDescriptor = allocate(m_samplerDescriptorHeap, heapProps.samplerDescriptorSize,
                                       heapProps.samplerDescriptorAlignment);
        const auto samplerDst = hostRange(m_samplerDescriptorHeap, m_samplerDescriptor);
        m_context.GetDevice()->writeSamplerDescriptorsEXT(samplerCreateInfo, samplerDst);

        m_uboDescriptor = InFlight { m_inFlightContext, std::move(uboDescriptors) };
    } else {
        m_pipelineLayoutCache = PipelineLayoutCache { *m_context.GetDevice() };
        m_descriptorLayoutCache = DescriptorLayoutCache { *m_context.GetDevice() };

        const vk::raii::DescriptorSetLayout& descriptorSetLayout = m_descriptorLayoutCache.GetLayout({
            { 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex },
            { 1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
        });

        const auto& pipelineLayout = m_pipelineLayoutCache.GetLayout(
            { .descriptorSetLayouts = { descriptorSetLayout }, .pushConstants = { } });

        // Схуяли я вообще должен заранее знать что и сколько я хочу выделить?!!
        vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo;
        std::array descriptorPoolSizes {
            vk::DescriptorPoolSize { vk::DescriptorType::eUniformBuffer, kFramesInFlightCount },
            vk::DescriptorPoolSize { vk::DescriptorType::eCombinedImageSampler, kFramesInFlightCount }
        };
        descriptorPoolCreateInfo.setPoolSizes(descriptorPoolSizes);
        descriptorPoolCreateInfo.setMaxSets(kFramesInFlightCount);
        m_descriptorPool = vk::raii::DescriptorPool { *m_context.GetDevice(), descriptorPoolCreateInfo };

        std::array colorFormats { m_swapchain.SurfaceFormat().format };
        m_pipeline =
            GraphicsPipeline { m_context.GetDevice(),
                               GraphicsPipelineCreateInfo { .layout = pipelineLayout,
                                                            .shaders = { &vert, &frag },
                                                            .vertexBindings = { },
                                                            .heapMappings = { },
                                                            .renderingInfo = { { }, colorFormats } } };

        // Allocate pack of descriptors
        vk::DescriptorSetAllocateInfo descriptorSetAllocationInfo { };
        std::vector descriptorSetLayouts { m_inFlightContext.FrameCount(), *descriptorSetLayout };
        descriptorSetAllocationInfo.descriptorPool = m_descriptorPool;
        descriptorSetAllocationInfo.setSetLayouts(descriptorSetLayouts);
        std::vector<vk::DescriptorSet> descriptorSets =
            m_context.GetDevice()->allocateDescriptorSets(descriptorSetAllocationInfo) |
            std::views::transform([](vk::raii::DescriptorSet& set) { return set.release(); }) |
            std::ranges::to<std::vector<vk::DescriptorSet>>();
        m_descriptorSet = InFlight { m_inFlightContext, std::move(descriptorSets) };

        vk::SamplerCreateInfo samplerCreateInfo { };
        samplerCreateInfo.maxLod = vk::LodClampNone;
        samplerCreateInfo.magFilter = vk::Filter::eLinear;
        samplerCreateInfo.minFilter = vk::Filter::eLinear;
        samplerCreateInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
        samplerCreateInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
        samplerCreateInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
        samplerCreateInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
        m_sampler = vk::raii::Sampler { *m_context.GetDevice(), samplerCreateInfo };

        DescriptorWriter descriptorWriter { *m_context.GetDevice() };
        for (const auto i : utils::Range(m_inFlightContext.FrameCount())) {
            descriptorWriter.Clear();
            descriptorWriter
                .WriteBuffer(0, *m_viewProjection[i], m_viewProjection[i].Size(), 0,
                             vk::DescriptorType::eUniformBuffer)
                .WriteImage(1, m_texture.View(), m_sampler, vk::ImageLayout::eShaderReadOnlyOptimal,
                            vk::DescriptorType::eCombinedImageSampler)
                .UpdateSet(m_descriptorSet[i]);
        }
    }
}

Renderer::~Renderer() {
    try {
        m_context.GetDevice()->waitIdle();
    } catch (const vk::SystemError& e) {
        log::Error("Failed to destroy renderer: {}", e.what());
    }
}

void Renderer::BeginFrame() {
    if (m_frameActive || m_surfaceLost) return;

    try {
        // This part should handle suboptimal
        // May throw surface lost error
        if (m_swapchain.IsOutdated()) {
            m_context.GetDevice()->waitIdle();
            RecreateSwapchain();
        }

        {
            ZoneNamed(waitFrameInFlight, "Wait frame in flight");
            std::ignore = m_context.GetDevice()->waitForFences(
                { m_fence.Get() }, vk::True, std::numeric_limits<std::uint64_t>::max());
        }

        // Acquire next image from the swapchain
        vk::Result result;
        std::tie(result, m_currentImageIndex) = m_swapchain->acquireNextImage(
            std::numeric_limits<std::uint64_t>::max(), *m_imageAvailableSemaphore.Get());

        if (result == vk::Result::eErrorOutOfDateKHR) {
            m_context.GetDevice()->waitIdle();
            RecreateSwapchain();

            std::tie(result, m_currentImageIndex) = m_swapchain->acquireNextImage(
                std::numeric_limits<std::uint64_t>::max(), *m_imageAvailableSemaphore.Get());

            // If we still can't restore the swapchain, we simply return
            if (result == vk::Result::eErrorOutOfDateKHR) {
                return;
            }
        }
    } catch (const vk::SurfaceLostKHRError&) {
        MarkSurfaceLost();
        return;
    }

    const auto& cmd = m_commandBuffers.Get();
    cmd->reset();
    cmd->begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    cmd.PipelineBarrier({ ImageBarrier { .image = m_swapchain.Images()[m_currentImageIndex],
                                         .range = m_swapchain.Images()[m_currentImageIndex].FullRange(),
                                         .oldUsage = Usage::eSwapchainAcquire,
                                         .newUsage = Usage::eColorAttachment } });

    if (m_context.GetDevice().Caps().descriptorHeap) {
        vk::BindHeapInfoEXT bind { };
        bind.heapRange = vk::DeviceAddressRangeEXT { m_resourceDescriptorHeap.Address(),
                                                     m_resourceDescriptorHeap.Size() };
        bind.reservedRangeOffset = m_resourceHeapReservedOff;
        bind.reservedRangeSize = m_resourceHeapReservedSize;
        cmd->bindResourceHeapEXT(bind);

        bind.heapRange = vk::DeviceAddressRangeEXT { m_samplerDescriptorHeap.Address(),
                                                     m_samplerDescriptorHeap.Size() };
        bind.reservedRangeOffset = m_samplerHeapReservedOff;
        bind.reservedRangeSize = m_samplerHeapReservedSize;
        cmd->bindSamplerHeapEXT(bind);
    }

    m_frameActive = true;
}

void Renderer::Draw(const glm::mat4 view, const float fov, float /*deltatime*/) {
    if (!m_frameActive) return;

    // Update model and view
    auto [width, height] = m_swapchain.Extent();

    // Rotate render if we need
    const vk::SurfaceTransformFlagBitsKHR surfaceTransform = m_swapchain.SurfaceTransform();
    glm::mat4 rotation { 1 };

    if (surfaceTransform == vk::SurfaceTransformFlagBitsKHR::eRotate90) {
        rotation = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0, 0, 1));
    } else if (surfaceTransform == vk::SurfaceTransformFlagBitsKHR::eRotate270) {
        rotation = glm::rotate(glm::mat4(1.0f), glm::radians(270.0f), glm::vec3(0, 0, 1));
    } else if (surfaceTransform == vk::SurfaceTransformFlagBitsKHR::eRotate180) {
        rotation = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0, 0, 1));
    }

    const ViewProjection viewProjection {
        .view = view,
        .projection = rotation * ReverseZPerspective(width, height, fov),
    };

    std::memcpy(m_viewProjection.Get().Data(), &viewProjection, sizeof(viewProjection));

    // Render frame
    const auto& cmd = m_commandBuffers.Get();
    const auto& swapchainImage = m_swapchain.Images()[m_currentImageIndex];

    vk::RenderingAttachmentInfo attachInfo { };
    attachInfo.imageView = *swapchainImage.View();
    attachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    attachInfo.loadOp = vk::AttachmentLoadOp::eClear;
    attachInfo.storeOp = vk::AttachmentStoreOp::eStore;
    attachInfo.clearValue.color = std::array { 0.0f, 0.0f, 0.0f, 0.0f };

    vk::RenderingInfo renderInfo { };
    renderInfo.renderArea = vk::Rect2D { { 0, 0 }, swapchainImage.Extent2D() };
    renderInfo.layerCount = 1;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachments = &attachInfo;

    cmd->beginRendering(renderInfo);

    cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipeline);
    cmd->setViewport(0, { { 0.0f, 0.0f, static_cast<float>(swapchainImage.Extent().width),
                            static_cast<float>(swapchainImage.Extent().height), 0.0f, 1.0f } });
    cmd->setScissor(0, { { { 0, 0 }, swapchainImage.Extent2D() } });

    if (m_context.GetDevice().Caps().descriptorHeap) {
        PushData pd {
            .uboOffset = static_cast<std::uint32_t>(m_uboDescriptor.Get().getInfo().offset),
            .imageOffset = static_cast<std::uint32_t>(m_imageDescriptor.getInfo().offset),
            .samplerOffset = static_cast<std::uint32_t>(m_samplerDescriptor.getInfo().offset),
        };
        vk::PushDataInfoEXT push { };
        push.offset = 0;
        push.data = vk::HostAddressRangeConstEXT { &pd, sizeof(pd) };
        cmd->pushDataEXT(push);
    } else {
        cmd->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipeline.Layout(), 0,
                                m_descriptorSet.Get(), { });
    }

    cmd->draw(3, 1, 0, 0);

    cmd->endRendering();
}

void Renderer::EndFrame() {
    if (!m_frameActive) return;
    m_frameActive = false;

    const auto& cmd = m_commandBuffers.Get();
    cmd.PipelineBarrier({ ImageBarrier { .image = m_swapchain.Images()[m_currentImageIndex],
                                         .range = m_swapchain.Images()[m_currentImageIndex].FullRange(),
                                         .oldUsage = Usage::eColorAttachment,
                                         .newUsage = Usage::ePresent } });

    cmd->end();

    m_context.GetDevice()->resetFences({ m_fence.Get() });

    vk::SemaphoreSubmitInfo waitInfo { };
    waitInfo.setSemaphore(*m_imageAvailableSemaphore.Get());
    waitInfo.setStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput);

    vk::SemaphoreSubmitInfo signalInfo { };
    signalInfo.setSemaphore(*m_swapchain.RenderFinishedSemaphores()[m_currentImageIndex]);
    signalInfo.setStageMask(vk::PipelineStageFlagBits2::eAllCommands);

    vk::CommandBufferSubmitInfo cmdInfo { };
    cmdInfo.setCommandBuffer(**m_commandBuffers.Get());

    vk::SubmitInfo2 finalSubmit { };
    finalSubmit.setWaitSemaphoreInfos(waitInfo);
    finalSubmit.setCommandBufferInfos(cmdInfo);
    finalSubmit.setSignalSemaphoreInfos(signalInfo);
    m_context.GetDevice().GraphicsQueue()->submit2(finalSubmit, m_fence.Get());

    m_inFlightContext.NextFrame();

    try {
        vk::PresentInfoKHR presentInfo { };
        presentInfo.setWaitSemaphores(*m_swapchain.RenderFinishedSemaphores()[m_currentImageIndex]);
        presentInfo.setSwapchains({ **m_swapchain });
        presentInfo.setImageIndices({ m_currentImageIndex });
        if (const vk::Result result = m_context.GetDevice().PresentQueue()->presentKHR(presentInfo);
            result == vk::Result::eErrorOutOfDateKHR) {
            m_context.GetDevice()->waitIdle();
            RecreateSwapchain();
        }
    } catch (const vk::SurfaceLostKHRError&) {
        MarkSurfaceLost();
    }
}

void Renderer::OnDeviceReset() {
    m_context.GetDevice()->waitIdle();
    m_swapchain.Clear();
    m_context.RecreateSurface();
    RecreateSwapchain();
    m_surfaceLost = false;
}

void Renderer::RecreateSwapchain() { m_swapchain = Swapchain { m_swapchain, { } }; }

void Renderer::MarkSurfaceLost() { m_surfaceLost = true; }

Image Renderer::LoadTexture(const std::string& path, const std::string& debugName) {
    std::unique_ptr<filesystem::IImage> ramTexture =
        m_filesystem->LoadImage(path, filesystem::ImageFormat::eRGBA32);

    if (!ramTexture) {
        throw std::runtime_error(fmt::format("Failed to load texture: {}", path));
    }

    auto [width, height] = ramTexture->Extent();
    const vk::DeviceSize imageSize = static_cast<vk::DeviceSize>(width) * height * 4;

    if (m_stagingBuffer.Size() < imageSize) {
        m_stagingBuffer =
            Buffer { m_context.GetDevice(), *m_context.GetAllocator(), imageSize,
                     MemoryLocation::eHostVisible, vk::BufferUsageFlagBits2::eTransferSrc };
    }

    std::memcpy(m_stagingBuffer.Data(), ramTexture->Pixels(), static_cast<std::size_t>(imageSize));

    const std::uint32_t mipLevels =
        static_cast<std::uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

    Image vramTexture = Image { *m_context.GetDevice(),
                                *m_context.GetAllocator(),
                                {
                                    .extent = vk::Extent3D { width, height, 1 },
                                    .format = vk::Format::eR8G8B8A8Srgb,
                                    .mipLevels = mipLevels,
                                    .usageFlags = vk::ImageUsageFlagBits::eTransferSrc |
                                                  vk::ImageUsageFlagBits::eTransferDst |
                                                  vk::ImageUsageFlagBits::eSampled,
                                } };

    m_commandBuffers.Get().ImmediateSubmit(
        *m_context.GetDevice().GraphicsQueue(), [&](const CommandBuffer& cmd) {
            cmd.PipelineBarrier({ ImageBarrier { vramTexture, vramTexture.FullRange(), Usage::eNone,
                                                 Usage::eTransferWrite } });
            cmd.Copy(m_stagingBuffer, vramTexture);
            cmd.GenerateMipmaps(vramTexture);
        });

    if (m_context.GetInstance().IsExtensionEnabled(vk::EXTDebugUtilsExtensionName)) {
        m_context.GetDevice()->setDebugUtilsObjectNameEXT(*vramTexture, debugName);
    }

    return vramTexture;
}
}
