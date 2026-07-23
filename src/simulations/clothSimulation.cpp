#include "simulations/clothSimulation.h"
#include "core/shaderUtils.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>
#include "imgui/imgui.h"

struct Vertex
{
  glm::vec2 position;
  glm::vec3 color;
};

Particle::Particle(float x, float y) : currentPosition(x, y), previousPosition(x, y)
{
  this->isPinned = false;
  this->wasPinned = false;
}

void Particle::update(float deltaTime, float drag, const glm::vec2 &gravity)
{
  if (isPinned)
  {
    return;
  }

  glm::vec2 velocity = (currentPosition - previousPosition) * (1.0f - drag);
  previousPosition = currentPosition;

  currentPosition = currentPosition + velocity + gravity * (deltaTime * deltaTime);
}

Constraint::Constraint(unsigned int p0Index, unsigned int p1Index, float length)
{
  this->p0Index = p0Index;
  this->p1Index = p1Index;
  this->length = length;
  this->isActive = true;
}

void Constraint::update(std::vector<Particle> &particles, float elasticity)
{
  if (!isActive)
  {
    return;
  }

  Particle &p0 = particles[p0Index];
  Particle &p1 = particles[p1Index];

  glm::vec2 positionDifference = p0.currentPosition - p1.currentPosition;
  float distance = glm::length(positionDifference);

  if (distance == 0.0f)
  {
    return;
  }

  // Tear the cloth if it is too stretched
  if (distance > length * elasticity)
  {
    isActive = false;

    return;
  }

  float differenceFactor = (length - distance) / distance;
  glm::vec2 offset = positionDifference * (differenceFactor * 0.5f);

  if (!p0.isPinned)
  {
    p0.currentPosition += offset;
  }

  if (!p1.isPinned)
  {
    p1.currentPosition -= offset;
  }
}

void ClothSimulation::rebuildCloth(int width, int height, float spacing)
{
  particles.clear();
  constraints.clear();

  for (int y = 0; y < height; y++)
  {
    for (int x = 0; x < width; x++)
    {
      particles.emplace_back(startX + x * spacing, startY + y * spacing);

      if (y == 0)
      {
        particles.back().isPinned = true;
        particles.back().wasPinned = true;
      }
    }
  }

  for (int y = 0; y < height; y++)
  {
    for (int x = 0; x < width; x++)
    {
      int index = y * width + x;

      if (x < width - 1)
      {
        constraints.emplace_back(index, index + 1, spacing);
      }

      if (y < height - 1)
      {
        constraints.emplace_back(index, index + width, spacing);
      }
    }
  }
}

ClothSimulation::ClothSimulation(int width, int height, float spacing, float startX, float startY, int screenWidth, int screenHeight) : Simulation2D(screenWidth, screenHeight)
{
  this->width = width;
  this->height = height;

  this->startX = startX;
  this->startY = startY;
  this->spacing = spacing;

  rebuildCloth(width, height, spacing);
}

ClothSimulation::~ClothSimulation()
{
  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
  glDeleteProgram(shaderProgram);
}

void ClothSimulation::init()
{
  shaderProgram = ShaderUtils::makeShader("../src/shaders/clothVertex.glsl", "../src/shaders/clothFragment.glsl");

  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);

  // Attribute 0: Position (vec2)
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, position));
  glEnableVertexAttribArray(0);

  // Attribute 1: Color (vec3)
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, color));
  glEnableVertexAttribArray(1);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

void ClothSimulation::update(float deltaTime)
{
  if (isLeftMouseDown)
  {
    if (!draggedPoint)
    {
      float closestDist = 20.0f;
      for (auto &particle : particles)
      {
        float dist = glm::length(particle.currentPosition - worldMousePosition);

        if (dist < closestDist)
        {
          closestDist = dist;

          draggedPoint = &particle;
        }
      }

      if (draggedPoint)
      {
        draggedPoint->isPinned = true;
      }
    }

    if (draggedPoint)
    {
      draggedPoint->currentPosition = worldMousePosition;
      draggedPoint->previousPosition = worldMousePosition;
    }
  }
  else if (draggedPoint)
  {
    draggedPoint->isPinned = draggedPoint->wasPinned;

    draggedPoint = nullptr;
  }

  if (isRightMouseDown)
  {
    for (auto &constraint : constraints)
    {
      if (!constraint.isActive)
      {
        continue;
      }

      glm::vec2 midPoint = (particles[constraint.p0Index].currentPosition + particles[constraint.p1Index].currentPosition) * 0.5f;
      if (glm::length(midPoint - worldMousePosition) < mouseTearRadius)
      {
        constraint.isActive = false;
      }
    }
  }

  mouseTearRadius = 20.0f + (mouseScrollY * 5.0f);

  if (mouseTearRadius < 2.0f)
  {
    mouseTearRadius = 2.0f;
  }

  if (mouseTearRadius > 100.0f)
  {
    mouseTearRadius = 100.0f;
  }

  mouseScrollY = (mouseTearRadius - 20.0f) / 5.0f;

  glm::vec2 appliedForces(windForceX, gravityY);
  appliedForces *= 100.0f;

  for (auto &particle : particles)
  {
    particle.update(deltaTime, drag, appliedForces);
  }

  for (int i = 0; i < constraintIterations; i++)
  {
    for (auto &constraint : constraints)
    {
      constraint.update(particles, elasticity);
    }
  }
}

void ClothSimulation::render()
{
  std::vector<Vertex> vertices;

  glm::vec3 white(1.0f, 1.0f, 1.0f);
  glm::vec3 red(1.0f, 0.0f, 0.0f);

  for (auto &constraint : constraints)
  {
    if (!constraint.isActive)
    {
      continue;
    }

    glm::vec2 midPoint = (particles[constraint.p0Index].currentPosition + particles[constraint.p1Index].currentPosition) * 0.5f;
    glm::vec3 color = (glm::length(midPoint - worldMousePosition) <= mouseTearRadius) ? red : white;

    vertices.push_back({particles[constraint.p0Index].currentPosition, color});
    vertices.push_back({particles[constraint.p1Index].currentPosition, color});
  }

  int stickVertexCount = vertices.size();

  int segments = 32;
  for (int i = 0; i < segments; i++)
  {
    float theta = 2.0f * glm::pi<float>() * static_cast<float>(i) / static_cast<float>(segments);
    glm::vec2 circlePos = worldMousePosition + glm::vec2(mouseTearRadius * cos(theta), mouseTearRadius * sin(theta));
    vertices.push_back({circlePos, red});
  }

  if (vertices.empty())
  {
    return;
  }

  glUseProgram(shaderProgram);

  glm::mat4 viewProjection = getViewProjectionMatrix();
  glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(viewProjection));

  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);

  glDrawArrays(GL_LINES, 0, stickVertexCount);

  glDrawArrays(GL_LINE_LOOP, stickVertexCount, segments);

  glBindVertexArray(0);
}

void ClothSimulation::renderUI()
{
  ImGui::Text("Cloth Physical Properties");

  ImGui::SliderFloat("Gravity (Y)", &gravityY, -20.0f, 100.0f);
  ImGui::SliderFloat("Wind (X)", &windForceX, -50.0f, 50.0f);
  ImGui::SliderFloat("Elasticity", &elasticity, 1.0f, 10.0f);
  ImGui::SliderFloat("Air Drag", &drag, 0.0f, 0.1f);

  ImGui::Spacing();

  ImGui::Text("Structural Changes (Requires Restart)");

  ImGui::SliderInt("Cloth Width", &width, 10, 200);
  ImGui::SliderInt("Cloth Height", &height, 10, 200);
  ImGui::SliderInt("Spacing", &spacing, 5, 50);

  if (ImGui::Button("Rebuild Cloth"))
  {
    draggedPoint = nullptr;

    rebuildCloth(width, height, spacing);
  }
}