#pragma once
#include <skylabs/public/pch.hpp>

namespace SDL::TTF {
class PUBLIC_CLASS CContext {
public:
    explicit CContext();
    explicit CContext(std::nullptr_t) {}
    CContext(const CContext&) = delete;
    CContext(CContext&& other) noexcept;
    CContext& operator=(const CContext&) = delete;
    CContext& operator=(CContext&& other) noexcept;
    ~CContext();

private:
    void Cleanup();

    bool m_initialized = 0;
};
}
