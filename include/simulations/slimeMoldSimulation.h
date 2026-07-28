#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "core/simulation2D.h"

struct Agent
{
  glm::vec2 position;
  float angle;
  unsigned int speciesIndex;
};

struct SpeciesSettings
{
  float moveSpeed = 50.0f;
  float turnSpeed = 10.0f;
  float sensorAngle = 0.392f;
  float sensorDistance = 15.0f;
  unsigned int sensorSize = 1;
  float depositAmount = 2.0f;
  float padding[2]; // Padding for std430 alignment
  glm::vec4 color = glm::vec4(1.0f);
};

class SlimeMoldSimulation : public Simulation2D
{
private:
  unsigned int currentTextureCapacity = 4;

  unsigned int agentSSBO;
  unsigned int speciesSSBO;
  unsigned int trailMapTexture;
  unsigned int diffusedTrailMapTexture;

  unsigned int agentComputeShaderProgram = 0, diffuseComputeShaderProgram = 0;
  unsigned int basicShaderProgram = 0, renderShaderProgram = 0;
  unsigned int quadVAO = 0, quadVBO = 0;
  unsigned int brushVAO = 0, brushVBO = 0;

  std::vector<Agent> agents;
  std::vector<SpeciesSettings> speciesList;

  unsigned int initialAgentCount = 0;
  bool spawnStartingAgents = true;
  unsigned int agentCount;

  float evaporationRate = 0.5f;
  float diffusionRate = 0.9f;

  bool enableDrawing = true;
  float brushRadius = 15.0f;
  unsigned int spawnRate = 10000;
  unsigned int activeUISpecies = 0;

  void createTextureArray(unsigned int &texture, unsigned int textureCapacity);
  void expandTextureArray(unsigned int &oldTexture, unsigned int newCapacity);
  void initTextures();
  void resetAgents();
  void updateSpeciesBuffer();
  void spawnAgentsAt(glm::vec2 pos, unsigned int amount, unsigned int speciesId);

public:
  SlimeMoldSimulation(unsigned int agentCount, unsigned int screenWidth, unsigned int screenHeight);
  ~SlimeMoldSimulation();

  void onResize(int newScreenWidth, int newScreenHeight) override;

  virtual void init() override;

  virtual void update(float deltaTime) override;

  virtual void render() override;

  virtual void renderUI() override;
};