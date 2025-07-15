#pragma once

class TomlConfigReader;

struct ApplicationInfo {
    int framesInFlight{};
    bool isFramerateLimited{};
    bool enableFrameTiming{};

    void loadConfig(TomlConfigReader *tomlConfigReader);
};
