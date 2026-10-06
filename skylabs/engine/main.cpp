#include <tracy/Tracy.hpp>

#include "sk_engine_export.h"
#include "engine.hpp"

SK_ENGINE_PUBLIC_INTERFACE int SkMain(int /*argc*/, char* /*argv*/[]) {
    tracy::StartupProfiler();
    {
        sk::Engine engine;
        engine.Run();
    }
    tracy::ShutdownProfiler();
    return 0;
}
