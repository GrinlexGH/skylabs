#include "sk_core_export.h"
#include "skylabs/core/launcher.hpp"

SK_CORE_PUBLIC_INTERFACE int SkMain(int /*argc*/, char* /*argv*/[]) {
    sk::Launcher launcher;
    launcher.Run();

    return 0;
}
