# Player Character Walkthrough

## Overview
Phase 3 "Player Character Deep Dive" has begun with the implementation of a dedicated `Player` class. This replaces the simple camera controller with a physics-based entity capable of advanced movement.

## Features Implemented
### 1. Dedicated Player Controller (`game/Player.cpp`)
- **Physics**: Gravity, Ground Detection (Terrain Raycasting), Momentum.
- **States**: 
  - `WALKING` (Normal speed)
  - `SPRINTING` (Shift key, Faster speed + Heavy Head Bob)
  - `CROUCHING` (Ctrl key, Slower speed, Lower camera + Smooth transition)
  - `AIRBORNE` (Space key, Gravity applies)
  
### 2. Camera Effects & Immersion
- **Head Bob**: Procedural camera motion simulating footsteps. Frequency and amplitude change based on speed (Sprint vs Walk vs Crouch).
- **Weapon Sway**: Procedural weapon lag based on mouse movement input. Calculates offset `(mouseDelta * swayAmount)` and smooths it with `Lerp` for a weighty feel.
- **Crouch Smoothing**: Camera smoothly lerps between standing and crouching height.

### 3. Interaction System
- **Raycasting**: Forward ray from camera checks collisions with `InteractableObject` derived entities (e.g., `ResourceNode`).
- **UI Prompt**: Displays "Press E to Interact" and Object Name when focusing on an interactable object within range (3.0m).
- **Integration**: `main.cpp` feeds active region interactables to `Player::CheckInteractions()`.

### 4. Architecture
- `Player` inherits from `GameEntity`.
- `g_player` (Global Instance) instantiated in `main.cpp` when switching to FPS mode.
- Clean separation of Input/Physics/Rendering.

## Verification
- **Build**: Successful.
- **Controls**:
  - `WASD`: Move
  - `SHIFT`: Sprint
  - `CTRL`: Crouch
  - `SPACE`: Jump
  - `MOUSE`: Look

### 5. Debugging & Polish (Final Fixes)
- **Input Pipeline**: Fixed issue where `g_player` was not initialized or updated, causing the game to fall back to legacy settler controls (WASD only). Added explicit `g_player->update(deltaTime)` in `main.cpp` loop.
- **Controls**: Fixed inverted strafing (A/D keys were swapped).
- **Visuals**: Adjusted `m_weaponBasePos` to bring the hand model closer to the center and camera, solving visibility issues.

## Final Status
- **FPS Mode**: Fully Functional. 
- **Controls**: Responsive (Movement + Look + Actions).
- **Visuals**: Correctly aligned.

## Next Steps
- Begin Phase 3: Leadership & Management (Recruitment, Authority, Delegation).
