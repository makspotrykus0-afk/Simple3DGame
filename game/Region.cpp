#include "Region.h"
#include "BuildingInstance.h"
#include "Colony.h"
#include "ResourceNode.h"
#include "Terrain.h"
#include "Tree.h"
#include <iostream>

Region::Region(GridCoord coord, Vector3 center)
    : gridCoord(coord), worldCenter(center), state(RegionState::UNINITIALIZED),
      timeSinceLastPassiveTick(0.0f) {
  std::cout << "[Region (" << coord.x << "," << coord.z
            << ")] Created at world pos (" << center.x << ", " << center.z
            << ")" << std::endl;
}

Region::~Region() {
  if (state == RegionState::ACTIVE) {
    DeactivateToPassive();
  }
}

// ActivateFullSimulation is now a wrapper around SetState(ACTIVE)
// Implementation moved below SetState in previous chunk

void Region::SetState(RegionState newState) {
  if (state == newState)
    return;

  RegionState oldState = state;

  // EXITING ACTIVE STATE
  if (oldState == RegionState::ACTIVE) {
    std::cout << "[Region (" << gridCoord.x << "," << gridCoord.z
              << ")] Deactivating..." << std::endl;
    SyncToPassiveState();

    colony.reset();
    terrain.reset();
    treesInRegion.clear();
    resourceNodesInRegion.clear();
    buildingsInRegion.clear();
  }

  state = newState;

  // ENTERING ACTIVE STATE
  if (newState == RegionState::ACTIVE) {
    std::cout << "[Region (" << gridCoord.x << "," << gridCoord.z
              << ")] Activating full simulation..." << std::endl;
    InitializeActiveState();
    // Restore from abstract state
    if (oldState == RegionState::PASSIVE ||
        oldState == RegionState::BACKGROUND) {
      SyncFromPassiveState();
    }
  }
}

void Region::ActivateFullSimulation() { SetState(RegionState::ACTIVE); }

void Region::DeactivateToPassive() { SetState(RegionState::PASSIVE); }

void Region::BackgroundTick(float deltaTime) {
  // Extremely simplified simulation or just time accumulation
  // For now, let's say background regions just accumulate "Pending Growth"
  // without running the expensive PassiveTick logic every 5 seconds.
  // They might only update once per minute.

  timeSinceLastPassiveTick += deltaTime;

  // If we accumulate enough time, maybe we do one PassiveTick update?
  // Let's make Background update 4x slower than Passive
  if (timeSinceLastPassiveTick >= PASSIVE_TICK_INTERVAL * 4.0f) {
    // Run one tick of passive logic but scale it?
    // Or just run normal PassiveTick? Let's alias it for simplicity.
    // But we need to fake the time diff.

    // Strategy: Just run PassiveTick but less often.
    // It uses PASSIVE_TICK_INTERVAL constant, so we need to be careful.
    // Actually, PassiveTick implementation uses global constants.
    // Let's just run logic here manually or call PassiveTick.

    // Run Passive Tick logic
    state = RegionState::PASSIVE; // Temp switch to allow tick
    PassiveTick(deltaTime);
    state = RegionState::BACKGROUND; // Switch back

    // Reset timer is handled in PassiveTick if it triggers
  }
}

void Region::Update(float deltaTime) {
  if (state != RegionState::ACTIVE) {
    return;
  }

  // Update colony if exists
  if (colony) {
    // Phase 0: Colony update needs trees and buildings lists
    // These will be populated from global lists in main.cpp integration
    std::vector<std::unique_ptr<Tree>> emptyTrees; // Placeholder
    std::vector<std::unique_ptr<BuildingInstance>>
        emptyBuildings; // Placeholder

    // TODO: In main.cpp integration, pass real trees and buildings
    // colony->update(deltaTime, emptyTrees, emptyBuildings);
  }
}

void Region::PassiveTick(float deltaTime) {
  if (state == RegionState::ACTIVE) {
    return; // Don't passive tick when active
  }

  timeSinceLastPassiveTick += deltaTime;

  if (timeSinceLastPassiveTick >= PASSIVE_TICK_INTERVAL) {
    timeSinceLastPassiveTick = 0.0f;

    // --- DARK FANTASY PASSIVE SIMULATION ---

    // 1. Corruption Spread (New Mechanic)
    // Regions slowly accumulate corruption if abandoned
    passiveState.growthTimer +=
        PASSIVE_TICK_INTERVAL; // abusing growthTimer as corruption for now

    // 2. Population Struggle in the Dark
    if (passiveState.abstractFood >= passiveState.abstractPopulation) {
      // Surplus: Slow growth (fear inhibits reproduction)
      passiveState.abstractPopulation += 0.05f;
    } else {
      // Famine: Rapid decline + Death from darkness
      passiveState.abstractPopulation -= 0.2f;
    }

    // Hard cap population based on "Corruption" (growthTimer)
    if (passiveState.abstractPopulation >
        50.0f - (passiveState.growthTimer * 0.1f)) {
      passiveState.abstractPopulation -= 0.5f; // Culling
    }

    // 3. Resource Production (Grim Harvest)
    // Villagers produce less when afraid
    float efficiency = 0.8f;
    if (passiveState.growthTimer > 100.0f)
      efficiency = 0.5f;

    passiveState.abstractFood +=
        passiveState.abstractPopulation * 0.4f * efficiency;
    passiveState.abstractWood +=
        passiveState.abstractPopulation * 0.3f * efficiency;

    // 4. Consumption (Survival)
    passiveState.abstractFood -= passiveState.abstractPopulation * 0.35f;
    if (passiveState.abstractFood < 0)
      passiveState.abstractFood = 0;
    if (passiveState.abstractPopulation < 0)
      passiveState.abstractPopulation = 0;
  }
}

void Region::Render() {
  if (state != RegionState::ACTIVE) {
    return;
  }

  // Render terrain
  if (terrain) {
    terrain->render();
  }

  // Render colony
  if (colony) {
    colony->render();
  }

// Debug: Draw region bounds
#ifdef DEBUG_REGIONS
  DrawDebugBounds();
#endif
}

void Region::DrawDebugBounds() {
  const float halfSize = 50.0f; // REGION_SIZE / 2

  Vector3 corners[4] = {
      {worldCenter.x - halfSize, 0.1f, worldCenter.z - halfSize},
      {worldCenter.x + halfSize, 0.1f, worldCenter.z - halfSize},
      {worldCenter.x + halfSize, 0.1f, worldCenter.z + halfSize},
      {worldCenter.x - halfSize, 0.1f, worldCenter.z + halfSize}};

  Color lineColor = (state == RegionState::ACTIVE) ? GREEN : BLUE;

  for (int i = 0; i < 4; ++i) {
    DrawLine3D(corners[i], corners[(i + 1) % 4], lineColor);
  }

  // Draw center marker
  DrawSphere(worldCenter, 1.0f, lineColor);
}

void Region::InitializeActiveState() {
  std::cout << "[Region (" << gridCoord.x << "," << gridCoord.z
            << ")] Initializing active state..." << std::endl;

  // Create terrain (Phase 0: simple flat terrain)
  // TODO: Create proper procedural terrain based on grid coordinates
  // terrain = std::make_unique<Terrain>();

  // Create colony (Phase 0: basic empty colony)
  // TODO: Create colony with proper initialization
  // colony = std::make_unique<Colony>();

  std::cout << "[Region (" << gridCoord.x << "," << gridCoord.z
            << ")] Active state initialized" << std::endl;
}

void Region::SyncToPassiveState() {
  if (!colony) {
    return;
  }

  // Extract abstract data from Colony
  passiveState.abstractPopulation =
      static_cast<float>(colony->getSettlers().size());

  // Extract resources from colony storage
  passiveState.abstractFood = static_cast<float>(colony->getFood());
  passiveState.abstractWood = static_cast<float>(colony->getWood());
  passiveState.abstractStone = static_cast<float>(colony->getStone());

  std::cout << "[Region (" << gridCoord.x << "," << gridCoord.z
            << ")] Synced to passive: pop=" << passiveState.abstractPopulation
            << " food=" << passiveState.abstractFood << std::endl;
}

void Region::SyncFromPassiveState() {
  if (!colony) {
    return;
  }

  // Restore Colony from abstract data
  int targetPopulation = static_cast<int>(passiveState.abstractPopulation);

  // Spawn settlers to match abstract population
  while (static_cast<int>(colony->getSettlers().size()) < targetPopulation) {
    Vector3 spawnPos = worldCenter;
    spawnPos.x += GetRandomValue(-10, 10);
    spawnPos.z += GetRandomValue(-10, 10);
    colony->addSettler(spawnPos, "Survivor");
  }

  // TODO: Restore resources (Requires Colony::SetResource methods which are
  // missing in Phase 0) For now we just accept the decay happened in passive
  // mode

  std::cout << "[Region (" << gridCoord.x << "," << gridCoord.z
            << ")] Synced from passive: target pop=" << targetPopulation
            << std::endl;
}
