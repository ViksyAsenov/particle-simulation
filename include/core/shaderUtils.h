#pragma once

#include <config.h>

class ShaderUtils
{
public:
  ShaderUtils() = delete;
  ShaderUtils(const ShaderUtils &other) = delete;
  ShaderUtils &operator=(const ShaderUtils &other) = delete;

  static unsigned int makeModule(const std::string &filepath, unsigned int moduleType);

  static unsigned int makeShader(const std::string &vertexFilepath, const std::string &fragmentFilepath);
};