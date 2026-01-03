#pragma once

#include "raylib.h"
#include <string>
#include <vector>


// Forward declarations
class Faction;

enum class SettlementBuildingType {
  NONE = 0,
  HOUSE,
  FARM,
  LUMBERMILL,
  MINE,
  BARRACKS,
  TOWER
};

/**
 * @brief Represents a single settlement owned by a Faction
 *
 * In passive mode, this is just a simulation container.
 * In active mode (Phase 2), this will spawn actual buildings in the world.
 */
class Settlement {
private:
  std::string name;
  Vector3 location;
  Faction *owner; // Pointer to owning faction

  // Population
  int population;
  float growthTimer = 0.0f;

  // Infrastructure
  struct BuildingEntry {
    SettlementBuildingType type;
    int count;
  };
  std::vector<BuildingEntry> buildings;

  // Logic
  float buildTimer = 0.0f;
  static constexpr float BUILD_INTERVAL = 30.0f; // Try to build every 30s

public:
  Settlement(std::string name, Vector3 location, Faction *owner);

  void Update(float deltaTime);

  // Simulation
  void SimulateGrowth();
  void ConstructBuilding(SettlementBuildingType type);

  // Getters
  std::string GetName() const { return name; }
  Vector3 GetLocation() const { return location; }
  int GetPopulation() const { return population; }
  int GetBuildingCount(SettlementBuildingType type) const;
  std::string GetBuildingName(SettlementBuildingType type) const;
};
