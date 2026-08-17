#include "simulations/nBodyGravitySimulation.h"
#include "core/shaderUtils.h"
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdlib>
#include "imgui/imgui.h"
#include <random>

NBodyGravitySimulation::NBodyGravitySimulation(int screenWidth, int screenHeight)
    : Simulation2D(screenWidth, screenHeight)
{
  setCameraFocus(glm::vec2(0.0f, 0.0f), 1.0f);
}

NBodyGravitySimulation::~NBodyGravitySimulation()
{
  glDeleteVertexArrays(1, &particleVAO);
  glDeleteVertexArrays(1, &trailFadeVAO);
  glDeleteBuffers(1, &trailFadeVBO);
  glDeleteBuffers(1, &particlesInSSBO);
  glDeleteBuffers(1, &particlesOutSSBO);
  glDeleteProgram(computeShaderProgram);
  glDeleteProgram(renderShaderProgram);
  glDeleteProgram(trailFadeShaderProgram);
}

void NBodyGravitySimulation::init()
{
  computeShaderProgram = ShaderUtils::makeComputeShader("../src/shaders/nBodyGravitySimulation/nBodyGravityCompute.comp");
  renderShaderProgram = ShaderUtils::makeShader("../src/shaders/nBodyGravitySimulation/nBodyGravityVertex.glsl", "../src/shaders/nBodyGravitySimulation/nBodyGravityFragment.glsl");

  glGenVertexArrays(1, &particleVAO);
  glGenBuffers(1, &particlesInSSBO);
  glGenBuffers(1, &particlesOutSSBO);

  glBindBuffer(GL_ARRAY_BUFFER, particlesInSSBO);
  glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * sizeof(NBodyGravityParticle), nullptr, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particlesInSSBO);

  glBindBuffer(GL_ARRAY_BUFFER, particlesOutSSBO);
  glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * sizeof(NBodyGravityParticle), nullptr, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, particlesOutSSBO);

  glBindVertexArray(particleVAO);

  // 1. Position
  glEnableVertexAttribArray(0);
  glVertexAttribFormat(0, 2, GL_FLOAT, GL_FALSE, offsetof(NBodyGravityParticle, position));
  glVertexAttribBinding(0, 0);

  // 2. Color
  glEnableVertexAttribArray(1);
  glVertexAttribFormat(1, 4, GL_FLOAT, GL_FALSE, offsetof(NBodyGravityParticle, color));
  glVertexAttribBinding(1, 0);

  // 3. Mass & Radius
  glEnableVertexAttribArray(2);
  glVertexAttribFormat(2, 2, GL_FLOAT, GL_FALSE, offsetof(NBodyGravityParticle, mass_radius));
  glVertexAttribBinding(2, 0);

  glBindVertexArray(0);

  trailFadeShaderProgram = ShaderUtils::makeShader(
      "../src/shaders/nBodyGravitySimulation/trailFadeVertex.glsl",
      "../src/shaders/nBodyGravitySimulation/trailFadeFragment.glsl");

  float quadVerts[] = {
      -1.0f,
      -1.0f,
      1.0f,
      -1.0f,
      1.0f,
      1.0f,
      -1.0f,
      -1.0f,
      1.0f,
      1.0f,
      -1.0f,
      1.0f,
  };

  glGenVertexArrays(1, &trailFadeVAO);
  glGenBuffers(1, &trailFadeVBO);

  glBindVertexArray(trailFadeVAO);
  glBindBuffer(GL_ARRAY_BUFFER, trailFadeVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
  glBindVertexArray(0);

  loadPreset(currentPreset);
}

glm::vec2 NBodyGravitySimulation::getOrbitalVelocity(glm::vec2 position, glm::vec2 centerPosition, float centerMass)
{
  glm::vec2 positionDifference = position - centerPosition;

  float r = glm::length(positionDifference);

  if (r <= 0.0001f)
  {
    return glm::vec2(0.0f);
  }

  // v = sqrt(G * M / r)
  float speed = std::sqrt(gravity * centerMass / r);

  glm::vec2 direction = positionDifference / r;

  glm::vec2 tangent(-direction.y, direction.x);

  return tangent * speed;
}

void NBodyGravitySimulation::addParticle(glm::vec2 pos, glm::vec2 vel, float mass, float radius, glm::vec4 color)
{
  if ((activeParticles + 1) >= MAX_PARTICLES)
  {
    return;
  }

  NBodyGravityParticle p;
  p.position = pos;
  p.velocity = vel;
  p.color = color;
  p.mass_radius = glm::vec2(mass, radius);
  p.padding = glm::vec2(0.0f);

  glBindBuffer(GL_ARRAY_BUFFER, particlesInSSBO);
  glBufferSubData(GL_ARRAY_BUFFER, activeParticles * sizeof(NBodyGravityParticle), sizeof(NBodyGravityParticle), &p);

  glBindBuffer(GL_ARRAY_BUFFER, particlesOutSSBO);
  glBufferSubData(GL_ARRAY_BUFFER, activeParticles * sizeof(NBodyGravityParticle), sizeof(NBodyGravityParticle), &p);

  activeParticles++;
}

void NBodyGravitySimulation::addParticle(std::vector<NBodyGravityParticle> &particles)
{
  if ((activeParticles + particles.size()) >= MAX_PARTICLES)
  {
    return;
  }

  glBindBuffer(GL_ARRAY_BUFFER, particlesInSSBO);
  glBufferSubData(GL_ARRAY_BUFFER, activeParticles * sizeof(NBodyGravityParticle), particles.size() * sizeof(NBodyGravityParticle), particles.data());

  glBindBuffer(GL_ARRAY_BUFFER, particlesOutSSBO);
  glBufferSubData(GL_ARRAY_BUFFER, activeParticles * sizeof(NBodyGravityParticle), particles.size() * sizeof(NBodyGravityParticle), particles.data());

  activeParticles += particles.size();
}

void NBodyGravitySimulation::createGalaxy(
    glm::vec2 centerPosition, int starsCount, float galaxyRadius, float centralRadius, float centralMass, glm::vec4 baseColor)
{
  if (activeParticles + starsCount + 1 >= MAX_PARTICLES)
  {
    return;
  }

  glm::vec4 coreColor = (baseColor * 0.2f) + glm::vec4(0.8f, 0.8f, 0.8f, 0.0f);
  coreColor.a = baseColor.a;

  glm::vec4 innerStarColor = baseColor;

  glm::vec4 outerStarColor = glm::vec4(baseColor.r * 0.3f, baseColor.g * 0.3f, baseColor.b * 0.3f, baseColor.a);

  std::vector<NBodyGravityParticle> galaxyParticles;
  galaxyParticles.reserve(starsCount + 1);

  NBodyGravityParticle centralStar;
  centralStar.position = centerPosition;
  centralStar.velocity = glm::vec2(0.0f, 0.0f);
  centralStar.mass_radius = glm::vec2(centralMass, centralRadius);
  centralStar.color = coreColor;
  centralStar.padding = glm::vec2(0.0f);

  galaxyParticles.push_back(centralStar);

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<float> randomAngle(0.0f, 2.0f * glm::pi<float>());
  std::normal_distribution<float> randomRadius(0.0f, galaxyRadius / 2.0f);
  std::uniform_real_distribution<float> variance(0.95f, 1.05f);

  for (int i = 0; i < starsCount; i++)
  {
    float r = std::abs(randomRadius(gen));

    if (r < 2.0f)
    {
      r = 2.0f;
    }

    float theta = randomAngle(gen);
    glm::vec2 pos = centerPosition + glm::vec2(std::cos(theta), std::sin(theta)) * r;

    NBodyGravityParticle star;
    star.position = pos;

    star.velocity = getOrbitalVelocity(pos, centerPosition, centralMass) * variance(gen);

    star.mass_radius = glm::vec2(1.0f, 1.0f);

    float normalizedDist = r / galaxyRadius;
    star.color = (innerStarColor * (1.0f - normalizedDist)) + (outerStarColor * normalizedDist);
    star.padding = glm::vec2(0.0f);

    galaxyParticles.push_back(star);
  }

  addParticle(galaxyParticles);
}

void NBodyGravitySimulation::loadPreset(SimulationPreset preset)
{
  activeParticles = 0;
  firstFrameCleared = false;

  switch (preset)
  {
  case SimulationPreset::TWO_BODY:
  {
    gravity = 100.0f;
    softening = 3.0f;
    damping = 1.0f;

    float mass = 10.0f;
    float separation = 100.0f;

    glm::vec2 posA = glm::vec2(-separation * 0.5f, 0.0f);
    glm::vec2 posB = glm::vec2(separation * 0.5f, 0.0f);

    float eccentricityFactor = 0.7f;

    // compute orbital velocity using the reduced problem

    float totalMass = mass + mass;
    float r = separation;
    float velocityCircularRelative = std::sqrt(gravity * totalMass / r);
    float velocityRelative = velocityCircularRelative * eccentricityFactor;

    glm::vec2 tangent(0.0f, 1.0f); // perpendicular to separation (along x-axis here)
    glm::vec2 velA = -tangent * velocityRelative * (mass / totalMass);
    glm::vec2 velB = tangent * velocityRelative * (mass / totalMass);

    addParticle(posA, velA, mass, 5.0f, glm::vec4(1, 1, 0, 1));
    addParticle(posB, velB, mass, 5.0f, glm::vec4(0, 0.5f, 1, 1));

    break;
  }

  case SimulationPreset::GALAXY:
  {
    gravity = 1.0f;
    softening = 1.0f;
    damping = 1.0f;

    createGalaxy(
        glm::vec2(0.0f, 0.0f), 40000, 400.0f, 5.0f, 80000.0f,
        glm::vec4(1.0f, 0.3f, 0.0f, 1.0f));

    break;
  }

  case SimulationPreset::GALAXY_COLLISION:
    gravity = 2.0f;
    softening = 30.0f;
    damping = 1.0f;

    createGalaxy(
        glm::vec2(-300.0f, -100.0f),
        25000,
        400.0f,
        5.0f,
        80000.0f,
        glm::vec4(1.0f, 0.3f, 0.0f, 1.0f));

    createGalaxy(
        glm::vec2(300.0f, 100.0f),
        15000,
        250.0f,
        5.0f,
        40000.0f,
        glm::vec4(0.0f, 0.5f, 1.0f, 1.0f));

    break;
  }
}

void NBodyGravitySimulation::update(float deltaTime)
{
  if (activeParticles == 0)
  {
    return;
  }

  float scaledDeltaTime = (deltaTime * timeScale);

  glUseProgram(computeShaderProgram);

  glUniform1f(glGetUniformLocation(computeShaderProgram, "deltaTime"), scaledDeltaTime);
  glUniform1f(glGetUniformLocation(computeShaderProgram, "gravity"), gravity);
  glUniform1f(glGetUniformLocation(computeShaderProgram, "softening"), softening);
  glUniform1ui(glGetUniformLocation(computeShaderProgram, "activeParticles"), activeParticles);
  glUniform1f(glGetUniformLocation(computeShaderProgram, "damping"), damping);

  int workGroups = (activeParticles + 255) / 256;

  glDispatchCompute(workGroups, 1, 1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  std::swap(particlesInSSBO, particlesOutSSBO);

  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particlesInSSBO);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, particlesOutSSBO);
}

void NBodyGravitySimulation::render()
{
  if (activeParticles == 0)
  {
    return;
  }

  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);

  // A bit hacky solution for having trails in the main buffer without clearing the color buffer every frame. The first frame is cleared normally,
  // then subsequent frames only clear the alpha channel to fade out trails. On the other hand it looks very nice
  if (!firstFrameCleared)
  {
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearColor(0.0f, 0.0f, 0.0f, 0.75f);
    glClear(GL_COLOR_BUFFER_BIT);
    firstFrameCleared = true;
  }

  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
  glClearColor(0.0f, 0.0f, 0.0f, 0.75f);
  glClear(GL_COLOR_BUFFER_BIT);

  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);

  if (trailsEnabled)
  {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(trailFadeShaderProgram);

    glUniform1f(glGetUniformLocation(trailFadeShaderProgram, "fadeAmount"), trailFadeAmount);

    glBindVertexArray(trailFadeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
  }

  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glDepthMask(GL_FALSE);

  glUseProgram(renderShaderProgram);

  glm::mat4 viewProjection = getViewProjectionMatrix();
  glUniformMatrix4fv(glGetUniformLocation(renderShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(viewProjection));

  glEnable(GL_PROGRAM_POINT_SIZE);

  glBindVertexArray(particleVAO);

  glBindVertexBuffer(0, particlesInSSBO, 0, sizeof(NBodyGravityParticle));

  glDrawArrays(GL_POINTS, 0, activeParticles);

  glBindVertexArray(0);

  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_BLEND);
  glDepthMask(GL_TRUE);
  glEnable(GL_DEPTH_TEST);

  if (isLeftMouseDown)
  {
    switch (currentMode)
    {
    case InteractionMode::PLACE_PLANET:
      addParticle(worldMousePosition, glm::vec2(0.0f), spawnPlanetMass, spawnPlanetRadius, glm::vec4(spawnColor, 1.0f));
      break;
    case InteractionMode::SPAWN_GALAXY:
      createGalaxy(worldMousePosition, spawnGalaxyStarsCount, spawnGalaxyRadius, spawnGalaxyCentralRadius, spawnGalaxyCentralMass, glm::vec4(spawnColor, 1.0f));
      break;
    }

    isLeftMouseDown = false;
  }
}

void NBodyGravitySimulation::renderUI()
{
  ImGui::Text("Active Particles: %d", activeParticles);

  ImGui::Spacing();
  ImGui::Text("Environment");
  ImGui::SliderFloat("Gravity", &gravity, 1.0f, 1000.0f);
  ImGui::SliderFloat("Softening", &softening, 0.1f, 50.0f);
  ImGui::SliderFloat("Damping", &damping, 0.01f, 5.0f);

  ImGui::Separator();
  ImGui::Text("Presets");

  int currentPresetIndex = static_cast<int>(currentPreset);
  if (ImGui::Combo("Load", &currentPresetIndex, simulationPresetStrings, IM_ARRAYSIZE(simulationPresetStrings)))
  {
    currentPreset = static_cast<SimulationPreset>(currentPresetIndex);
    loadPreset(currentPreset);
  }

  ImGui::Separator();
  ImGui::Text("Tools");

  int currentModeIndex = static_cast<int>(currentMode);
  for (int i = 0; i < IM_ARRAYSIZE(interactionModeStrings); ++i)
  {
    if (i > 0)
    {
      ImGui::SameLine();
    }

    if (ImGui::RadioButton(interactionModeStrings[i], &currentModeIndex, i))
    {
      currentMode = static_cast<InteractionMode>(currentModeIndex);
    }
  }

  if (currentMode != InteractionMode::VIEW)
  {
    ImGui::Spacing();

    switch (currentMode)
    {
    case InteractionMode::PLACE_PLANET:
    {
      ImGui::SliderFloat("Planet Mass", &spawnPlanetMass, 1.0f, 2000.0f);
      ImGui::SliderFloat("Planet Radius", &spawnPlanetRadius, 1.0f, 50.0f);

      break;
    }
    case InteractionMode::SPAWN_GALAXY:
    {
      ImGui::SliderFloat("Radius", &spawnGalaxyRadius, 1.0f, 10000.0f);
      ImGui::SliderInt("Stars Count", &spawnGalaxyStarsCount, 100, 75000);
      ImGui::SliderFloat("Central Mass", &spawnGalaxyCentralMass, 1000.0f, 20000.0f);
      ImGui::SliderFloat("Central Radius", &spawnGalaxyCentralRadius, 1.0f, 100.0f);

      break;
    }
    }

    ImGui::ColorEdit3("Spawn Color", reinterpret_cast<float *>(&spawnColor));
  }

  ImGui::Separator();
  ImGui::Text("Trails");

  ImGui::Checkbox("Enabled", &trailsEnabled);
  ImGui::SliderFloat("Fade Amount", &trailFadeAmount, 0.001f, 1.0f, "%.3f");

  if (ImGui::Button("Clear Trails"))
  {
    firstFrameCleared = false;
  }

  ImGui::Separator();
  ImGui::Text("Time");

  ImGui::SliderFloat("Time Scale", &timeScale, 0.0f, 10.0f, "%.2fx");

  if (ImGui::Button("Pause"))
  {
    timeScale = 0.0f;
  }

  ImGui::SameLine();
  if (ImGui::Button("1x"))
  {
    timeScale = 1.0f;
  }
}

bool NBodyGravitySimulation::handlesOwnClear()
{
  return trailsEnabled;
}