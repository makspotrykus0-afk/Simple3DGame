
#include "ColonyAI.h"

#include <map>

#include <iostream>

#include <algorithm>

#include "../systems/BuildingSystem.h"

#include "Colony.h"

#include "Settler.h" // Needed for TaskType

#include "Tree.h" // Needed for Tree

#include "Terrain.h" // Needed for Terrain access

#include "../core/GameSystem.h" // Needed for GameSystem::getTerrain

#include "../game/BuildingTask.h" // Needed for BuildTask
ColonyAI::ColonyAI(Colony *colony, BuildingSystem *buildingSystem)

    : m_colony(colony), m_buildingSystem(buildingSystem), m_timer(0.0f),
      m_updateInterval(0.5f), m_needsHousing(false)

{}
ColonyAI::~ColonyAI()

{}
void ColonyAI::update(float deltaTime)

{

  m_timer += deltaTime;

  if (m_timer >= m_updateInterval) {

    m_timer = 0.0f;

    checkNeeds();

    processConstructionRequests();

    assignJobs(); // New job assignment phase
  }
}
void ColonyAI::registerHouse(Vector3 pos, int size, bool occupied)

{

  float currentTime = (float)GetTime();

  m_knownHouses.push_back({pos, size, occupied, currentTime});
}
void ColonyAI::checkNeeds()

{

  if (!m_colony || !m_buildingSystem)
    return;
  float currentTime = (float)GetTime();

  // REFRESH KNOWN HOUSES FROM ACTUAL WORLD STATE
  m_knownHouses.clear();
  const auto &buildings = m_buildingSystem->getAllBuildings();

  for (const auto &b : buildings) {
    std::string id = b->getBlueprintId();
    // Check if residential
    bool isHouse = (id.find("house") != std::string::npos);
    if (isHouse) {
      int size = 4; // Default (house_4)
      if (id == "house_4")
        size = 4;
      else if (id == "house_6")
        size = 6;
      else if (id == "house_9")
        size = 9;
      // Try parsing dynamic size if still using old format or generic dynamic
      else if (id.find("house_") == 0) {
        try {
          size = std::stoi(id.substr(6));
        } catch (...) {
        }
      }

      // Check occupancy via settler data later, for now add as available
      m_knownHouses.push_back({b->getPosition(), size, false, currentTime});
    }
  }

  // 3. Assign houses to settlers
  auto settlers = m_colony->getSettlers();
  m_needsHousing = false;

      // Phase 1: Stickiness - Keep previous houses if possible
    for (auto* settler : settlers) {
        if (settler->hasHouse) {
             // Find his house and mark as occupied
             Vector3 myHouse = settler->myHousePos();
             for (auto& h : m_knownHouses) {
                 if (!h.occupied && Vector3Distance(h.position, myHouse) < 1.0f) {
                     h.occupied = true;
                     break;
                 }
             }
        }
    }

  // Phase 2: Find new homes for homeless
  for (auto *settler : settlers) {
    if (settler->hasHouse || settler->isIndependent())
      continue;

    // SKIP IF SLEEPING or TIRED - don't interrupt rest with assignment logic
    // that might trigger moves
    if (settler->GetState() == SettlerState::SLEEPING ||
        settler->actionState == "Going to Sleep") {
      continue;
    }

    int bestIdx = -1;
    int minDiff = 999;

    for (size_t i = 0; i < m_knownHouses.size(); ++i) {
      if (m_knownHouses[i].occupied)
        continue;

      // Cast to int to fix warning
      if (m_knownHouses[i].size >= (int)settler->preferredHouseSize) {
        int diff = m_knownHouses[i].size - (int)settler->preferredHouseSize;
        if (diff < minDiff) {
          minDiff = diff;
          bestIdx = (int)i;
        }
      }
    }

    if (bestIdx != -1) {
      m_knownHouses[bestIdx].occupied = true;
      settler->hasHouse = true; // Fixed: Remember assignment!
      settler->myHousePos() = m_knownHouses[bestIdx].position;
      std::cout << "ColonyAI: Assigned " << m_knownHouses[bestIdx].size
                << "-bedroom house to " << settler->getName() << std::endl;
    } else {
      // Need to build!
      m_needsHousing = true;

      // Map preferences to valid blueprints (4, 6, 9)
      int size = (int)settler->preferredHouseSize;
      std::string bpId;
      if (size <= 4) {
        size = 4;
        bpId = "house_4";
      } else if (size <= 6) {
        size = 6;
        bpId = "house_6";
      } else {
        size = 9;
        bpId = "house_9";
      }

      // Check pending builds BEFORE ordering new one
      if (m_buildingSystem->getPendingBuildCount(bpId) > 0) {
        continue; // Already building one of this type, wait
      }

      Vector3 center = settler->getPosition();
      Vector3 buildPos = findBuildPosition(center, 40.0f, bpId);

      if (buildPos.y > -500.0f) {
        bool success = false; // Add success tracking
        BuildTask *task = m_buildingSystem->startBuilding(
            bpId, buildPos, nullptr, 0.0f, false, false, false, &success);

        if (task || success) { // Modified check: task might be null for
                               // composite, but success is true
          std::cout << "ColonyAI: Started building house " << bpId << " for "
                    << settler->getName() << std::endl;
          // Mark as "having a house" immediately to prevent spam, assuming it
          // will be built
          settler->hasHouse = true;
        } else {
          std::cerr << "ColonyAI: Failed to start building " << bpId
                    << std::endl;
        }
      }
    }
  }

  int housedCount = 0;
  for (auto *settler : settlers) {
    if (settler->hasHouse)
      housedCount++;
  }
}
void ColonyAI::processConstructionRequests()

{

  // Logic moved to checkNeeds
}
void ColonyAI::assignJobs() {

  auto settlers = m_colony->getSettlers();

  auto buildTasks = m_buildingSystem->getActiveBuildTasks();

  if (!m_buildingSystem)
    return;
  // Only assign jobs to idle settlers
  for (auto *settler : settlers) {
    if (settler->GetState() != SettlerState::IDLE)
      continue;

    // Skip if hungry or tired (NeedsSystem should handle this, but extra check
    // doesn't hurt)
    if (settler->getStats().getCurrentEnergy() < 20.0f ||
        settler->getStats().getCurrentHunger() < 20.0f)
      continue;

    // Priority 1: Building
    if (settler->performBuilding && !buildTasks.empty()) {
      BuildTask *bestTask = nullptr;
      float minTaskDistSq = 999999.0f;
      Vector3 settlerPos = settler->getPosition();

      for (auto *task : buildTasks) {
        // Limit workers per task to avoid crowding (e.g., 3 workers per
        // building)
        if (task->getWorkerCount() >= 3)
          continue;

        float distSq = Vector3DistanceSqr(settlerPos, task->getPosition());
        if (distSq < minTaskDistSq) {
          minTaskDistSq = distSq;
          bestTask = task;
        }
      }

      if (bestTask) {
        settler->AssignBuildTask(bestTask);
        // Manually increment worker count since Settler::AssignBuildTask might
        // not do it immediately or logic might be split. Ideally
        // Settler::AssignBuildTask calls task->addWorker() Let's assume we need
        // to manage it here or verify if Settler does it. Based on analysis,
        // Settler::AssignBuildTask sets m_currentBuildTask. We should probably
        // let the task know, but `BuildTask` has `addWorker` method. For
        // safety, let's update it here if Settler doesn't. However, if Settler
        // calls it in Update, double counting might occur. Let's verify
        // BuildTask usage in Settler.cpp (it wasn't explicitly calling
        // addWorker in reviewed snippet). So we call it here to be safe and
        // consistent with "centralized" logic.
        bestTask->addWorker(settler);
        std::cout << "ColonyAI: Assigned build task to " << settler->getName()
                  << std::endl;
        continue; // Job assigned, move to next settler
      }
    }

    // Priority 2: Gathering
    // Check if there is a global request for Wood
    if (settler->gatherWood && m_colony->isGatheringTaskActive("Wood")) {
      // Find nearest unreserved tree
      Tree *bestTree = nullptr;
      float minDistSq = 99999.0f;
      Vector3 settlerPos = settler->getPosition();

      Terrain *terrain = GameSystem::getTerrain();
      if (terrain) {
        const auto &trees = terrain->getTrees();

        for (const auto &treePtr : trees) {
          Tree *tree = treePtr.get();
          if (!tree->isActive() || tree->isStump() || tree->isReserved())
            continue;

          float distSq = Vector3DistanceSqr(settlerPos, tree->getPosition());
          // Search radius for gathering e.g. 50 units
          if (distSq < 50.0f * 50.0f && distSq < minDistSq) {
            minDistSq = distSq;
            bestTree = tree;
          }
        }

        if (bestTree) {
          // Decrement task count (assume we took one unit of work)
          // Note: ideally we decrement when task completes, but simple logic:
          // If we take it, maybe we should "claim" one task slot?
          // For now, let's just assign.
          settler->assignTask(TaskType::CHOP_TREE, bestTree,
                              bestTree->getPosition());
          bestTree->reserve(settler->getName()); // Important: Reserve it!
          std::cout << "ColonyAI: Assigned chop tree task to "
                    << settler->getName() << std::endl;
          continue; // Job assigned, move to next settler
        }
      }
    }

    // Priority 3: Hunting (only if huntAnimals job flag is set)
    if (settler->huntAnimals) {
      // Find nearest alive animal
      Animal *bestAnimal = nullptr;
      float minDistSq = 99999.0f;
      Vector3 settlerPos = settler->getPosition();

      const auto &animals = m_colony->getAnimals();

      for (const auto &animalPtr : animals) {
        Animal *animal = animalPtr.get();
        if (!animal || !animal->isActive() || animal->isDead())
          continue;

        float distSq = Vector3DistanceSqr(settlerPos, animal->getPosition());
        // Search radius for hunting: 50 units
        if (distSq < 50.0f * 50.0f && distSq < minDistSq) {
          minDistSq = distSq;
          bestAnimal = animal;
        }
      }

      if (bestAnimal) {
        settler->setState(SettlerState::HUNTING);
        std::cout << "ColonyAI: Assigned hunting task to " << settler->getName()
                  << std::endl;
        continue; // Job assigned, move to next settler
      }
    }
  }
}
Vector3 ColonyAI::findBuildPosition(Vector3 center, float radius,
                                    const std::string &blueprintId)

{

  (void)radius;

  float step = 15.0f;

  int maxSteps = 20;
  float x = 0;
  float z = 0;
  float dx = 0;
  float dz = -1;
  float t = step;

  for (int i = 0; i < maxSteps * maxSteps; ++i) {
    if (-maxSteps / 2 < x && x <= maxSteps / 2 && -maxSteps / 2 < z &&
        z <= maxSteps / 2) {
      Vector3 testPos = {center.x + x * step, 0.0f, center.z + z * step};

      if (m_buildingSystem->canBuild(blueprintId, testPos)) {
        return testPos;
      }
    }

    if (x == z || (x < 0 && x == -z) || (x > 0 && x == 1 - z)) {
      t = dx;
      dx = -dz;
      dz = t;
    }
    x += dx;
    z += dz;
  }

  return {0.0f, -1000.0f, 0.0f};
}
