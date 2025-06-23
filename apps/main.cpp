#include "app/Application.hpp"
#include "utils/logger/Logger.hpp"

#include <memory>

int main() {
    Logger logger{};
    // Application app{&logger};
    {
        auto app = std::make_unique<Application>(&logger);
        app->run();
    }
    logger.info("Application should have finished running");

    return EXIT_SUCCESS;
}
