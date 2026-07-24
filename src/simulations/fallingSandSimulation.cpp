#include "simulations/fallingSandSimulation.h"
#include "core/shaderUtils.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>
#include <cstdlib>
#include "imgui/imgui.h"
#include <glad/glad.h>

FallingSandSimulation::FallingSandSimulation(int gridWidth, int gridHeight, float cellSize, int screenWidth, int screenHeight)
    : Simulation2D(screenWidth, screenHeight)
{
  this->gridWidth = gridWidth;
  this->gridHeight = gridHeight;
  this->cellSize = cellSize;

  this->nextGridWidth = gridWidth;
  this->nextGridHeight = gridHeight;
  this->nextCellSize = cellSize;

  rebuildGrid(gridWidth, gridHeight, cellSize);
}

FallingSandSimulation::~FallingSandSimulation()
{
  glDeleteVertexArrays(1, &quadVAO);
  glDeleteBuffers(1, &quadVBO);
  glDeleteBuffers(1, &instanceVBO);
  glDeleteProgram(fallingSandShaderProgram);

  glDeleteVertexArrays(1, &lineVAO);
  glDeleteBuffers(1, &lineVBO);
  glDeleteProgram(basicShaderProgram);
}

void FallingSandSimulation::init()
{
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  fallingSandShaderProgram = ShaderUtils::makeShader("../src/shaders/fallingSandSimulation/fallingSandVertex.glsl", "../src/shaders/fallingSandSimulation/fallingSandFragment.glsl");

  float quadVertices[] = {
      0.0f, 0.0f,
      1.0f, 0.0f,
      0.0f, 1.0f,
      1.0f, 1.0f};

  glGenVertexArrays(1, &quadVAO);
  glGenBuffers(1, &quadVBO);
  glGenBuffers(1, &instanceVBO);

  glBindVertexArray(quadVAO);

  // Bind base quad vertices (Location 0)
  glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);

  // Bind instance data
  glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

  // Location 1: Instance Offset
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void *)offsetof(InstanceData, position));
  glVertexAttribDivisor(1, 1); // Tell OpenGL this changes per-instance, not per-vertex

  // Location 2: Instance Color
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void *)offsetof(InstanceData, color));
  glVertexAttribDivisor(2, 1);

  // Location 3: Instance Type
  glEnableVertexAttribArray(3);
  glVertexAttribIPointer(3, 1, GL_UNSIGNED_INT, sizeof(InstanceData), (void *)offsetof(InstanceData, type));
  glVertexAttribDivisor(3, 1);

  basicShaderProgram = ShaderUtils::makeShader("../src/shaders/basicVertex.glsl", "../src/shaders/basicFragment.glsl");

  glGenVertexArrays(1, &lineVAO);
  glGenBuffers(1, &lineVBO);

  glBindVertexArray(lineVAO);
  glBindBuffer(GL_ARRAY_BUFFER, lineVBO);

  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, position));
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, color));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);
}

void FallingSandSimulation::rebuildGrid(int newWidth, int newHeight, float newCellSize)
{
  gridWidth = newWidth;
  gridHeight = newHeight;
  cellSize = newCellSize;

  grid.clear();
  grid.resize(gridWidth * gridHeight, Cell());
}

bool FallingSandSimulation::isInBounds(int x, int y) const
{
  return x >= 0 && x < gridWidth && y >= 0 && y < gridHeight;
}

int FallingSandSimulation::getIndex(int x, int y) const
{
  return y * gridWidth + x;
}

glm::vec3 FallingSandSimulation::getDefaultColor(ParticleType type)
{
  float variation = ((rand() % 20) - 10) / 255.0f;

  switch (type)
  {
  case ParticleType::SAND:
    return glm::vec3(0.9f + variation, 0.8f + variation, 0.4f);
  case ParticleType::WATER:
    return glm::vec3(0.2f, 0.4f + variation, 0.9f + variation);
  case ParticleType::STONE:
    return glm::vec3(0.5f + variation, 0.5f + variation, 0.5f + variation);
  case ParticleType::WOOD:
    return glm::vec3(0.4f + variation, 0.2f + variation, 0.1f);
  case ParticleType::FIRE:
    return glm::vec3(0.9f, 0.3f + (rand() % 30 / 100.0f), 0.1f);
  default:
    return glm::vec3(0.0f);
  }
}

void FallingSandSimulation::setCell(int x, int y, ParticleType type)
{
  if (!isInBounds(x, y))
  {
    return;
  }

  int index = getIndex(x, y);

  grid[index].type = type;
  grid[index].color = getDefaultColor(type);
  grid[index].updatedThisFrame = true;

  if (type == ParticleType::FIRE)
    grid[index].lifeTime = 1.0f + ((rand() % 10) / 10.0f);

  if (type == ParticleType::WATER)
  {
    grid[index].direction = (rand() % 2 == 0) ? -1 : 1;
  }
}

void FallingSandSimulation::swapCells(int x1, int y1, int x2, int y2)
{
  int index1 = getIndex(x1, y1);
  int index2 = getIndex(x2, y2);

  Cell temp = grid[index1];

  grid[index1] = grid[index2];
  grid[index2] = temp;

  grid[index1].updatedThisFrame = true;
  grid[index2].updatedThisFrame = true;
}

void FallingSandSimulation::update(float deltaTime)
{
  totalTime += deltaTime;

  brushSize = static_cast<int>(mouseScrollY);

  if (brushSize < 1)
  {
    brushSize = 1;
  }
  if (brushSize > 50)
  {
    brushSize = 50;
  }

  mouseScrollY = static_cast<float>(brushSize);

  if (isLeftMouseDown || isRightMouseDown)
  {
    int mouseGridX = static_cast<int>(worldMousePosition.x / cellSize);
    int mouseGridY = static_cast<int>(worldMousePosition.y / cellSize);

    ParticleType typeToDraw = isLeftMouseDown ? currentBrushType : ParticleType::EMPTY;

    for (int y = -brushSize; y <= brushSize; y++)
    {
      for (int x = -brushSize; x <= brushSize; x++)
      {
        setCell(mouseGridX + x, mouseGridY + y, typeToDraw);
      }
    }
  }

  for (auto &cell : grid)
  {
    cell.updatedThisFrame = false;
  }

  bool leftToRight = rand() % 2 == 0;
  for (int y = gridHeight - 1; y >= 0; y--)
  {
    for (int i = 0; i < gridWidth; i++)
    {
      int x = leftToRight ? i : (gridWidth - 1 - i);
      int index = getIndex(x, y);

      if (grid[index].updatedThisFrame || grid[index].type == ParticleType::EMPTY)
      {
        continue;
      }

      switch (grid[index].type)
      {
      case ParticleType::SAND:
        updateSand(x, y);
        break;
      case ParticleType::WATER:
        updateWater(x, y);
        break;
      case ParticleType::FIRE:
        updateFire(x, y, deltaTime);
        break;
      case ParticleType::STONE:
      case ParticleType::WOOD:
        break;
      default:
        break;
      }
    }
  }
}

void FallingSandSimulation::updateSand(int x, int y)
{
  if (isInBounds(x, y + 1) && (grid[getIndex(x, y + 1)].type == ParticleType::EMPTY || grid[getIndex(x, y + 1)].type == ParticleType::WATER))
  {
    swapCells(x, y, x, y + 1);
  }
  else
  {
    int direction = (rand() % 2 == 0) ? -1 : 1;

    if (isInBounds(x + direction, y + 1) && (grid[getIndex(x + direction, y + 1)].type == ParticleType::EMPTY || grid[getIndex(x + direction, y + 1)].type == ParticleType::WATER))
    {
      swapCells(x, y, x + direction, y + 1);
    }
    else if (isInBounds(x - direction, y + 1) && (grid[getIndex(x - direction, y + 1)].type == ParticleType::EMPTY || grid[getIndex(x - direction, y + 1)].type == ParticleType::WATER))
    {
      swapCells(x, y, x - direction, y + 1);
    }
  }
}

void FallingSandSimulation::updateWater(int x, int y)
{
  int index = getIndex(x, y);

  if (grid[index].direction == 0)
  {
    grid[index].direction = (rand() % 2 == 0) ? -1 : 1;
  }

  int direction = grid[index].direction;

  if (isInBounds(x, y + 1) && grid[getIndex(x, y + 1)].type == ParticleType::EMPTY)
  {
    swapCells(x, y, x, y + 1);
  }
  else if (isInBounds(x + direction, y + 1) && grid[getIndex(x + direction, y + 1)].type == ParticleType::EMPTY)
  {
    swapCells(x, y, x + direction, y + 1);
  }
  else if (isInBounds(x - direction, y + 1) && grid[getIndex(x - direction, y + 1)].type == ParticleType::EMPTY)
  {
    swapCells(x, y, x - direction, y + 1);
    grid[getIndex(x - direction, y + 1)].direction = -direction;
  }
  else if (isInBounds(x + direction, y) && grid[getIndex(x + direction, y)].type == ParticleType::EMPTY)
  {
    swapCells(x, y, x + direction, y);
  }
  else
  {
    grid[index].direction = -direction;

    if (isInBounds(x - direction, y) && grid[getIndex(x - direction, y)].type == ParticleType::EMPTY)
    {
      swapCells(x, y, x - direction, y);
    }
  }
}

void FallingSandSimulation::updateFire(int x, int y, float deltaTime)
{
  int index = getIndex(x, y);

  grid[index].lifeTime -= deltaTime;
  grid[index].color = getDefaultColor(ParticleType::FIRE);

  if (grid[index].lifeTime <= 0.0f)
  {
    setCell(x, y, ParticleType::EMPTY);

    return;
  }

  if (rand() % 100 < 60)
  {
    int direction = (rand() % 3) - 1;
    if (isInBounds(x + direction, y - 1) && grid[getIndex(x + direction, y - 1)].type == ParticleType::EMPTY)
    {
      swapCells(x, y, x + direction, y - 1);

      x += direction;
      y -= 1;
    }
  }

  int neighbors[4][2] = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}};
  for (auto &offset : neighbors)
  {
    int nextX = x + offset[0];
    int nextY = y + offset[1];

    if (isInBounds(nextX, nextY) && grid[getIndex(nextX, nextY)].type == ParticleType::WOOD)
    {
      if (rand() % 100 < 5)
      {
        setCell(nextX, nextY, ParticleType::FIRE);
      }
    }
  }
}

void FallingSandSimulation::render()
{
  glm::mat4 viewProjection = getViewProjectionMatrix();

  std::vector<InstanceData> instances;

  for (int y = 0; y < gridHeight; y++)
  {
    for (int x = 0; x < gridWidth; x++)
    {
      int index = getIndex(x, y);

      if (grid[index].type != ParticleType::EMPTY)
      {
        glm::vec2 position = glm::vec2(x * cellSize, y * cellSize);

        instances.push_back({position, grid[index].color, static_cast<unsigned int>(grid[index].type)});
      }
    }
  }

  if (!instances.empty())
  {
    glUseProgram(fallingSandShaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(fallingSandShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(viewProjection));
    glUniform1f(glGetUniformLocation(fallingSandShaderProgram, "cellSize"), cellSize);
    glUniform1f(glGetUniformLocation(fallingSandShaderProgram, "time"), totalTime);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, instances.size() * sizeof(InstanceData), instances.data(), GL_DYNAMIC_DRAW);

    // Draw 4 vertices (the base quad) as a Triangle Strip, instantiated for every active particle
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, instances.size());
  }

  // Grid and brush lines
  glUseProgram(basicShaderProgram);
  glUniformMatrix4fv(glGetUniformLocation(basicShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(viewProjection));
  glBindVertexArray(lineVAO);
  glBindBuffer(GL_ARRAY_BUFFER, lineVBO);

  if (showGridLines)
  {
    std::vector<Vertex> lineVertices;
    glm::vec3 gridColor(0.2f, 0.2f, 0.2f);

    for (int x = 0; x <= gridWidth; x++)
    {
      lineVertices.push_back({{x * cellSize, 0}, gridColor});
      lineVertices.push_back({{x * cellSize, gridHeight * cellSize}, gridColor});
    }

    for (int y = 0; y <= gridHeight; y++)
    {
      lineVertices.push_back({{0, y * cellSize}, gridColor});
      lineVertices.push_back({{gridWidth * cellSize, y * cellSize}, gridColor});
    }

    glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(Vertex), lineVertices.data(), GL_STATIC_DRAW);
    glDrawArrays(GL_LINES, 0, lineVertices.size());
  }

  std::vector<Vertex> brushVertices;
  glm::vec3 brushColor(1.0f, 1.0f, 1.0f);

  int mouseGridX = static_cast<int>(worldMousePosition.x / cellSize);
  int mouseGridY = static_cast<int>(worldMousePosition.y / cellSize);

  float startX = (mouseGridX - brushSize) * cellSize;
  float startY = (mouseGridY - brushSize) * cellSize;
  float endX = (mouseGridX + brushSize + 1) * cellSize;
  float endY = (mouseGridY + brushSize + 1) * cellSize;

  brushVertices.push_back({{startX, startY}, brushColor});
  brushVertices.push_back({{endX, startY}, brushColor});
  brushVertices.push_back({{endX, endY}, brushColor});
  brushVertices.push_back({{startX, endY}, brushColor});

  glBufferData(GL_ARRAY_BUFFER, brushVertices.size() * sizeof(Vertex), brushVertices.data(), GL_DYNAMIC_DRAW);
  glDrawArrays(GL_LINE_LOOP, 0, brushVertices.size());

  glBindVertexArray(0);
}

void FallingSandSimulation::renderUI()
{
  ImGui::Text("Materials");

  int type = static_cast<int>(currentBrushType);
  ImGui::RadioButton("Sand", &type, static_cast<int>(ParticleType::SAND));
  ImGui::RadioButton("Water", &type, static_cast<int>(ParticleType::WATER));
  ImGui::RadioButton("Stone", &type, static_cast<int>(ParticleType::STONE));
  ImGui::RadioButton("Wood", &type, static_cast<int>(ParticleType::WOOD));
  ImGui::RadioButton("Fire", &type, static_cast<int>(ParticleType::FIRE));
  currentBrushType = static_cast<ParticleType>(type);

  ImGui::Spacing();

  if (ImGui::Button("Clear Grid"))
  {
    std::fill(grid.begin(), grid.end(), Cell());
  }

  ImGui::Spacing();

  ImGui::Text("Grid Properties");
  ImGui::Checkbox("Show Grid Lines", &showGridLines);
  ImGui::SliderInt("New Width", &nextGridWidth, 30, 500);
  ImGui::SliderInt("New Height", &nextGridHeight, 30, 500);
  ImGui::SliderFloat("New Cell Size", &nextCellSize, 1.0f, 20.0f);

  if (ImGui::Button("Apply Grid Size (Resets Sim)"))
  {
    rebuildGrid(nextGridWidth, nextGridHeight, nextCellSize);
  }
}