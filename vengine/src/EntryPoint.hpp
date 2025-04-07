#pragma once

#include "Application.hpp"

extern VEngine::Application *VEngine::createApplication(int argc, char **argv);

int main(int argc, char **argv) {
  auto app = VEngine::createApplication(argc, argv);
  app->run();
  delete app;
  return 0;
}
