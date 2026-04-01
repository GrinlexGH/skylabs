#include <skylabs/core/render/vulkan/pipeline/shader.hpp>
#include <skylabs/public/filesystem.hpp>

#include <spirv_reflect.h>

namespace Vulkan {
CShader::CShader(const CContext& context, const vk::ShaderStageFlagBits type, const std::string_view name) : m_stage(type) {
    const std::vector<std::uint32_t> code = Filesystem::LoadAsVector32(name);

    vk::ShaderModuleCreateInfo createInfo {};
    createInfo.setCode(code);

    m_handle = vk::raii::ShaderModule { *context.Device(), createInfo };

    Reflect(code);
}

void CShader::Reflect(const std::vector<std::uint32_t>& code) {
    SpvReflectShaderModule module;
    spvReflectCreateShaderModule(code.size() * 4, code.data(), &module);

    std::uint32_t set_count = 0;
    spvReflectEnumerateDescriptorSets(&module, &set_count, nullptr);
    std::vector<SpvReflectDescriptorSet*> sets(set_count);
    spvReflectEnumerateDescriptorSets(&module, &set_count, sets.data());

    for (auto* set : sets) {
        auto& bindings = m_reflection.sets[set->set];
        for (std::uint32_t i = 0; i < set->binding_count; ++i) {
            auto* b = set->bindings[i];
            vk::DescriptorSetLayoutBinding binding;
            binding.binding = b->binding;
            binding.descriptorType = static_cast<vk::DescriptorType>(b->descriptor_type);
            binding.descriptorCount = b->count;
            binding.stageFlags = m_stage;
            bindings.push_back(binding);
        }
    }

    std::uint32_t pc_count = 0;
    spvReflectEnumeratePushConstantBlocks(&module, &pc_count, nullptr);
    std::vector<SpvReflectBlockVariable*> pcs(pc_count);
    spvReflectEnumeratePushConstantBlocks(&module, &pc_count, pcs.data());

    for (auto* pc : pcs) {
        vk::PushConstantRange range;
        range.offset = pc->offset;
        range.size = pc->size;
        range.stageFlags = m_stage;
        m_reflection.pushConstants.push_back(range);
    }

    spvReflectDestroyShaderModule(&module);
}
}
