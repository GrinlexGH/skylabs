#include "skylabs/base/application.hpp"

namespace sk {
void BaseApplication::Run() {
    PreCreate();
    Create();
    PostCreate();

    Main();

    PreDestroy();
    Destroy();
    PostDestroy();
}
}
