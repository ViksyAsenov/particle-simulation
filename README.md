# Particle Simulation

A small OpenGL particle and cellular simulation project featuring four interactive systems: cloth, falling sand, slime mold, and n-body gravity. The app is built with CMake and uses GLFW + OpenGL for real-time rendering.

## Included Simulations

### 1. Cloth Simulation

A verlet-based cloth simulation with structural constraints, wind, gravity, drag, and interactive tearing/dragging. You can pull points and tear the cloth by holding the right mouse button.

<video src="videos/cloth.mp4" controls muted playsinline width="100%"></video>

### 2. Falling Sand

A grid-based particle sandbox with sand, water, stone, wood, and fire. The simulation supports brush painting, grid resizing, and particle interactions.

<video src="videos/fallingSand.mp4" controls muted playsinline width="100%"></video>

### 3. Slime Mold

A compute-shader based Physarum-inspired simulation where agents move through an evolving trail map, deposit matter, and react to neighboring species behaviors. This simulation includes multiple species with distinct movement and sensing rules.

<video src="videos/slimeMold.mp4" controls muted playsinline width="100%"></video>

### 4. N-Body Gravity

A GPU-accelerated gravity simulation with orbital motion and galaxy generation. The app includes presets for two-body systems, single galaxies, and galaxy collisions, along with interactive planet placement and trail effects.

<video src="videos/nBodyGravity.mp4" controls muted playsinline width="100%"></video>

## Features

- OpenGL rendering with shader-based visual effects
- Interactive mouse controls for drawing, tearing, dragging, and placement tools
- ImGui-based settings panels for tuning simulation parameters
- Camera zoom and pan behavior across 2D simulation scenes
- CMake build setup for local compilation

## Build and Run

### Requirements

- CMake 3.20+
- OpenGL
- C++ compiler
- GLFW and GLM are bundled in the project under `libs/`

### Build

```bash
mkdir build
cd build
cmake ..
make
```

### Run

```bash
./particle-simulation
```

## Controls and Usage

The app is organized as a tab-based simulation browser. Use the tabs in the left-side controls panel to switch between the available simulations, then adjust the parameters in each simulation's ImGui section.

- Left mouse button: interact with the active simulation
- Right mouse button: erase or tear depending on the simulation
- Middle mouse button: drag the camera to pan around the scene
- Mouse scroll: simulation-specific actions such as changing brush size or other per-simulation values
- Ctrl + mouse scroll: zoom the camera in or out
- C: reset the camera to the default focus and zoom
