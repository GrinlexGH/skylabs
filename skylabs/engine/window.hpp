#pragma once
#include "skylabs/engine/utils.hpp"

namespace sk {
class IWindow {
public:
    virtual ~IWindow() = default;

    [[nodiscard]] virtual utils::Extent2D DrawableSize() const = 0;
    [[nodiscard]] virtual bool IsMinimized() const = 0;
};
}
