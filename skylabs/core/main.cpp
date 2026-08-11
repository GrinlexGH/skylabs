#include <skylabs/core/launcher.hpp>
#include <skylabs/public/dll_export.hpp>

extern "C" DLL_EXPORT int CoreMain(int /*argc*/, char* /*argv*/[]) {
    CLauncher launcher;
    launcher.Run();

    return 0;
}
