#pragma once

#include <string>
#include <vector>

class Logger;
namespace FileReader {
std::string readShaderSourceCode(const std::string &fullPathToFile, Logger *logger);
std::vector<char> readShaderBinary(const std::string &fullPathToFile, Logger *logger);
}; // namespace FileReader
