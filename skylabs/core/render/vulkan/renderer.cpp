#include <skylabs/core/render/vulkan/renderer.hpp>
#include <skylabs/public/logging.hpp>

template<> struct std::hash<CVertex> {
    std::size_t operator()(const CVertex& vertex) const noexcept {
        std::size_t seed = 0;
        boost::hash_combine(seed, vertex.m_position.x);
        boost::hash_combine(seed, vertex.m_position.y);
        boost::hash_combine(seed, vertex.m_position.z);

        boost::hash_combine(seed, vertex.m_texCoord.x);
        boost::hash_combine(seed, vertex.m_texCoord.y);
        return seed;
    }
};

namespace {
glm::mat4 ReverseZPerspective(const unsigned int width, const unsigned int height, const float fov = 90, const float nearZ = 0.01f) {
    glm::mat4 proj = glm::mat4(0.0f);
    float g = 1.0f / std::tan(0.5f * glm::radians(fov));
    proj[0][0] = g / (static_cast<float>(width) / static_cast<float>(height));
    proj[1][1] = -g;
    proj[2][3] = -1.0f;
    proj[3][2] = nearZ;

    return proj;
}
}

namespace Vulkan {
CRenderer::CRenderer(
    const IWindow* const window,
    const IOSConnector* const osConnector,
    const CFilesystem& filesystem
) : m_filesystem(&filesystem) {
    m_context = CContext { window, osConnector };
    const auto& device = m_context.Device();

    m_swapchain = CSwapchain {
        *m_context.PhysicalDevice(), device,
        m_context.Window(), *m_context.Surface(),
        2, vk::PresentModeKHR::eMailbox
    };
    m_inFlightContext = CInFlightContext { m_swapchain.Images().size() };

    m_pipelineLayoutCache = CPipelineLayoutCache { *device };
    m_descriptorLayoutCache = CDescriptorLayoutCache { *device };
    m_descriptorAllocator = CDescriptorAllocator { *device };

    m_commandBufferAllocator = CCommandBufferAllocator {
        *device, m_context.Device().GraphicsQueue().FamilyIndex()
    };
    m_graphicsCmd = InFlight { m_inFlightContext,
        m_commandBufferAllocator.Allocate(
            vk::CommandBufferLevel::ePrimary, static_cast<std::uint32_t>(m_inFlightContext.FrameCount())
        )
    };

    m_firstUse = InFlight<bool> { m_inFlightContext, true };
    m_fence = InFlight<vk::raii::Fence> { m_inFlightContext,
        *m_context.Device(), vk::FenceCreateInfo { vk::FenceCreateFlagBits::eSignaled }
    };
    m_imageAvailableSemaphore = InFlight<vk::raii::Semaphore> { m_inFlightContext,
        *m_context.Device(), vk::SemaphoreCreateInfo {}
    };

    const std::size_t imageCount = m_swapchain.Images().size();
    m_renderFinishedSemaphores.reserve(imageCount);
    for (auto i = 0u; i < imageCount; ++i) {
        m_renderFinishedSemaphores.emplace_back(*m_context.Device(), vk::SemaphoreCreateInfo {});
    }

    m_mainPass = CMainPass {
        m_context.Device(), m_inFlightContext, m_context.Allocator(), m_pipelineLayoutCache, m_descriptorLayoutCache,
        m_descriptorAllocator, filesystem,
        { m_swapchain.Extent().width, m_swapchain.Extent().height }
    };
    m_postProcessPass = CPostProcessPass {
        m_context.Device(), m_inFlightContext, m_pipelineLayoutCache, m_descriptorLayoutCache, m_descriptorAllocator,
        filesystem,
        m_mainPass.MainAttachment(), m_swapchain.SurfaceFormat().format
    };

    m_vertexBuffer = CBuffer {
        *m_context.Allocator(), GEOMETRY_POOL_SIZE,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
        MemoryLocation::eDeviceOnly
    };

    m_indexBuffer = CBuffer {
        *m_context.Allocator(), GEOMETRY_POOL_SIZE,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
        MemoryLocation::eDeviceOnly
    };

    m_mainPass.WriteDescriptors(m_meshTextures);
}

CRenderer::~CRenderer() {
    if (**m_context.Device()) {
        try { m_context.Device()->waitIdle();}
        catch (const vk::SystemError& e) {
            Log::Error("Failed to wait device idle in renderer destructor: {}", e.what());
        }
    }
}

void CRenderer::Draw(const glm::mat4 view, const float fov, float /*deltatime*/) {
    const auto& device = m_context.Device();
    const auto& cmd = m_graphicsCmd.Get();

    UpdateMVP(view, fov);

    // Wait for fence to ensure that the previous frame rendering is finished
    std::ignore = device->waitForFences(
        { m_fence.Get() }, vk::True,
        std::numeric_limits<std::uint64_t>::max()
    );

    // Acquire next image from the swapchain
    auto [acquireResult, imageIndex] = m_swapchain.AcquireImage(*m_imageAvailableSemaphore.Get());
    if (acquireResult != vk::Result::eSuccess) {
        Log::Debug("Acquire result: {}", vk::to_string(acquireResult));
        if (acquireResult == vk::Result::eErrorOutOfDateKHR) {
            RecreateSwapchain();
            std::tie(acquireResult, imageIndex) = m_swapchain.AcquireImage(*m_imageAvailableSemaphore.Get());
        }

#ifdef PLATFORM_WINDOWS
        if (acquireResult == vk::Result::eSuboptimalKHR && !m_context.Window()->Minimized()) {
            RecreateSwapchain();
            m_imageAvailableSemaphore.Get() = vk::raii::Semaphore { *m_context.Device(), vk::SemaphoreCreateInfo { } };
            std::tie(acquireResult, imageIndex) = m_swapchain.AcquireImage(*m_imageAvailableSemaphore.Get());
        }
#endif

        if (acquireResult == vk::Result::eErrorSurfaceLostKHR) {
            return;
        }
    }

    // Reset fence after resizing to avoid deadlock on next invocation of Draw()
    device->resetFences({ m_fence.Get() });

    const auto& mainColor = m_mainPass.MainAttachment().Get();
    const auto& mainColorMSAA = m_mainPass.MainMSAAAttachment().Get();
    const auto& mainDepthMSAA = m_mainPass.DepthMSAAAttachment().Get();
    const auto& swapchainImages = m_swapchain.Images();

    cmd->reset();
    cmd->begin({});

    if (m_firstUse.Get()) {
        cmd.PipelineBarrier({
            ImageBarrier { mainColor, mainColor.FullRange(),
                Usage::eNone, Usage::eColorAttachment },
            ImageBarrier { mainColorMSAA, mainColorMSAA.FullRange(),
                Usage::eNone, Usage::eColorAttachment },
            ImageBarrier { mainDepthMSAA, mainDepthMSAA.FullRange(),
                Usage::eNone, Usage::eDepthWrite },
        });
        m_firstUse.Get() = false;
    } else {
        cmd.PipelineBarrier({
            ImageBarrier { mainColor, mainColor.FullRange(),
                Usage::eSampledFragment, Usage::eColorAttachment },
        });
    }

    m_mainPass.Draw(cmd, m_vertexBuffer, m_indexBuffer, m_meshes, m_objects);

    cmd.PipelineBarrier({
        ImageBarrier { mainColor, mainColor.FullRange(),
            Usage::eColorAttachment, Usage::eSampledFragment },
        ImageBarrier { swapchainImages[imageIndex], swapchainImages[imageIndex].FullRange(),
            Usage::eNone, Usage::eColorAttachment }
    });

    m_postProcessPass.Draw(cmd, m_swapchain.Images()[imageIndex]);

    cmd.PipelineBarrier({
        ImageBarrier { swapchainImages[imageIndex], swapchainImages[imageIndex].FullRange(),
            Usage::eColorAttachment, Usage::ePresent }
    });

    cmd->end();

    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

    vk::SubmitInfo finalSubmit {};
    finalSubmit.setWaitSemaphores({ *m_imageAvailableSemaphore.Get() });
    finalSubmit.setWaitDstStageMask({ waitStage });
    finalSubmit.setCommandBuffers({ **cmd });
    finalSubmit.setSignalSemaphores({ *m_renderFinishedSemaphores[imageIndex] });
    m_context.Device().GraphicsQueue()->submit(finalSubmit, m_fence.Get());

    // Present
    vk::Result presentResult = m_swapchain.PresentImage(imageIndex, { *m_renderFinishedSemaphores[imageIndex] });
    if (presentResult != vk::Result::eSuccess) {
        Log::Debug("Present result: {}", vk::to_string(presentResult));
#ifdef PLATFORM_ANDROID
        if (presentResult == vk::Result::eSuboptimalKHR) {
            RecreateSwapchain();
        }
#endif
    }

    m_inFlightContext.NextFrame();
}

void CRenderer::OnDeviceLost() {
    m_context.Device()->waitIdle();
    m_swapchain.Clear();
    m_context.RepairSurface();
    RecreateSwapchain();
}

void CRenderer::OnPossiblyWindowSizeChange() {
    if (const auto [width, height] = m_context.Window()->DrawableSize();
        vk::Extent2D { width, height } != m_swapchain.Extent()
    ) {
        RecreateSwapchain();
    }
}

void CRenderer::RecreateSwapchain() {
    m_context.Device()->waitIdle();
    const auto [oldWidth, oldHeight] = m_swapchain.Extent();
    m_swapchain.Recreate({ });

    if (const auto [newWidth, newHeight] = m_swapchain.Extent();
        oldWidth != newWidth || oldHeight != newHeight
    ) { ResizeTextures(); }
}

void CRenderer::ResizeTextures() {
    // Reset sync state
    for (auto&& i : m_firstUse) { i = true; }

    m_mainPass.Resize({ m_swapchain.Extent().width, m_swapchain.Extent().height });
    m_postProcessPass.Resize(m_mainPass.MainAttachment());
}

void CRenderer::UpdateMVP(const glm::mat4& view, float fov) {
    auto [width, height] = m_swapchain.Extent();

    // Rotate render if we need
    vk::SurfaceTransformFlagBitsKHR surfaceTransform = m_swapchain.SurfaceTransform();
    glm::mat4 rot = glm::mat4(1.0f);

    if (surfaceTransform == vk::SurfaceTransformFlagBitsKHR::eRotate90) {
        rot = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0, 0, 1));
    } else if (surfaceTransform == vk::SurfaceTransformFlagBitsKHR::eRotate270) {
        rot = glm::rotate(glm::mat4(1.0f), glm::radians(270.0f), glm::vec3(0, 0, 1));
    } else if (surfaceTransform == vk::SurfaceTransformFlagBitsKHR::eRotate180) {
        rot = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0, 0, 1));
    }

    const CMVP ubo {
        .view = view,
        .proj = rot * ReverseZPerspective(width, height, fov),
    };

    std::memcpy(m_mainPass.MVP().Get().Data(), &ubo, sizeof(ubo));
}

std::uint32_t CRenderer::UploadMesh(const std::vector<CVertex>& vertices, const std::vector<std::uint16_t>& indices) {
    auto UploadToPool = [&] {
        SubMesh mesh { };

        vk::DeviceSize vSize = vertices.size() * sizeof(vertices[0]);
        vk::DeviceSize iSize = indices.size() * sizeof(indices[0]);

        vma::VirtualAllocationCreateInfo allocationInfo { };
        allocationInfo.setSize(vSize);
        mesh.vtxAlloc = vma::raii::VirtualAllocation { m_vertexBuffer.VirtualBlock(), allocationInfo };
        allocationInfo.setSize(iSize);
        mesh.idxAlloc = vma::raii::VirtualAllocation { m_indexBuffer.VirtualBlock(), allocationInfo };
        mesh.indexCount = static_cast<std::uint32_t>(indices.size());

        vk::DeviceSize totalSize = vSize + iSize;
        if (m_stagingBuffer.Size() < totalSize) {
            m_stagingBuffer = CBuffer { *m_context.Allocator(), totalSize,
                vk::BufferUsageFlagBits::eTransferSrc, MemoryLocation::eHostVisible
            };
        }

        std::memcpy(m_stagingBuffer.Data(), vertices.data(), vSize);
        std::memcpy(static_cast<std::uint8_t*>(m_stagingBuffer.Data()) + vSize, indices.data(), iSize);

        m_graphicsCmd.Get().ImmediateSubmit(*m_context.Device().GraphicsQueue(),
            [&](const CCommandBuffer& cmd) {
                cmd.Copy(m_stagingBuffer, m_vertexBuffer, vSize, { 0, mesh.VtxOffset() });
                cmd.Copy(m_stagingBuffer, m_indexBuffer, iSize, { vSize, mesh.IdxOffset() } );
            }
        );

        return mesh;
    };

    m_meshes.emplace_back(UploadToPool());
    return static_cast<std::uint32_t>(m_meshes.size() - 1);
}

::CRenderObject& CRenderer::GetObjectData(const std::uint32_t id) {
    assert(id < m_objects.size());
    return m_objects[id];
}

const ::CRenderObject& CRenderer::GetObjectData(const std::uint32_t id) const {
    assert(id < m_objects.size());
    return m_objects[id];
}

CRenderObject CRenderer::UploadGameObject(const std::uint32_t meshId, const glm::mat4& matrix, std::uint16_t colorId) {
    m_objects.emplace_back(meshId, colorId, matrix);
    return CRenderObject { this, static_cast<std::uint32_t>(m_objects.size() - 1) };
}

// void CRenderer::LoadTextures() {
//     auto LoadTexture = [&](const std::string& path, const std::string& debugName) {
//         std::unique_ptr<IFileStream> stream = Filesystem::LoadAsIO(path);
//         SDL_IOStream* sdlStream = SDL::CreateIOStreamFromResource(stream.get());

//         SDL_Surface* imageRaw = IMG_Load_IO(sdlStream, false);
//         if (!imageRaw) {
//             throw std::runtime_error("Failed to load texture: " + path);
//         }

//         SDL_Surface* image = SDL_ConvertSurface(imageRaw, SDL_PIXELFORMAT_RGBA32);
//         SDL_DestroySurface(imageRaw);

//         const vk::DeviceSize imageSize = static_cast<vk::DeviceSize>(image->w) * image->h * 4;
//         if (m_stagingBuffer.Size() < imageSize) {
//             m_stagingBuffer = CBuffer {
//                 m_context, imageSize,
//                 vk::BufferUsageFlagBits::eTransferSrc,
//                 MemoryLocation::eHostVisible
//             };
//         }
//         std::memcpy(m_stagingBuffer.Data(), image->pixels, static_cast<std::size_t>(imageSize));

//         const std::uint32_t mipLevels =
//             static_cast<std::uint32_t>(std::floor(std::log2(std::max(image->w, image->h)))) + 1;

//         CImage texture { m_context, {
//             .m_extent = vk::Extent3D {
//                 static_cast<std::uint32_t>(image->w),
//                 static_cast<std::uint32_t>(image->h),
//                 1
//             },
//             .m_format = vk::Format::eR8G8B8A8Srgb,
//             .m_mipLevels = mipLevels,
//             .m_usageFlags =
//                 vk::ImageUsageFlagBits::eTransferSrc |
//                 vk::ImageUsageFlagBits::eTransferDst |
//                 vk::ImageUsageFlagBits::eSampled,
//         }};

//         m_graphicsCmd.Get().ImmediateSubmit(*m_context.Device().GraphicsQueue(),
//             [&](const CCommandBuffer& cmd) {
//                 cmd.PipelineBarrier({ ImageBarrier { texture, texture.FullRange(), Vulkan::Usage::eNone, Vulkan::Usage::eTransferWrite } });
//                 cmd.Copy(texture, m_stagingBuffer);
//                 cmd.GenerateMipmaps(texture);
//             }
//         );

//         SDL_DestroySurface(image);

//         if (m_context.Instance().IsExtensionEnabled(vk::EXTDebugUtilsExtensionName)) {
//             m_context.Device()->setDebugUtilsObjectNameEXT(*texture, debugName);
//         }

//         return texture;
//     };

//     m_meshTextures.push_back(LoadTexture("assets://matroskin.png", "matroskin"));
//     m_meshTextures.push_back(LoadTexture("assets://viking_room.png", "viking-room"));
// }

// void CRenderer::LoadModels() {
//     auto LoadModel = [&](const std::string_view filename) {
//         std::vector<CVertex> vertices;
//         std::vector<std::uint16_t> indices;

//         tinyobj::ObjReader reader;
//         reader.ParseFromString(Filesystem::LoadAsString(filename), "");
//         if (!reader.Valid()) {
//             throw std::runtime_error(reader.Warning() + " " + reader.Error());
//         }

//         tinyobj::attrib_t attrib = reader.GetAttrib();
//         std::vector<tinyobj::shape_t> shapes = reader.GetShapes();

//         std::unordered_map<CVertex, std::uint32_t> uniqueVertices {};
//         for (const auto& shape : shapes) {
//             for (const auto& index : shape.mesh.indices) {
//                 CVertex vertex {};

//                 if (index.vertex_index >= 0) {
//                     vertex.m_position = {
//                         attrib.vertices[(3 * index.vertex_index) + 0],
//                         attrib.vertices[(3 * index.vertex_index) + 1],
//                         attrib.vertices[(3 * index.vertex_index) + 2]
//                     };
//                 }

//                 if (index.texcoord_index >= 0) {
//                     vertex.m_texCoord = {
//                         attrib.texcoords[(2 * index.texcoord_index) + 0],
//                         1.0f - attrib.texcoords[(2 * index.texcoord_index) + 1]
//                     };
//                 } else {
//                     vertex.m_texCoord = {
//                         (vertex.m_position.x * 0.5f) + 0.5f,
//                         (vertex.m_position.z * 0.5f) + 0.5f
//                     };
//                 }

//                 if (!uniqueVertices.contains(vertex)) {
//                     uniqueVertices[vertex] = static_cast<std::uint32_t>(vertices.size());
//                     vertices.push_back(vertex);
//                 }

//                 indices.push_back(static_cast<std::uint16_t>(uniqueVertices[vertex]));
//             }
//         }

//         return std::tuple { vertices, indices };
//     };

//     auto UploadToPool = [&](const std::string& path) {
//         SubMesh mesh { };
//         auto [vertices, indices] = LoadModel(path);

//         vk::DeviceSize vSize = vertices.size() * sizeof(vertices[0]);
//         vk::DeviceSize iSize = indices.size() * sizeof(indices[0]);

//         vma::VirtualAllocationCreateInfo allocationInfo { };
//         allocationInfo.setSize(vSize);
//         mesh.vtxAlloc = vma::raii::VirtualAllocation { m_vertexBuffer.VirtualBlock(), allocationInfo };
//         allocationInfo.setSize(iSize);
//         mesh.idxAlloc = vma::raii::VirtualAllocation { m_indexBuffer.VirtualBlock(), allocationInfo };
//         mesh.indexCount = static_cast<std::uint32_t>(indices.size());

//         vk::DeviceSize totalSize = vSize + iSize;
//         if (m_stagingBuffer.Size() < totalSize) {
//             m_stagingBuffer = CBuffer { m_context, totalSize,
//                 vk::BufferUsageFlagBits::eTransferSrc, MemoryLocation::eHostVisible
//             };
//         }

//         std::memcpy(m_stagingBuffer.Data(), vertices.data(), vSize);
//         std::memcpy(static_cast<std::uint8_t*>(m_stagingBuffer.Data()) + vSize, indices.data(), iSize);

//         m_graphicsCmd.Get().ImmediateSubmit(*m_context.Device().GraphicsQueue(),
//             [&](const CCommandBuffer& cmd) {
//                 cmd.Copy(m_vertexBuffer, m_stagingBuffer, vSize, { 0, mesh.VtxOffset() });
//                 cmd.Copy(m_indexBuffer, m_stagingBuffer, iSize, { vSize, mesh.IdxOffset() } );
//             }
//         );

//         return mesh;
//     };

//     m_vertexBuffer = CBuffer {
//         m_context, GEOMETRY_POOL_SIZE,
//         vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
//         MemoryLocation::eDeviceOnly
//     };

//     m_indexBuffer = CBuffer {
//         m_context, GEOMETRY_POOL_SIZE,
//         vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
//         MemoryLocation::eDeviceOnly
//     };

//     m_meshes.push_back(UploadToPool("assets://matroskin.obj"));
//     m_meshes.push_back(UploadToPool("assets://viking_room.obj"));
// }

// void CRenderer::LoadObjects() {
//     m_objects.emplace_back(0, 0, glm::rotate(glm::translate(glm::identity<glm::mat4>(), glm::vec3 { 0, 0, 1 }), glm::radians(90.0f), glm::vec3 { 1, 0, 0 }));
//     m_objects.emplace_back(1, 1);
// }
}
