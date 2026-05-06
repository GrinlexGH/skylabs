#include <skylabs/core/render/vulkan/pipeline/descriptor_writer.hpp>

namespace Vulkan {
CDescriptorWriter::CDescriptorWriter(const CContext& context) : m_device(&*context.Device()) {}

CDescriptorWriter& CDescriptorWriter::WriteBuffer(
    std::uint32_t binding,
    vk::Buffer buffer,
    std::size_t size,
    std::size_t offset,
    vk::DescriptorType type,
    std::uint32_t arrayElement
) {
    vk::DescriptorBufferInfo& info = m_bufferInfos.emplace_back(buffer, offset, size);

    vk::WriteDescriptorSet write {};
    write.dstBinding = binding;
    write.dstArrayElement = arrayElement;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pBufferInfo = &info;

    m_writes.push_back(write);
    return *this;
}

CDescriptorWriter& CDescriptorWriter::WriteImage(
    std::uint32_t binding,
    vk::ImageView view,
    vk::Sampler sampler,
    vk::ImageLayout layout,
    vk::DescriptorType type,
    std::uint32_t arrayElement
) {
    vk::DescriptorImageInfo& info = m_imageInfos.emplace_back(sampler, view, layout);

    vk::WriteDescriptorSet write{};
    write.dstBinding = binding;
    write.dstArrayElement = arrayElement;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pImageInfo = &info;

    m_writes.push_back(write);
    return *this;
}

void CDescriptorWriter::UpdateSet(vk::DescriptorSet set) {
    for (auto& write : m_writes) { write.dstSet = set; }
    m_device->updateDescriptorSets(m_writes, nullptr);
}

void CDescriptorWriter::Clear() {
    m_imageInfos.clear();
    m_bufferInfos.clear();
    m_writes.clear();
}
}
