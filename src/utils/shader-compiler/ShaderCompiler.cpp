#include "ShaderCompiler.hpp"

#include "CustomFileIncluder.hpp"
#include "utils/logger/Logger.hpp"

struct PathInfo {
  std::string fullPathToDir;
  std::string fileName;
};

namespace {
// input: a/b/c.glsl
// output: {a/b/, c.glsl}
PathInfo _getFullDirAndFileName(const std::string &fullPath, Logger *logger) {
  std::string dirName;
  std::string fileName;
  size_t found = fullPath.find_last_of('/');
  if (found != std::string::npos) {
    dirName  = fullPath.substr(0, found + 1);
    fileName = fullPath.substr(found + 1);
  } else {
    logger->error("failed to parse path: {}", fullPath);
  }
  return {dirName, fileName};
}

shaderc_shader_kind _getShaderKind(ShaderStage stage) {
  switch (stage) {
  case ShaderStage::kInferFromSource:
    return shaderc_glsl_infer_from_source;
  case ShaderStage::kCompute:
    return shaderc_glsl_compute_shader;
  case ShaderStage::kFrag:
    return shaderc_glsl_fragment_shader;
  case ShaderStage::kVert:
    return shaderc_glsl_vertex_shader;
  }
}
}; // namespace

ShaderCompiler::ShaderCompiler(Logger *logger,
                               std::function<void(std::string const &)> &&includeCallback)
    : _logger(logger) {
  std::unique_ptr<CustomFileIncluder> fileIncluder =
      std::make_unique<CustomFileIncluder>(logger, std::move(includeCallback));

  // _defaultOptions takes the ownership of fileIncluder, but doesn't provide a way to retrieve it,
  // so we need to store it as a raw pointer
  _fileIncluder = fileIncluder.get();

  _defaultOptions.SetIncluder(std::move(fileIncluder));
  // _defaultOptions.SetTargetSpirv(shaderc_spirv_version_1_3);
  _defaultOptions.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_1);
  _defaultOptions.SetOptimizationLevel(shaderc_optimization_level_performance);
}

std::optional<std::vector<uint32_t>>
ShaderCompiler::compileShaderFromFile(ShaderStage shaderStage, const std::string &fullPathToFile,
                                      std::string const &sourceCode) {
  auto const fullDirAndFileName = _getFullDirAndFileName(fullPathToFile, _logger);

  _fileIncluder->setIncludeDir(fullDirAndFileName.fullPathToDir);

  std::optional<std::vector<uint32_t>> res = std::nullopt;
  shaderc_shader_kind const kind           = _getShaderKind(shaderStage);

  // from shaderc's doc:
  // the input_file_name is used as a tag to identify the source string in cases like emitting error
  // messages, it doesn't have to be a file name
  shaderc::SpvCompilationResult compilationResult = this->CompileGlslToSpv(
      sourceCode, kind, fullDirAndFileName.fileName.c_str(), _defaultOptions);

  if (compilationResult.GetCompilationStatus() != shaderc_compilation_status_success) {
    _logger->warn(compilationResult.GetErrorMessage());
    return std::nullopt;
  }
  return std::vector<uint32_t>(compilationResult.cbegin(), compilationResult.cend());
}
