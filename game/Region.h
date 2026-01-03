#pragma once

#include "WorldManager.h"
#include "raylib.h"
#include <memory>
#include <vector>

// Forward declarations
class Colony;
class Terrain;
class Tree;
class ResourceNode;
class BuildingInstance;

/**
 * RegionState - Current simulation mode of the region
 */
enum class RegionState {
  UNINITIALIZED, // Just created, no data
  BACKGROUND,    // Distant simulation (very low frequency)
  PASSIVE,       // Background simulation (abstract)
  ACTIVE         // Full simulation (all game objects)
};

/**
 * Region - A chunk of the game world (100x100m by default)
 *
 * Can be in two states:
 * - ACTIVE: Full simulation with Colony, Settlers, buildings, etc.
 * - PASSIVE: Abstract simulation (population count, resource production)
 *
 * Phase 0: Basic active/passive switching
 * Future: Proper state serialization, seamless transition
 */
class Region {
private:
  GridCoord gridCoord;
  Vector3 worldCenter;
  RegionState state;

  // Active simulation data
  std::unique_ptr<Colony> colony;
  std::unique_ptr<Terrain> terrain;

  // References to world objects (don't own them, just track)
  // References to world objects (don't own them, just track)
  std::vector<Tree *> treesInRegion;
  std::vector<ResourceNode *> resourceNodesInRegion;
  std::vector<BuildingInstance *> buildingsInRegion;

  // Passive simulation data (abstract state)
  struct PassiveState {
    float abstractPopulation = 0.0f;
    float abstractFood = 100.0f;
    float abstractWood = 50.0f;
    float abstractStone = 30.0f;
    float growthTimer = 0.0f;
  } passiveState;

  // Timing
  float timeSinceLastPassiveTick = 0.0f;
  static constexpr float PASSIVE_TICK_INTERVAL = 5.0f; // Tick every 5 seconds

public:
  Region(GridCoord coord, Vector3 center);
  ~Region();

  // Lifecycle
  void ActivateFullSimulation();
  void SetState(RegionState newState);
  void DeactivateToPassive();
  void Update(float deltaTime);
  void PassiveTick(float deltaTime);
  void BackgroundTick(float deltaTime);
  void Render();

  // State queries
  bool IsActive() const { return state == RegionState::ACTIVE; }
  RegionState GetState() const { return state; }
  GridCoord GetGridCoord() const { return gridCoord; }
  Vector3 GetCenter() const { return worldCenter; }

  // Access to game objects (only valid when ACTIVE)
  Colony *GetColony() { return colony.get(); }
  Terrain *GetTerrain() { return terrain.get(); }
  const std::vector<ResourceNode *> &GetResourceNodes() const {
    return resourceNodesInRegion;
  }

  // Debug
  void DrawDebugBounds();

private:
  void InitializeActiveState();
  void SyncToPassiveState();
  void SyncFromPassiveState();
};
