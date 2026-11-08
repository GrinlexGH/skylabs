#include <tracy/Tracy.hpp>

#include "sk_engine_export.h"
#include "skylabs/engine/engine.hpp"

SK_ENGINE_PUBLIC_INTERFACE int SkMain(int /*argc*/, char* /*argv*/[]) {
#ifdef TRACY_ENABLE
    tracy::StartupProfiler();
#endif

    try {
        sk::Engine engine;
        engine.Run();
    } catch (const std::exception& e) {
        fmt::println("{}", e.what());
        return 1;
    }

#ifdef TRACY_ENABLE
    tracy::ShutdownProfiler();
#endif

    return 0;
}
