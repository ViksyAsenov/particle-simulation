#include <config.h>

#include "core/shaderUtils.h"

unsigned int ShaderUtils::makeModule(const std::string &filepath, unsigned int moduleType)
{
  std::ifstream file;
  std::stringstream bufferedLines;
  std::string line;

  file.open(filepath);
  while (std::getline(file, line))
  {
    bufferedLines << line << "\n";
  }

  std::string shaderSource = bufferedLines.str();
  const char *shaderSourceCStr = shaderSource.c_str();

  bufferedLines.str("");
  file.close();

  unsigned int shaderModule = glCreateShader(moduleType);
  glShaderSource(shaderModule, 1, &shaderSourceCStr, NULL);
  glCompileShader(shaderModule);

  int success;
  glGetShaderiv(shaderModule, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    char errorLog[1024];
    glGetShaderInfoLog(shaderModule, 1024, NULL, errorLog);
    std::cout << "Shader module compilation failed: " << errorLog << std::endl;
  }

  return shaderModule;
}

unsigned int ShaderUtils::makeShader(const std::string &vertexFilepath, const std::string &fragmentFilepath)
{
  std::vector<unsigned int> shaderModules;
  shaderModules.push_back(makeModule(vertexFilepath, GL_VERTEX_SHADER));
  shaderModules.push_back(makeModule(fragmentFilepath, GL_FRAGMENT_SHADER));

  unsigned int shaderProgram = glCreateProgram();

  for (unsigned int shaderModule : shaderModules)
  {
    glAttachShader(shaderProgram, shaderModule);
  }

  glLinkProgram(shaderProgram);

  int success;
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success)
  {
    char errorLog[1024];
    glGetProgramInfoLog(shaderProgram, 1024, NULL, errorLog);
    std::cout << "Shader program linking failed: " << errorLog << std::endl;
  }

  for (unsigned int shaderModule : shaderModules)
  {
    glDeleteShader(shaderModule);
  }

  return shaderProgram;
}