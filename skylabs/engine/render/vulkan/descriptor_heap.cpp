#include "skylabs/engine/render/vulkan/descriptor_heap.hpp"
#include "skylabs/engine/utils.hpp"

namespace sk::render::vulkan {
DescriptorHeap::DescriptorHeap(const Device& device, const vma::raii::Allocator& allocator)
    : m_device(&device), m_allocator(&allocator) {
    m_heapProperties = device.GetPhysicalDevice()
                           ->getProperties2<vk::PhysicalDeviceProperties2,
                                            vk::PhysicalDeviceDescriptorHeapPropertiesEXT>()
                           .get<vk::PhysicalDeviceDescriptorHeapPropertiesEXT>();

    CreateResourceHeap(device, allocator);
    CreateSamplerHeap(device, allocator);
}

void DescriptorHeap::BeginFrame(const vk::raii::CommandBuffer& cmd) const {
    vk::BindHeapInfoEXT bind { };
    bind.heapRange =
        vk::DeviceAddressRangeEXT { m_resourceHeap.heap.Address(), m_resourceHeap.heap.Size() };
    bind.reservedRangeOffset = m_resourceHeap.reservedOff;
    bind.reservedRangeSize = m_resourceHeap.reservedSize;
    cmd.bindResourceHeapEXT(bind);

    bind.heapRange =
        vk::DeviceAddressRangeEXT { m_samplerHeap.heap.Address(), m_samplerHeap.heap.Size() };
    bind.reservedRangeOffset = m_samplerHeap.reservedOff;
    bind.reservedRangeSize = m_samplerHeap.reservedSize;
    cmd.bindSamplerHeapEXT(bind);
}

void DescriptorHeap::Bind(const vk::raii::CommandBuffer& cmd) const {
    vk::DeviceAddress address = m_indexArray.Address();
    vk::PushDataInfoEXT push { };
    push.offset = 0;
    push.data = vk::HostAddressRangeConstEXT { &address, sizeof(address) };
    cmd.pushDataEXT(push);
}

void DescriptorHeap::AllocateAndWriteDescriptors(const std::vector<Buffer>& ubo, const Image& texture) {
    auto allocate = [](Buffer& heap, const vk::DeviceSize size, const vk::DeviceSize alignment) {
        vma::VirtualAllocationCreateInfo ci;
        ci.size = size;
        ci.alignment = alignment;
        return heap.VirtualBlock().allocate(ci);
    };

    auto hostRange = [](Buffer& heap, const vma::raii::VirtualAllocation& a) {
        const auto info = a.getInfo();
        return vk::HostAddressRangeEXT { static_cast<std::byte*>(heap.Data()) + info.offset, info.size };
    };

    const auto frameCount = ubo.size();

    std::vector<vk::DeviceAddressRangeEXT> uboRanges;
    std::vector<vk::ResourceDescriptorInfoEXT> resourceInfos;
    std::vector<vk::HostAddressRangeEXT> resourceDsts;
    uboRanges.reserve(frameCount);
    resourceInfos.reserve(frameCount + 1);
    resourceDsts.reserve(frameCount + 1);

    m_uboDescriptor.reserve(frameCount);

    for (const auto i : utils::Range(frameCount)) {
        uboRanges.emplace_back(ubo[i].Address(), ubo[i].Size());

        vk::ResourceDescriptorInfoEXT info { };
        info.type = vk::DescriptorType::eUniformBuffer;
        info.data.pAddressRange = &uboRanges.back();
        resourceInfos.push_back(info);

        m_uboDescriptor.push_back(allocate(m_resourceHeap.heap, m_heapProperties.bufferDescriptorSize,
                                           m_heapProperties.bufferDescriptorAlignment));
        resourceDsts.push_back(hostRange(m_resourceHeap.heap, m_uboDescriptor.back()));
    }

    vk::ImageViewCreateInfo textureViewCreateInfo { };
    textureViewCreateInfo.image = *texture;
    textureViewCreateInfo.viewType = vk::ImageViewType::e2D;
    textureViewCreateInfo.format = texture.Format();
    textureViewCreateInfo.subresourceRange = texture.FullRange();
    const vk::ImageDescriptorInfoEXT image { &textureViewCreateInfo,
                                             vk::ImageLayout::eShaderReadOnlyOptimal };

    vk::ResourceDescriptorInfoEXT imageInfo { };
    imageInfo.type = vk::DescriptorType::eSampledImage;
    imageInfo.data.pImage = &image;
    resourceInfos.push_back(imageInfo);

    m_imageDescriptor = allocate(m_resourceHeap.heap, m_heapProperties.imageDescriptorSize,
                                 m_heapProperties.imageDescriptorAlignment);
    resourceDsts.push_back(hostRange(m_resourceHeap.heap, m_imageDescriptor));

    (*m_device)->writeResourceDescriptorsEXT(resourceInfos, resourceDsts);

    vk::SamplerCreateInfo samplerCreateInfo { };
    samplerCreateInfo.maxLod = vk::LodClampNone;
    samplerCreateInfo.magFilter = vk::Filter::eLinear;
    samplerCreateInfo.minFilter = vk::Filter::eLinear;
    samplerCreateInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    samplerCreateInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
    samplerCreateInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
    samplerCreateInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;

    m_samplerDescriptor = allocate(m_samplerHeap.heap, m_heapProperties.samplerDescriptorSize,
                                   m_heapProperties.samplerDescriptorAlignment);
    const auto samplerDst = hostRange(m_samplerHeap.heap, m_samplerDescriptor);
    (*m_device)->writeSamplerDescriptorsEXT(samplerCreateInfo, samplerDst);
}

void DescriptorHeap::CreateIndexArray() {
    // Construct 32-bit addresses (offsets)
    m_indexArray =
        Buffer { *m_device, *m_allocator,
                 (kMaxSamplerDescriptors + kMaxResourceDescriptors) * 4 * m_uboDescriptor.size(),
                 MemoryLocation::eHostVisible, vk::BufferUsageFlagBits2::eUniformBuffer };

    // [ubo][img][spl][ubo][img][spl]...
    std::vector<std::uint32_t> a;
    for (auto i : utils::Range(m_uboDescriptor.size())) {
        a.push_back(static_cast<std::uint32_t>(m_uboDescriptor[i].getInfo().offset));
        a.push_back(static_cast<std::uint32_t>(m_imageDescriptor.getInfo().offset));
        a.push_back(static_cast<std::uint32_t>(m_samplerDescriptor.getInfo().offset));
    }
    std::memcpy(m_indexArray.Data(), a.data(), 4 * 3);
}

void DescriptorHeap::CreateResourceHeap(const Device& device, const vma::raii::Allocator& allocator) {
    const auto heapUserSize =
        kMaxResourceDescriptors * utils::AlignUp(std::max(m_heapProperties.bufferDescriptorSize,
                                                          m_heapProperties.imageDescriptorSize),
                                                 m_heapProperties.resourceHeapAlignment);

    m_resourceHeap.reservedOff = heapUserSize;
    m_resourceHeap.reservedSize = m_heapProperties.minResourceHeapReservedRange;
    m_resourceHeap.heap = Buffer { device,
                                   allocator,
                                   m_resourceHeap.reservedOff + m_resourceHeap.reservedSize,
                                   MemoryLocation::eHostVisible,
                                   vk::BufferUsageFlagBits2::eDescriptorHeapEXT,
                                   m_resourceHeap.reservedOff };
}

void DescriptorHeap::CreateSamplerHeap(const Device& device, const vma::raii::Allocator& allocator) {
    const auto heapUserSize =
        kMaxSamplerDescriptors *
        utils::AlignUp(m_heapProperties.samplerDescriptorSize, m_heapProperties.samplerHeapAlignment);

    m_samplerHeap.reservedOff = heapUserSize;
    m_samplerHeap.reservedSize = m_heapProperties.minSamplerHeapReservedRange;
    m_samplerHeap.heap = Buffer { device,
                                  allocator,
                                  m_samplerHeap.reservedOff + m_samplerHeap.reservedSize,
                                  MemoryLocation::eHostVisible,
                                  vk::BufferUsageFlagBits2::eDescriptorHeapEXT,
                                  m_samplerHeap.reservedOff };
}
}
