# WorldManager Expansion: Dynamic LOD System

## Overview
Phase 2 initialization focused on expanding the `WorldManager` to support a virtually infinite world using a Level of Detail (LOD) system. This ensures that only regions near the player are fully simulated, while distant regions enter low-cost states.

## Key Changes

### 1. 3-Tier Simulation State
We introduced a new `BACKGROUND` state to the `RegionState` enum, creating a 3-tier system:

- **ACTIVE**: Full simulation (Settlers, Pathfinding, Physics).
- **PASSIVE**: Abstract simulation (Simple resource math, "Dark Fantasy" decay).
- **BACKGROUND**: Minimal state (Time accumulation only, 4x slower updates).

**Logic in `Region.h`:**
```cpp
enum class RegionState {
  UNINITIALIZED,
  BACKGROUND,    // New: Distant regions
  PASSIVE,       // Nearby but not active
  ACTIVE         // Player presence
};
```

### 2. State Transition Logic (`SetState`)
Refactored `Region.cpp` to use a unified `SetState` method that handles all initialization and cleanup during transitions.

- **Active -> Passive**: Serializes colony data to `PassiveState`, destroys heavy objects (Terrain, Settlers).
- **Passive -> Active**: Rebuilds the world from `PassiveState`, spawns terrain and settlers.

### 3. Dynamic Distance-Based Loading
Updated `WorldManager::UpdateRegionActivation` to use distance thresholds from the player:
- **< 150m**: Force `ACTIVE`.
- **< 500m**: Ensure `PASSIVE`.
- **> 500m**: Demote to `BACKGROUND`.

 This replaces the static 3x3 grid logic with a dynamic radius checks, paving the way for infinite terrain generation.

### 4. Event Bus Integration (Storage)
Refactored `StorageSystem` to properly publish `ItemAddedToStorageEvent` and `ItemRemovedFromStorageEvent` using the central `EventBus`. This decouples resource logic from the UI and Colony.

## Verification
- **Build Status**: Successful.
- **LOD Logic**: Regions correctly switch states based on player movement.
- **Performance**: Distant regions now consume negligible CPU time.
