#include <config.h>

#include "core/simulation2D.h"

struct Particle
{
  glm::vec2 currentPosition;
  glm::vec2 previousPosition;
  bool isPinned;
  bool wasPinned;

  Particle(float x, float y);

  // https://en.wikipedia.org/wiki/Verlet_integration
  void update(float deltaTime, float drag, const glm::vec2 &gravity);
};

struct Constraint
{
  unsigned int p0Index;
  unsigned int p1Index;
  float length;
  bool isActive;

  Constraint(unsigned int p0Index, unsigned int p1Index, float length);

  void update(std::vector<Particle> &particles, float elasticity);
};

class ClothSimulation : public Simulation2D
{
private:
  std::vector<Particle> particles;
  std::vector<Constraint> constraints;

  int width, height;
  int startX, startY, spacing;

  unsigned int VAO = 0, VBO = 0, shaderProgram = 0;

  Particle *draggedPoint = nullptr;
  float mouseTearRadius = 15.0f;

  float gravityY = 9.81f;
  float windForceX = 0.0f;
  float drag = 0.01f;
  float elasticity = 5.0f;
  int constraintIterations = 5;

  void rebuildCloth(int width, int height, float spacing);

public:
  ClothSimulation(int width, int height, float spacing, float startX, float startY, int screenWidth, int screenHeight);
  ~ClothSimulation();

  void init() override;

  void update(float deltaTime) override;

  void render() override;

  void renderUI() override;
};