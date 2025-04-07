#include "VEngine.hpp"

#include <iostream>

class SandboxApplication : public VEngine::Application {
public:
  SandboxApplication() {
    // Initialization code here
    std::cout << "SandboxApplication initialized." << std::endl;
  }
  ~SandboxApplication() {}
};

VEngine::Application *VEngine::createApplication(int /*argc*/, char ** /*argv*/) {
  return new SandboxApplication();
}
