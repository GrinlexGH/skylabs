#pragma once
#include <skylabs/core/pch.hpp>

namespace Vulkan::Utils {
struct PNextChain {
    void* m_head = nullptr;

    template<typename T>
    requires requires(T t) {
        { t.pNext };
    }
    void Add(T& s) {
        s.pNext = m_head;
        m_head = &s;
    }
};
}
