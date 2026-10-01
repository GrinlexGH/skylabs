#include "sk_engine_export.h"
#include "engine.hpp"

SK_ENGINE_PUBLIC_INTERFACE int SkMain(int /*argc*/, char* /*argv*/[]) {
    sk::Engine engine;
    engine.Run();

    return 0;
}
