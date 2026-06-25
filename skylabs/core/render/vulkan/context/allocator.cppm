export module skylabs.vulkan.context:allocator;
export import :device;
export import vk_mem_alloc;

export namespace Vulkan {
class CAllocator
{
public:
    explicit CAllocator(std::nullptr_t) {}
    explicit CAllocator(
        const CInstance& instance,
        const vk::raii::PhysicalDevice& physicalDevice,
        const CDevice& device
    );
    CAllocator(const CAllocator&) = delete;
    CAllocator(CAllocator&& other) noexcept = default;
    CAllocator& operator=(const CAllocator&) = delete;
    CAllocator& operator=(CAllocator&& rhs) noexcept = default;
    ~CAllocator() = default;

    [[nodiscard]] const vma::raii::Allocator& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vma::raii::Allocator* operator->() const noexcept { return &m_handle; }

private:
    vma::raii::Allocator m_handle { nullptr };
};
}
