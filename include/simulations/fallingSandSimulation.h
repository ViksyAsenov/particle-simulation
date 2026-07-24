#pragma once

#include "core/simulation2D.h"
#include <vector>
#include <glm/glm.hpp>

enum class ParticleType
{
  EMPTY,
  SAND,
  WATER,
  STONE,
  WOOD,
  FIRE
};

struct Cell
{
  ParticleType type = ParticleType::EMPTY;
  glm::vec3 color = glm::vec3(0.0f);
  float lifeTime = 0.0f;
  int direction = 0;
  bool updatedThisFrame = false;
};

struct InstanceData
{
  glm::vec2 position;
  glm::vec3 color;
  unsigned int type;
};

class FallingSandSimulation : public Simulation2D
{
private:
  int gridWidth;
  int gridHeight;
  float cellSize;

  int nextGridWidth;
  int nextGridHeight;
  float nextCellSize;
  bool showGridLines = true;

  std::vector<Cell> grid;
  float totalTime = 0.0f;

  unsigned int quadVAO = 0, quadVBO = 0, instanceVBO = 0, fallingSandShaderProgram = 0;

  unsigned int lineVAO = 0, lineVBO = 0, basicShaderProgram = 0;

  ParticleType currentBrushType = ParticleType::SAND;
  int brushSize = 1;

  bool isInBounds(int x, int y) const;
  int getIndex(int x, int y) const;
  void setCell(int x, int y, ParticleType type);
  void swapCells(int x1, int y1, int x2, int y2);
  glm::vec3 getDefaultColor(ParticleType type);
  void rebuildGrid(int newWidth, int newHeight, float newCellSize);

  void updateSand(int x, int y);
  void updateWater(int x, int y);
  void updateFire(int x, int y, float deltaTime);

public:
  FallingSandSimulation(int gridWidth, int gridHeight, float cellSize, int screenWidth, int screenHeight);
  ~FallingSandSimulation();

  void init() override;

  void update(float deltaTime) override;

  void render() override;

  void renderUI() override;
};