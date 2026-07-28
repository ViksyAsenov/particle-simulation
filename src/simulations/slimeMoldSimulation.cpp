#include "simulations/slimeMoldSimulation.h"
#include "core/shaderUtils.h"
#include <config.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>
#include <random>
#include <algorithm>
#include <string>
#include "imgui/imgui.h"

SlimeMoldSimulation::SlimeMoldSimulation(unsigned int agentCount, unsigned int screenWidth, unsigned int screenHeight)
    : Simulation2D(screenWidth, screenHeight)
{
  this->initialAgentCount = agentCount;
  this->agentCount = agentCount;

  setCameraFocus(glm::vec2(screenWidth / 2.0f, screenHeight / 2.0f), 1.0f);
}

SlimeMoldSimulation::~SlimeMoldSimulation()
{
  glDeleteBuffers(1, &agentSSBO);
  glDeleteBuffers(1, &speciesSSBO);
  glDeleteTextures(1, &trailMapTexture);
  glDeleteTextures(1, &diffusedTrailMapTexture);
  glDeleteVertexArrays(1, &quadVAO);
  glDeleteBuffers(1, &quadVBO);
  glDeleteVertexArrays(1, &brushVAO);
  glDeleteBuffers(1, &brushVBO);
  glDeleteProgram(agentComputeShaderProgram);
  glDeleteProgram(diffuseComputeShaderProgram);
  glDeleteProgram(renderShaderProgram);
  glDeleteProgram(basicShaderProgram);
}

void SlimeMoldSimulation::createTextureArray(unsigned int &texture, unsigned int textureCapacity)
{
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D_ARRAY, texture);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R32F, screenWidth, screenHeight, textureCapacity, 0, GL_RED, GL_FLOAT, NULL);
}

void SlimeMoldSimulation::expandTextureArray(unsigned int &oldTexture, unsigned int newCapacity)
{
  unsigned int newTexture;
  createTextureArray(newTexture, newCapacity);

  glCopyImageSubData(oldTexture, GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0,
                     newTexture, GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0,
                     screenWidth, screenHeight, currentTextureCapacity);

  glDeleteTextures(1, &oldTexture);

  oldTexture = newTexture;
}

void SlimeMoldSimulation::initTextures()
{
  if (trailMapTexture != 0)
  {
    glDeleteTextures(1, &trailMapTexture);
  }

  if (diffusedTrailMapTexture != 0)
  {
    glDeleteTextures(1, &diffusedTrailMapTexture);
  }

  createTextureArray(trailMapTexture, currentTextureCapacity);
  createTextureArray(diffusedTrailMapTexture, currentTextureCapacity);
}

void SlimeMoldSimulation::updateSpeciesBuffer()
{
  if (speciesList.empty())
  {
    return;
  }

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, speciesSSBO);
  glBufferData(GL_SHADER_STORAGE_BUFFER, speciesList.size() * sizeof(SpeciesSettings), speciesList.data(), GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, speciesSSBO);
}

void SlimeMoldSimulation::resetAgents()
{
  agentCount = spawnStartingAgents ? initialAgentCount : 0;

  // Keep buffer size at least 1 to avoid OpenGL allocation errors on some drivers
  agents.resize(std::max(agentCount, 1u));

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<float> distanceRadius(0.0f, screenHeight * 0.4f);
  std::uniform_real_distribution<float> distanceAngle(0.0f, 2.0f * glm::pi<float>());

  glm::vec2 center(screenWidth / 2.0f, screenHeight / 2.0f);
  int numSpecies = std::max((int)speciesList.size(), 1);

  for (int i = 0; i < agentCount; i++)
  {
    float r = distanceRadius(gen);
    float theta = distanceAngle(gen);
    agents[i].position = center + glm::vec2(r * cos(theta), r * sin(theta));
    agents[i].angle = theta + glm::pi<float>();
    agents[i].speciesIndex = i % numSpecies;
  }

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, agentSSBO);
  glBufferData(GL_SHADER_STORAGE_BUFFER, agents.size() * sizeof(Agent), agents.data(), GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, agentSSBO);
}

void SlimeMoldSimulation::spawnAgentsAt(glm::vec2 pos, unsigned int amount, unsigned int speciesId)
{
  std::vector<Agent> newAgents;
  for (int i = 0; i < amount; i++)
  {
    Agent a;

    float rand1 = (float)rand() / RAND_MAX;
    float rand2 = (float)rand() / RAND_MAX;

    float spawnAngle = rand1 * 2.0f * glm::pi<float>();
    float spawnRadius = brushRadius * std::sqrt(rand2);

    glm::vec2 offset = glm::vec2(std::cos(spawnAngle), std::sin(spawnAngle)) * spawnRadius;
    a.position = pos + offset;

    a.angle = (rand() % 360) * glm::pi<float>() / 180.0f;
    a.speciesIndex = speciesId;

    newAgents.push_back(a);
  }

  int newTotalCount = agentCount + amount;

  unsigned int newSSBO;
  glGenBuffers(1, &newSSBO);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, newSSBO);
  glBufferData(GL_SHADER_STORAGE_BUFFER, newTotalCount * sizeof(Agent), NULL, GL_DYNAMIC_DRAW);

  if (agentCount > 0)
  {
    glBindBuffer(GL_COPY_READ_BUFFER, agentSSBO);
    glBindBuffer(GL_COPY_WRITE_BUFFER, newSSBO);
    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, agentCount * sizeof(Agent));
  }

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, newSSBO);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, agentCount * sizeof(Agent), amount * sizeof(Agent), newAgents.data());

  if (agentSSBO != 0)
  {
    glDeleteBuffers(1, &agentSSBO);
  }

  agentSSBO = newSSBO;
  agentCount = newTotalCount;
}

void SlimeMoldSimulation::onResize(int newScreenWidth, int newScreenHeight)
{
  unsigned int oldWidth = screenWidth;
  unsigned int oldHeight = screenHeight;

  Simulation2D::onResize(newScreenWidth, newScreenHeight);

  setCameraFocus(glm::vec2(screenWidth / 2.0f, screenHeight / 2.0f), 1.0f);

  unsigned int newTrailMap, newDiffusedMap;
  createTextureArray(newTrailMap, currentTextureCapacity);
  createTextureArray(newDiffusedMap, currentTextureCapacity);

  unsigned int copyWidth = std::min(oldWidth, screenWidth);
  unsigned int copyHeight = std::min(oldHeight, screenHeight);

  if (trailMapTexture != 0)
  {
    glCopyImageSubData(trailMapTexture, GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0,
                       newTrailMap, GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0,
                       copyWidth, copyHeight, currentTextureCapacity);
    glDeleteTextures(1, &trailMapTexture);
  }

  if (diffusedTrailMapTexture != 0)
  {
    glCopyImageSubData(diffusedTrailMapTexture, GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0,
                       newDiffusedMap, GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0,
                       copyWidth, copyHeight, currentTextureCapacity);
    glDeleteTextures(1, &diffusedTrailMapTexture);
  }

  trailMapTexture = newTrailMap;
  diffusedTrailMapTexture = newDiffusedMap;

  float mapW = static_cast<float>(screenWidth);
  float mapH = static_cast<float>(screenHeight);
  float quadVertices[] = {
      0.0f, mapH, 0.0f, 1.0f,
      0.0f, 0.0f, 0.0f, 0.0f,
      mapW, 0.0f, 1.0f, 0.0f,

      0.0f, mapH, 0.0f, 1.0f,
      mapW, 0.0f, 1.0f, 0.0f,
      mapW, mapH, 1.0f, 1.0f};

  glBindVertexArray(quadVAO);
  glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

void SlimeMoldSimulation::init()
{
  SpeciesSettings explorers;
  explorers.color = glm::vec4(1.0f, 0.0f, 0.2f, 1.0f);
  explorers.moveSpeed = 70.0f;
  explorers.turnSpeed = 12.0f;
  explorers.sensorAngle = 0.785f;   // 45 degrees: wide search area
  explorers.sensorDistance = 25.0f; // Looks moderately far ahead
  explorers.sensorSize = 1;
  explorers.depositAmount = 1.0f; // Light trails that decay faster
  speciesList.push_back(explorers);

  SpeciesSettings builders;
  builders.color = glm::vec4(0.2f, 1.0f, 0.2f, 1.0f);
  builders.moveSpeed = 25.0f;
  builders.turnSpeed = 3.0f;       // Sluggish turning creates straight paths
  builders.sensorAngle = 0.174f;   // 10 degrees: highly focused forward vision
  builders.sensorDistance = 40.0f; // Looks very far ahead
  builders.sensorSize = 2;         // Wider sensors to detect broader trails
  builders.depositAmount = 3.5f;   // Heavy deposits create strong gravitational pull
  speciesList.push_back(builders);

  SpeciesSettings swarm;
  swarm.color = glm::vec4(1.0f, 0.45f, 0.0f, 1.0f);
  swarm.moveSpeed = 45.0f;
  swarm.turnSpeed = 40.0f;     // Snaps to new directions almost instantly
  swarm.sensorAngle = 1.57f;   // 90 degrees: sees things completely sideways
  swarm.sensorDistance = 8.0f; // Near-sighted
  swarm.sensorSize = 1;
  swarm.depositAmount = 2.0f;
  speciesList.push_back(swarm);

  agentComputeShaderProgram = ShaderUtils::makeComputeShader("../src/shaders/slimeMoldSimulation/physarumAgent.comp");
  diffuseComputeShaderProgram = ShaderUtils::makeComputeShader("../src/shaders/slimeMoldSimulation/physarumDiffuse.comp");

  renderShaderProgram = ShaderUtils::makeShader("../src/shaders/slimeMoldSimulation/quadVertex.glsl", "../src/shaders/slimeMoldSimulation/quadFragment.glsl");
  basicShaderProgram = ShaderUtils::makeShader("../src/shaders/basicVertex.glsl", "../src/shaders/basicFragment.glsl");

  glGenBuffers(1, &agentSSBO);
  glGenBuffers(1, &speciesSSBO);

  initTextures();
  updateSpeciesBuffer();
  resetAgents();

  float mapW = static_cast<float>(screenWidth);
  float mapH = static_cast<float>(screenHeight);
  float quadVertices[] = {
      0.0f, mapH, 0.0f, 1.0f,
      0.0f, 0.0f, 0.0f, 0.0f,
      mapW, 0.0f, 1.0f, 0.0f,

      0.0f, mapH, 0.0f, 1.0f,
      mapW, 0.0f, 1.0f, 0.0f,
      mapW, mapH, 1.0f, 1.0f};

  glGenVertexArrays(1, &quadVAO);
  glGenBuffers(1, &quadVBO);
  glBindVertexArray(quadVAO);
  glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));

  glGenVertexArrays(1, &brushVAO);
  glGenBuffers(1, &brushVBO);
  glBindVertexArray(brushVAO);
  glBindBuffer(GL_ARRAY_BUFFER, brushVBO);

  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, position));
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, color));
  glEnableVertexAttribArray(1);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

void SlimeMoldSimulation::update(float deltaTime)
{
  if (mouseScrollDeltaY != 0.0f)
  {
    float radiusChange = mouseScrollDeltaY * 5.0f;
    brushRadius = std::clamp(brushRadius + radiusChange, 5.0f, 100.0f);
    mouseScrollDeltaY = 0.0f;
  }

  if (enableDrawing && isLeftMouseDown)
  {
    int agentsToSpawn = static_cast<int>(spawnRate * deltaTime);

    spawnAgentsAt(worldMousePosition, agentsToSpawn, activeUISpecies);
  }

  // Pass 1: Update Agents
  glUseProgram(agentComputeShaderProgram);

  glUniform1f(glGetUniformLocation(agentComputeShaderProgram, "deltaTime"), deltaTime);
  glUniform2i(glGetUniformLocation(agentComputeShaderProgram, "resolution"), screenWidth, screenHeight);

  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, agentSSBO);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, speciesSSBO);

  glBindImageTexture(0, trailMapTexture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32F);

  glDispatchCompute((agentCount + 63) / 64, 1, 1);
  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

  // Pass 2: Diffuse and Evaporate
  glUseProgram(diffuseComputeShaderProgram);

  glUniform1f(glGetUniformLocation(diffuseComputeShaderProgram, "deltaTime"), deltaTime);
  glUniform1f(glGetUniformLocation(diffuseComputeShaderProgram, "evaporationRate"), evaporationRate);
  glUniform1f(glGetUniformLocation(diffuseComputeShaderProgram, "diffusionRate"), diffusionRate);

  glBindImageTexture(0, trailMapTexture, 0, GL_TRUE, 0, GL_READ_ONLY, GL_R32F);
  glBindImageTexture(1, diffusedTrailMapTexture, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32F);

  int currentSpeciesCount = std::max((int)speciesList.size(), 1);
  glDispatchCompute((screenWidth + 15) / 16, (screenHeight + 15) / 16, currentSpeciesCount);
  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

  std::swap(trailMapTexture, diffusedTrailMapTexture);
}

void SlimeMoldSimulation::render()
{
  glm::mat4 viewProjection = getViewProjectionMatrix();

  glUseProgram(renderShaderProgram);
  glUniformMatrix4fv(glGetUniformLocation(renderShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(viewProjection));
  glUniform1i(glGetUniformLocation(renderShaderProgram, "speciesCount"), speciesList.size());

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D_ARRAY, trailMapTexture);
  glUniform1i(glGetUniformLocation(renderShaderProgram, "trailMap"), 0);

  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, speciesSSBO);

  glBindVertexArray(quadVAO);
  glDrawArrays(GL_TRIANGLES, 0, 6);

  if (enableDrawing && !speciesList.empty())
  {
    std::vector<Vertex> brushVertices;
    int segments = 32;
    glm::vec3 brushColor = glm::vec3(speciesList[activeUISpecies].color);

    for (int i = 0; i < segments; i++)
    {
      float theta = 2.0f * glm::pi<float>() * static_cast<float>(i) / static_cast<float>(segments);
      glm::vec2 circlePosition = worldMousePosition + glm::vec2(brushRadius * cos(theta), brushRadius * sin(theta));

      brushVertices.push_back({circlePosition, brushColor});
    }

    glUseProgram(basicShaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(basicShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(viewProjection));

    glBindVertexArray(brushVAO);
    glBindBuffer(GL_ARRAY_BUFFER, brushVBO);
    glBufferData(GL_ARRAY_BUFFER, brushVertices.size() * sizeof(Vertex), brushVertices.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_LINE_LOOP, 0, segments);
  }

  glBindVertexArray(0);
}

void SlimeMoldSimulation::renderUI()
{
  ImGui::Text("Total Agents: %d", agentCount);

  if (ImGui::Button("Add New Species"))
  {
    if (speciesList.size() >= currentTextureCapacity)
    {
      int newCapacity = currentTextureCapacity * 2;

      expandTextureArray(trailMapTexture, newCapacity);
      expandTextureArray(diffusedTrailMapTexture, newCapacity);

      currentTextureCapacity = newCapacity;
    }

    SpeciesSettings newSpecies;
    newSpecies.color = glm::vec4((rand() % 100) / 100.0f, (rand() % 100) / 100.0f, (rand() % 100) / 100.0f, 1.0f);

    speciesList.push_back(newSpecies);

    updateSpeciesBuffer();
    activeUISpecies = speciesList.size() - 1;
  }

  if (speciesList.empty())
  {
    return;
  }

  ImGui::Separator();

  std::string previewText = "Species " + std::to_string(activeUISpecies);
  if (ImGui::BeginCombo("Select Species", previewText.c_str()))
  {
    for (int i = 0; i < speciesList.size(); i++)
    {
      bool isSelected = (activeUISpecies == i);
      if (ImGui::Selectable(("Species " + std::to_string(i)).c_str(), isSelected))
      {
        activeUISpecies = i;
      }

      if (isSelected)
      {
        ImGui::SetItemDefaultFocus();
      }
    }

    ImGui::EndCombo();
  }

  ImGui::Separator();

  SpeciesSettings &currentSpeciesSettings = speciesList[activeUISpecies];
  bool settingsChanged = false;

  if (ImGui::CollapsingHeader("Species Behavior", ImGuiTreeNodeFlags_DefaultOpen))
  {
    settingsChanged |= ImGui::ColorEdit4("Color", glm::value_ptr(currentSpeciesSettings.color));
    settingsChanged |= ImGui::SliderFloat("Move Speed", &currentSpeciesSettings.moveSpeed, 10.0f, 200.0f);
    settingsChanged |= ImGui::SliderFloat("Turn Speed", &currentSpeciesSettings.turnSpeed, 1.0f, 50.0f);
    settingsChanged |= ImGui::SliderFloat("Sensor Angle", &currentSpeciesSettings.sensorAngle, 0.0f, glm::pi<float>());
    settingsChanged |= ImGui::SliderFloat("Sensor Dist", &currentSpeciesSettings.sensorDistance, 1.0f, 50.0f);
    settingsChanged |= ImGui::SliderInt("Sensor Size", reinterpret_cast<int *>(&currentSpeciesSettings.sensorSize), 1, 5);
    settingsChanged |= ImGui::SliderFloat("Deposit Amount", &currentSpeciesSettings.depositAmount, 0.1f, 10.0f);
  }

  if (settingsChanged)
  {
    updateSpeciesBuffer();
  }

  if (ImGui::CollapsingHeader("Simulation Controls", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::Checkbox("Spawn Initial Agents on Reset", &spawnStartingAgents);

    if (ImGui::Button("Reset Simulation"))
    {
      initTextures();
      resetAgents();
    }

    ImGui::SameLine();

    if (ImGui::Button("Clear All Agents"))
    {
      agentCount = 0;
      agents.clear();

      agents.resize(1);
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, agentSSBO);
      glBufferData(GL_SHADER_STORAGE_BUFFER, agents.size() * sizeof(Agent), agents.data(), GL_DYNAMIC_DRAW);
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, agentSSBO);
    }

    if (ImGui::Button("Clear Trails Only"))
    {
      initTextures();
    }
  }

  if (ImGui::CollapsingHeader("Global Environment", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::SliderFloat("Evaporation Rate", &evaporationRate, 0.01f, 5.0f);
    ImGui::SliderFloat("Diffusion Rate", &diffusionRate, 0.0f, 1.0f);
  }

  if (ImGui::CollapsingHeader("Mouse Brush", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::Checkbox("Enable Drawing", &enableDrawing);
    ImGui::SliderInt("Spawn Rate", reinterpret_cast<int *>(&spawnRate), 1000, 50000);
  }
}