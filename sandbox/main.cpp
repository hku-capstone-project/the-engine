#include "VEngine.hpp"
#include "src/Application.hpp"

#include <iostream>
#include <memory>


class SandboxApplication : public VEngine::Application {
public:
  SandboxApplication() {
    // Initialization code here
    std::cout << "SandboxApplication initialized." << std::endl;
    
#ifdef VENGINE_BUILD_DLL
    std::cout << "VEngine is built as a DLL." << std::endl;
#else
    std::cout << "VEngine is not built as a DLL." << std::endl;
#endif
  }
  ~SandboxApplication() {}
};

int main() {
  std::unique_ptr<VEngine::Application> app = std::make_unique<SandboxApplication>();
  app->run();
  return 0;
}
