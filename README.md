# Hover Racing Game — TL-Engine (C++)

A 3D hover bike racing game built in C++ using the TL-Engine.
The player races through a track, avoiding obstacles and passing
checkpoints to complete the race.

## Features
- 3D environment with skybox, ground, walls, isles, and tank obstacles
- Realistic hover bike movement with acceleration and drag physics
- Mouse-controlled camera with pitch clamping (reset with key 1)
- Three collision detection methods:
  - Sphere-to-Box: walls and isles
  - Sphere-to-Sphere: tank obstacles
  - Point-to-Box: checkpoint detection
- Countdown timer (3, 2, 1, Go!) on game start
- Live speed display in km/h
- 3-stage checkpoint system with race completion message

## Controls
| Key | Action |
|-----|--------|
| W | Accelerate forward |
| S | Brake / Reverse |
| A | Turn left |
| D | Turn right |
| Mouse | Look around |
| 1 | Reset camera |
| Arrow Keys | Manual camera movement |
| Space | Start race |

## Requirements
- Visual Studio 2019 or later
- TL-Engine library (must be installed separately)
- Windows OS

## How to Run
1. Install the TL-Engine on your machine
2. Open `Hover_RaacingGame.sln` in Visual Studio
3. Build the solution (Ctrl+Shift+B)
4. Run with Ctrl+F5
5. Press Space to start the countdown and begin the race

## Project Structure
| File/Folder | Description |
|-------------|-------------|
| `Hover_RaacingGame.cpp` | Main game source code |
| `Media/` | All 3D models, textures and skybox assets |
| `*.vsh / *.psh` | Vertex and pixel shader files |
| `Hover_RaacingGame.sln` | Visual Studio solution file |
