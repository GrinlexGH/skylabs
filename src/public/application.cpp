#include "application.hpp"

void CBaseApplication::Run() {
  PreCreate();
  Create();
  PostCreate();

  Main();

  PreDestroy();
  Destroy();
  PostDestroy();
}
