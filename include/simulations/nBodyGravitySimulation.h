#pragma once
#include "core/simulation2D.h"
#include <vector>
#include <glm/glm.hpp>

struct NBodyGravityParticle
{
  glm::vec2 position;
  glm::vec2 velocity;
  glm::vec4 color;
  glm::vec2 mass_radius; // x = mass, y = radius
  glm::vec2 padding;     // required for std430 alignment
};

enum class InteractionMode
{
  VIEW,
  PLACE_PLANET,
  SPAWN_GALAXY
};

inline const char *interactionModeStrings[] = {
    "View / Camera",
    "Place Planet",
    "Spawn Galaxy"};

enum class SimulationPreset
{
  TWO_BODY,
  GALAXY,
  GALAXY_COLLISION
};

inline const char *simulationPresetStrings[] = {
    "Two Body",
    "Single Galaxy",
    "Galaxy Collision"};

class NBodyGravitySimulation : public Simulation2D
{
private:
  const int MAX_PARTICLES = 100000;

  int activeParticles = 0;

  float gravity = 1.0f;
  float softening = 3.0f;
  float damping = 1.0f;

  float timeScale = 1.0f;

  bool trailsEnabled = true;
  float trailFadeAmount = 0.05f;

  InteractionMode currentMode = InteractionMode::VIEW;
  SimulationPreset currentPreset = SimulationPreset::TWO_BODY;

  float spawnPlanetMass = 100.0f;
  float spawnPlanetRadius = 5.0f;

  float spawnGalaxyRadius = 400.0f;
  int spawnGalaxyStarsCount = 1000;
  float spawnGalaxyCentralRadius = 2.5f;
  float spawnGalaxyCentralMass = 8000.0f;

  glm::vec3 spawnColor = {1.0f, 0.8f, 0.2f};

  unsigned int computeShaderProgram = 0;
  unsigned int renderShaderProgram = 0;
  unsigned int trailFadeShaderProgram = 0;
  unsigned int particlesInSSBO = 0;
  unsigned int particlesOutSSBO = 0;
  unsigned int particleVAO = 0;
  unsigned int trailFadeVAO = 0;
  unsigned int trailFadeVBO = 0;

  bool firstFrameCleared = false;

  glm::vec2 getOrbitalVelocity(glm::vec2 position, glm::vec2 centerPosition, float centerMass);
  void addParticle(glm::vec2 position, glm::vec2 velocity, float mass, float radius, glm::vec4 color);
  void addParticle(std::vector<NBodyGravityParticle> &particles);

  void loadPreset(SimulationPreset preset);

  void createGalaxy(glm::vec2 centerPosition, int starsCount, float galaxyRadius, float centralRadius, float centralMass, glm::vec4 baseColor);

public:
  NBodyGravitySimulation(int screenWidth, int screenHeight);
  ~NBodyGravitySimulation();

  void init() override;

  void update(float deltaTime) override;

  void render() override;

  void renderUI() override;

  bool handlesOwnClear() override;
};