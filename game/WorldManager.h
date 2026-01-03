#pragma once

#include "raylib.h"
#include <map>
#include <memory>
#include <vector>


// Forward declarations
class Region;
class Faction;

// Grid coordinate for regions
struct GridCoord {
  int x;
  int z;

  bool operator==(const GridCoord &other) const {
    return x == other.x && z == other.z;
  }

  bool operator<(const GridCoord &other) const {
    if (x != other.x)
      return x < other.x;
    return z < other.z;
  }
};

/**
 * WorldManager - Singleton managing the entire game world state
 *
 * Responsibilities:
 * - Divide world into regions (spatial partitioning)
 * - Activate/deactivate regions based on player proximity
 * - Manage global time and world state
 * - Coordinate AI factions
 *
 * Phase 0: Basic 3x3 static grid, simple activation
 * Future: Dynamic loading, infinite procedural world
 */
class WorldManager {
private:
  static WorldManager *instance;

  // Region management
  std::map<GridCoord, std::unique_ptr<Region>> regions;
  std::vector<Region *> activeRegions;

  // Player tracking
  Vector3 lastPlayerPosition;
  GridCoord currentPlayerGrid;

  // Factions
  std::vector<std::unique_ptr<Faction>> factions;

  // World settings
  static constexpr float REGION_SIZE = 100.0f; // 100x100 meters per region
  static constexpr int ACTIVATION_RADIUS =
      1; // Activate regions within 1 grid cell

  // Private constructor (Singleton)
  WorldManager();

public:
  // Singleton access
  static WorldManager *GetInstance();
  static void Destroy();

  // Lifecycle
  void Initialize();
  void Update(float deltaTime, Vector3 playerPosition);
  void Render();
  void Shutdown();

  // Region access
  Region *GetRegionAt(Vector3 worldPos);
  Region *GetRegionByGrid(GridCoord coord);
  std::vector<Region *> &GetActiveRegions() { return activeRegions; }

  // Coordinate conversion
  GridCoord WorldPosToGrid(Vector3 pos);
  Vector3 GridToWorldPos(GridCoord coord);

  // Faction management (Phase 0: stubs)
  void RegisterFaction(std::unique_ptr<Faction> faction);

  // Debug
  void DrawDebugInfo();

private:
  void UpdateRegionActivation(Vector3 playerPos);
  void EnsureRegionExists(GridCoord coord);
};
