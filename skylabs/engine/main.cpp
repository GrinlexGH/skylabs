#include <tracy/Tracy.hpp>

#include "sk_engine_export.h"
#include "engine.hpp"

SK_ENGINE_PUBLIC_INTERFACE int SkMain(int /*argc*/, char* /*argv*/[]) {
#ifdef TRACY_ENABLE
    tracy::StartupProfiler();
#endif

    {
        sk::Engine engine;
        engine.Run();
    }

#ifdef TRACY_ENABLE
    tracy::ShutdownProfiler();
#endif

    return 0;
}
