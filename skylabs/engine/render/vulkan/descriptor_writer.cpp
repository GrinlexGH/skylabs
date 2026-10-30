#include "skylabs/engine/render/vulkan/descriptor_writer.hpp"

namespace sk::render::vulkan {
DescriptorWriter::DescriptorWriter(const vk::raii::Device& device) : m_device(&device) { }

DescriptorWriter& DescriptorWriter::WriteBuffer(std::uint32_t binding, vk::Buffer buffer,
                                                std::size_t size, std::size_t offset,
                                                vk::DescriptorType type, std::uint32_t arrayElement) {
    vk::DescriptorBufferInfo bufferInfo { };
    bufferInfo.buffer = buffer;
    bufferInfo.offset = offset;
    bufferInfo.range = size;
    m_bufferInfos.push_back(bufferInfo);

    vk::WriteDescriptorSet write { };
    write.dstBinding = binding;
    write.dstArrayElement = arrayElement;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pBufferInfo = &m_bufferInfos.back();

    m_writes.push_back(write);
    return *this;
}

DescriptorWriter& DescriptorWriter::WriteImage(std::uint32_t binding, vk::ImageView view,
                                               vk::Sampler sampler, vk::ImageLayout layout,
                                               vk::DescriptorType type, std::uint32_t arrayElement) {
    vk::DescriptorImageInfo imageInfo { };
    imageInfo.sampler = sampler;
    imageInfo.imageView = view;
    imageInfo.imageLayout = layout;
    m_imageInfos.push_back(imageInfo);

    vk::WriteDescriptorSet write { };
    write.dstBinding = binding;
    write.dstArrayElement = arrayElement;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pImageInfo = &m_imageInfos.back();

    m_writes.push_back(write);
    return *this;
}

void DescriptorWriter::UpdateSet(vk::DescriptorSet set) {
    for (auto& write : m_writes) {
        write.dstSet = set;
    }
    m_device->updateDescriptorSets(m_writes, nullptr);
}

void DescriptorWriter::Clear() {
    m_imageInfos.clear();
    m_bufferInfos.clear();
    m_writes.clear();
}
}
