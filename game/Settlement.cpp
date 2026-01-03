#include "Settlement.h"
#include "Faction.h"
#include <algorithm>
#include <iostream>


Settlement::Settlement(std::string name, Vector3 location, Faction *owner)
    : name(name), location(location), owner(owner), population(5) {

  // Initial buildings
  buildings.push_back({SettlementBuildingType::HOUSE, 1});
  buildings.push_back({SettlementBuildingType::FARM, 1});

  std::cout << "[Settlement] Founded " << name << " at " << location.x << ","
            << location.z << std::endl;
}

void Settlement::Update(float deltaTime) {
  // 1. Population Growth (Passive)
  growthTimer += deltaTime;
  if (growthTimer >= 10.0f) { // Every 10s
    growthTimer = 0.0f;
    SimulateGrowth();
  }

  // 2. Construction Cycle
  buildTimer += deltaTime;
  if (buildTimer >= BUILD_INTERVAL) {
    buildTimer = 0.0f;

    // Simple logic: Cycle through buildings
    // Cycle: HOUSE -> LUMBERMILL -> MINE -> TOWER
    if (GetBuildingCount(SettlementBuildingType::HOUSE) < population / 2) {
      ConstructBuilding(SettlementBuildingType::HOUSE);
    } else if (GetBuildingCount(SettlementBuildingType::LUMBERMILL) < 1) {
      ConstructBuilding(SettlementBuildingType::LUMBERMILL);
    } else if (GetBuildingCount(SettlementBuildingType::MINE) < 1) {
      ConstructBuilding(SettlementBuildingType::MINE);
    } else {
      ConstructBuilding(SettlementBuildingType::TOWER);
    }
  }
}

void Settlement::SimulateGrowth() {
  // Need food to grow (Abstract check from Faction Treasury)
  if (owner->GetTreasury().food > population * 2.0f) {
    population++;
    owner->GetTreasury().food -= population * 2.0f; // Consume food
    // std::cout << "[Settlement " << name << "] Grew to " << population << "
    // pop" << std::endl;
  } else {
    // Starvation logic or stagnation
  }

  // Production (Abstract)
  float laborForce = (float)population;

  // Farms produce food
  float farmProd = (float)GetBuildingCount(SettlementBuildingType::FARM) * 5.0f;
  owner->GetTreasury().food += farmProd;

  // Lumbermills produce wood
  float woodProd =
      (float)GetBuildingCount(SettlementBuildingType::LUMBERMILL) * 2.0f;
  owner->GetTreasury().wood += woodProd;
}

void Settlement::ConstructBuilding(SettlementBuildingType type) {
  // Cost check (Hardcoded for prototype)
  float woodCost = 50.0f;
  float stoneCost = 20.0f;

  if (owner->GetTreasury().wood >= woodCost &&
      owner->GetTreasury().stone >= stoneCost) {
    // Pay cost
    owner->GetTreasury().wood -= woodCost;
    owner->GetTreasury().stone -= stoneCost;

    // Add building
    bool found = false;
    for (auto &b : buildings) {
      if (b.type == type) {
        b.count++;
        found = true;
        break;
      }
    }
    if (!found) {
      buildings.push_back({type, 1});
    }

    std::cout << "[Settlement " << name << "] Constructed "
              << GetBuildingName(type) << std::endl;
  }
}

int Settlement::GetBuildingCount(SettlementBuildingType type) const {
  for (const auto &b : buildings) {
    if (b.type == type) {
      return b.count;
    }
  }
  return 0;
}

std::string Settlement::GetBuildingName(SettlementBuildingType type) const {
  switch (type) {
  case SettlementBuildingType::HOUSE:
    return "House";
  case SettlementBuildingType::FARM:
    return "Farm";
  case SettlementBuildingType::LUMBERMILL:
    return "Lumbermill";
  case SettlementBuildingType::MINE:
    return "Mine";
  case SettlementBuildingType::BARRACKS:
    return "Barracks";
  case SettlementBuildingType::TOWER:
    return "Guard Tower";
  default:
    return "Unknown";
  }
}
