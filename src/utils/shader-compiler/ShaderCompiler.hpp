#pragma once

#include "shaderc/shaderc.hpp"
#include "utils/io/FileReader.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>

class Logger;
class CustomFileIncluder;

enum class ShaderStage : uint32_t {
  kInferFromSource,
  kCompute,
  kFrag,
  kVert,
  // to be appended
};

class ShaderCompiler : public shaderc::Compiler {
public:
  ShaderCompiler(Logger *logger,
                 std::function<void(std::string const &)> &&includeCallback = nullptr);

  std::optional<std::vector<uint32_t>> compileShaderFromFile(ShaderStage shaderStage,
                                                             const std::string &fullPathToFile,
                                                             std::string const &sourceCode);

private:
  Logger *_logger;
  shaderc::CompileOptions _defaultOptions;
  CustomFileIncluder *_fileIncluder;

}; // namespace ShaderCompiler
