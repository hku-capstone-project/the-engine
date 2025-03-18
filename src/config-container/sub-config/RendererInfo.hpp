#pragma once

class TomlConfigReader;

struct RendererInfo {

  void loadConfig(TomlConfigReader *tomlConfigReader);
};
