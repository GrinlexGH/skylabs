#include "launcher.hpp"
#include "dll_export.hpp"
#include "logging.hpp"

extern "C" DLL_EXPORT int CoreMain(const int /*argc*/, char* /*argv*/[]) {
    CLauncher launcher;
    launcher.Run();

    return 0;
}
