#pragma once
#include <cstddef>

#include "sk_base_export.h"

namespace sk::sdl::mixer {
class SK_BASE_PUBLIC_CLASS Context {
public:
    explicit Context();
    explicit Context(std::nullptr_t) { }
    Context(const Context&) = delete;
    Context(Context&& other) noexcept;
    Context& operator=(const Context&) = delete;
    Context& operator=(Context&& other) noexcept;
    ~Context();

private:
    void Cleanup();

    bool m_initialized = 0;
};
}
