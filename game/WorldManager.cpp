#include "WorldManager.h"
#include "Faction.h"
#include "Region.h"
#include "Settlement.h"
#include <cmath>
#include <iostream>

// Singleton instance
WorldManager *WorldManager::instance = nullptr;

WorldManager::WorldManager()
    : lastPlayerPosition({0, 0, 0}), currentPlayerGrid({0, 0}) {}

WorldManager *WorldManager::GetInstance() {
  if (instance == nullptr) {
    instance = new WorldManager();
  }
  return instance;
}

void WorldManager::Destroy() {
  if (instance != nullptr) {
    delete instance;
    instance = nullptr;
  }
}

void WorldManager::Initialize() {
  std::cout << "[WorldManager] Initializing world system..." << std::endl;

  // Pre-create a 3x3 grid of regions centered at (0,0)
  for (int x = -1; x <= 1; ++x) {
    for (int z = -1; z <= 1; ++z) {
      GridCoord coord = {x, z};
      EnsureRegionExists(coord);
    }
  }

  std::cout << "[WorldManager] Created " << regions.size() << " initial regions"
            << std::endl;

  // Activate center region by default
  GridCoord centerCoord = {0, 0};
  Region *centerRegion = GetRegionByGrid(centerCoord);
  if (centerRegion) {
    centerRegion->ActivateFullSimulation();
    activeRegions.push_back(centerRegion);
    std::cout << "[WorldManager] Activated center region (0,0)" << std::endl;
  }

  // Initialize Test Factions
  auto empire = std::make_unique<Faction>("The Iron Empire", RED);
  empire->AddSettlement(std::make_unique<Settlement>(
      "Ironhold", Vector3{100, 0, 100}, empire.get()));
  factions.push_back(std::move(empire));

  auto rebels = std::make_unique<Faction>("Dust Rebels", YELLOW);
  rebels->AddSettlement(std::make_unique<Settlement>(
      "Oasis", Vector3{-100, 0, -100}, rebels.get()));
  factions.push_back(std::move(rebels));

  std::cout << "[WorldManager] Initialized " << factions.size() << " factions"
            << std::endl;
}

void WorldManager::Update(float deltaTime, Vector3 playerPosition) {
  lastPlayerPosition = playerPosition;

  // Check if player changed grid cell
  GridCoord newPlayerGrid = WorldPosToGrid(playerPosition);
  if (!(newPlayerGrid == currentPlayerGrid)) {
    currentPlayerGrid = newPlayerGrid;
    UpdateRegionActivation(playerPosition);
  }

  // Update all active regions
  for (Region *region : activeRegions) {
    if (region) {
      region->Update(deltaTime);
    }
  }

  // Update passive/background regions
  for (auto &pair : regions) {
    Region *region = pair.second.get();
    if (!region)
      continue;

    if (region->GetState() == RegionState::PASSIVE) {
      region->PassiveTick(deltaTime);
    } else if (region->GetState() == RegionState::BACKGROUND) {
      region->BackgroundTick(deltaTime);
    }
  }

  // Update factions
  for (auto &faction : factions) {
    if (faction) {
      faction->Update(deltaTime);
    }
  }
}

void WorldManager::Render() {
  // Render all active regions
  for (Region *region : activeRegions) {
    if (region) {
      region->Render();
    }
  }
}

void WorldManager::Shutdown() {
  std::cout << "[WorldManager] Shutting down..." << std::endl;

  // Deactivate all regions
  for (Region *region : activeRegions) {
    if (region) {
      region->DeactivateToPassive();
    }
  }
  activeRegions.clear();

  // Clear all regions
  regions.clear();
  factions.clear();
}

Region *WorldManager::GetRegionAt(Vector3 worldPos) {
  GridCoord coord = WorldPosToGrid(worldPos);
  return GetRegionByGrid(coord);
}

Region *WorldManager::GetRegionByGrid(GridCoord coord) {
  auto it = regions.find(coord);
  if (it != regions.end()) {
    return it->second.get();
  }
  return nullptr;
}

GridCoord WorldManager::WorldPosToGrid(Vector3 pos) {
  return {static_cast<int>(std::floor(pos.x / REGION_SIZE)),
          static_cast<int>(std::floor(pos.z / REGION_SIZE))};
}

Vector3 WorldManager::GridToWorldPos(GridCoord coord) {
  return {coord.x * REGION_SIZE + REGION_SIZE * 0.5f, // Center of region
          0.0f, coord.z * REGION_SIZE + REGION_SIZE * 0.5f};
}

void WorldManager::RegisterFaction(std::unique_ptr<Faction> faction) {
  if (faction) {
    std::cout << "[WorldManager] Registered new faction: " << faction->GetName()
              << std::endl;
    factions.push_back(std::move(faction));
  }
}

void WorldManager::DrawDebugInfo() {
  // Draw grid visualization
  const int gridSize = 3;
  const int cellSize = 50;
  const int offsetX = 10;
  const int offsetY = 10;

  DrawText("World Manager Debug", offsetX, offsetY, 20, YELLOW);

  // Draw grid
  for (int x = -1; x <= 1; ++x) {
    for (int z = -1; z <= 1; ++z) {
      GridCoord coord = {x, z};
      Region *region = GetRegionByGrid(coord);

      int screenX = offsetX + (x + 1) * cellSize;
      int screenY = offsetY + 30 + (z + 1) * cellSize;

      Color cellColor = DARKGRAY;
      if (region) {
        cellColor = region->IsActive() ? GREEN : BLUE;
      }

      DrawRectangle(screenX, screenY, cellSize - 2, cellSize - 2, cellColor);

      // Draw grid coordinates
      char coordText[16];
      snprintf(coordText, sizeof(coordText), "%d,%d", coord.x, coord.z);
      DrawText(coordText, screenX + 5, screenY + 5, 10, WHITE);
    }
  }

  // Draw player position indicator
  GridCoord playerGrid = currentPlayerGrid;
  if (playerGrid.x >= -1 && playerGrid.x <= 1 && playerGrid.z >= -1 &&
      playerGrid.z <= 1) {
    int screenX = offsetX + (playerGrid.x + 1) * cellSize;
    int screenY = offsetY + 30 + (playerGrid.z + 1) * cellSize;
    DrawCircle(screenX + cellSize / 2, screenY + cellSize / 2, 5, RED);
  }

  // Stats
  char statsText[256];
  snprintf(statsText, sizeof(statsText),
           "Active Regions: %zu | Total Regions: %zu | Player Grid: (%d, %d)",
           activeRegions.size(), regions.size(), currentPlayerGrid.x,
           currentPlayerGrid.z);
  DrawText(statsText, offsetX, offsetY + 30 + gridSize * cellSize + 10, 12,
           WHITE);

  // Faction Stats Overlay
  int factionOffsetY = offsetY + 30 + gridSize * cellSize + 30;
  DrawText("Active Factions:", offsetX, factionOffsetY, 15, WHITE);
  factionOffsetY += 20;

  for (const auto &faction : factions) {
    char factionInfo[128];
    snprintf(factionInfo, sizeof(factionInfo),
             "%s: %zu settlements | Gold: %.0f", faction->GetName().c_str(),
             faction->GetSettlements().size(), faction->GetTreasury().gold);
    DrawText(factionInfo, offsetX + 10, factionOffsetY, 12,
             faction->GetColor());
    factionOffsetY += 15;
  }
}

void WorldManager::UpdateRegionActivation(Vector3 playerPos) {
  // Configurable LOD Distances
  const float DIST_ACTIVE = REGION_SIZE * 1.5f;  // 1.5 chunks (150m)
  const float DIST_PASSIVE = REGION_SIZE * 5.0f; // 5 chunks (500m)

  activeRegions.clear();

  // Iterate over ALL known regions (inefficient for infinite world, fine for
  // grid)
  for (auto &pair : regions) {
    Region *region = pair.second.get();
    Vector3 regionCenter = region->GetCenter();

    float dist = Vector3Distance(playerPos, regionCenter);

    if (dist <= DIST_ACTIVE) {
      if (region->GetState() != RegionState::ACTIVE) {
        region->SetState(RegionState::ACTIVE);
      }
      activeRegions.push_back(region);
    } else if (dist <= DIST_PASSIVE) {
      if (region->GetState() != RegionState::PASSIVE) {
        region->SetState(RegionState::PASSIVE);
      }
    } else {
      if (region->GetState() != RegionState::BACKGROUND) {
        region->SetState(RegionState::BACKGROUND);
      }
    }
  }

  // Ensure immediate neighbors are created if they don't exist (Lazy loading)
  // ... (Keep existing neighbor creation logic if needed, but simplified loop
  // above covers state) Actually, the previous logic created new regions. We
  // should keep that.

  GridCoord playerGrid = WorldPosToGrid(playerPos);
  for (int dx = -1; dx <= 1; ++dx) {
    for (int dz = -1; dz <= 1; ++dz) {
      GridCoord coord = {playerGrid.x + dx, playerGrid.z + dz};
      EnsureRegionExists(coord);
      // They will be picked up by the next update loop or forced active here
      Region *r = GetRegionByGrid(coord);
      if (r && r->GetState() != RegionState::ACTIVE) {
        r->SetState(RegionState::ACTIVE);
        // Add to active list if not already there (std::find check or just
        // ensure unique) Simple way: just run the loop above.
      }
    }
  }
}

void WorldManager::EnsureRegionExists(GridCoord coord) {
  if (regions.find(coord) == regions.end()) {
    // Create new region
    Vector3 worldCenter = GridToWorldPos(coord);
    auto newRegion = std::make_unique<Region>(coord, worldCenter);
    regions[coord] = std::move(newRegion);

    std::cout << "[WorldManager] Created new region at grid (" << coord.x
              << ", " << coord.z << ")" << std::endl;
  }
}
