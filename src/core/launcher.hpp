#pragma once

#include "application.hpp"

class CLauncher final : public CBaseApplication {
public:
    CLauncher()     = default;
    ~CLauncher()    = default;

    void Main() override;
};
