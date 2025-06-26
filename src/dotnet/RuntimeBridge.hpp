#pragma once

class Logger;
class RuntimeApplication;

namespace RuntimeBridge {
void bootstrap(Logger *logger);
RuntimeApplication &getRuntimeApplication();
} // namespace RuntimeBridge
