#include "app/Application.hpp"
#include "utils/logger/Logger.hpp"

#include <memory>

int main() {
    Logger logger{};
    {
        auto app = std::make_unique<Application>(&logger);
        app->run();
    }
    return EXIT_SUCCESS;
}
