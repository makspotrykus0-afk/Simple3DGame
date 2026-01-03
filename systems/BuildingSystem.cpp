#include "BuildingSystem.h"
#include "../core/GameEngine.h" // Include GameEngine to access systems
#include "../core/GameSystem.h" // Include GameSystem for getTerrain()
#include "../game/Bed.h"
#include "../game/BuildingBlueprint.h"
#include "../game/BuildingInstance.h" // Ensure BuildingInstance is fully defined
#include "../game/Colony.h"
#include "../game/Door.h"
#include "../game/InteractableObject.h"
#include "../game/Settler.h" // Include Settler
#include "../game/Terrain.h" // Include Terrain to access height
#include "../systems/InteractionSystem.h"
#include "ResourceTypes.h"
#include "StorageSystem.h"
#include "raymath.h"
#include "rlgl.h"
#include <algorithm>
#include <cmath>
#include <iostream>

// Helper to check if a blueprint is a floor
bool isFloor(const std::string &id) {
  return id == "floor" || id.find("floor") != std::string::npos;
}

// Helper to compare colors
bool ColorEqual(Color c1, Color c2) {
  return (c1.r == c2.r) && (c1.g == c2.g) && (c1.b == c2.b) && (c1.a == c2.a);
}

// Static model for rendering cubes
static Model cubeModel = { 0 };
static bool cubeModelLoaded = false;
// Helper function to ensure storage is created for a building instance
void BuildingSystem::EnsureStorageForBuildingInstance(
    BuildingInstance *building) {
  if (!building || !m_storageSystem)
    return;

  // Check if it's a storage building and doesn't have storageId
  const BuildingBlueprint *bp = building->getBlueprint();
  if (!bp)
    return;

  if (bp->getCategory() == BuildingCategory::STORAGE &&
      building->getStorageId().empty()) {
    std::string storageId =
        m_storageSystem->createStorage(StorageType::WAREHOUSE, "Colony");
    building->setStorageId(storageId);
    std::cout << "[BuildingSystem] DEBUG: Assigned storageId=" << storageId
              << " to building blueprint=" << building->getBlueprintId()
              << std::endl;
  }
}

// --- BuildTask Implementations ---

BuildTask::BuildTask(BuildingBlueprint *blueprint, Vector3 position,
                     GameEntity *builder, float rotation)
    : m_blueprint(blueprint), m_position(position), m_builder(builder),
      m_rotation(rotation), m_progress(0.0f), m_state(BuildState::PLANNING),
      m_startTime(0.0f) {
  for (const auto &req : blueprint->getRequirements()) {
    m_collectedResources[req.resourceType] = 0;
  }
}

void BuildTask::addWorker(Settler *worker) {
  if (!worker)
    return;
  for (auto *w : m_workers) {
    if (w == worker)
      return;
  }
  m_workers.push_back(worker);
}

void BuildTask::removeWorker(Settler *worker) {
  auto it = std::remove(m_workers.begin(), m_workers.end(), worker);
  if (it != m_workers.end()) {
    m_workers.erase(it, m_workers.end());
  }
}

bool BuildTask::hasWorker(Settler *settler) const {
  return std::find(m_workers.begin(), m_workers.end(), settler) != m_workers.end();
}

void BuildTask::update(float deltaTime) { (void)deltaTime; }

void BuildTask::cancel() { m_state = BuildState::CANCELLED; }

void BuildTask::addResource(const std::string &resource, int amount) {
  if (m_collectedResources.find(resource) != m_collectedResources.end()) {
    m_collectedResources[resource] += amount;
    std::cout << "[BuildTask] Received " << amount << " " << resource
              << ". Current: " << m_collectedResources[resource] << "/"
              << m_blueprint->getRequirementAmount(resource) << std::endl;

    if (hasAllResources() && m_state == BuildState::PLANNING) {
      m_state = BuildState::CONSTRUCTION;
      std::cout << "[BuildTask] All resources collected. Transitioning to "
                   "CONSTRUCTION."
                << std::endl;
    }
  }
}

float BuildTask::getMaterialProgress01() const {
  int totalRequired = 0;
  int totalCollected = 0;

  for (const auto &req : m_blueprint->getRequirements()) {
    totalRequired += req.amount;
    auto it = m_collectedResources.find(req.resourceType);
    if (it != m_collectedResources.end()) {
      totalCollected += it->second;
    }
  }

  if (totalRequired == 0)
    return 1.0f;
  return (float)totalCollected / (float)totalRequired;
}

std::vector<ResourceRequirement> BuildTask::getMissingResources() const {
  std::vector<ResourceRequirement> missing;
  for (const auto &req : m_blueprint->getRequirements()) {
    int collected = 0;
    auto it = m_collectedResources.find(req.resourceType);
    if (it != m_collectedResources.end()) {
      collected = it->second;
    }

    if (collected < req.amount) {
      ResourceRequirement r = req;
      r.amount -= collected;
      missing.push_back(r);
    }
  }
  return missing;
}

bool BuildTask::hasAllResources() const {
  return getMissingResources().empty();
}

void BuildTask::advanceConstruction(float amount) {
  if (m_state != BuildState::CONSTRUCTION && m_state != BuildState::PLANNING)
    return;

  if (!hasAllResources())
    return;

  if (m_state == BuildState::PLANNING)
    m_state = BuildState::CONSTRUCTION;

  m_progress += amount;

  // Check completion first to clamp progress
  float maxProgress = m_blueprint->getBuildTime() * 100.0f;
  if (m_progress >= maxProgress) {
    m_progress = maxProgress;
    m_state = BuildState::COMPLETED;
  }

  // Log progress occasionally (every 20%)
  if (m_progress - m_lastLogProgress > 20.0f) {
    std::cout << "[BuildTask] Construction progress: " << (int)m_progress << "%"
              << std::endl;
    m_lastLogProgress = m_progress;
  }
}

BoundingBox BuildTask::getBoundingBox() const {
  Vector3 size = m_blueprint->getSize();
  Vector3 halfSize = Vector3Scale(size, 0.5f);
  return BoundingBox{Vector3Subtract(m_position, halfSize),
                     Vector3Add(m_position, halfSize)};
}

// --- BuildingSystem Implementations ---

BuildingSystem::BuildingSystem()
    : GameSystem("BuildingSystem"), m_interactionSystem(nullptr),
      m_colony(nullptr), m_gridSize(1.0f), m_snapToGrid(true),
      m_isPlanning(false) {
  registerDefaultBlueprints();
}

void BuildingSystem::initialize() {
  std::cout << "BuildingSystem initialized." << std::endl;

  // Connect to StorageSystem
  m_storageSystem = GameEngine::getInstance().getSystem<StorageSystem>();
  if (m_storageSystem) {
    std::cout << "BuildingSystem: Connected to StorageSystem." << std::endl;
  } else {
    std::cerr << "BuildingSystem: Failed to connect to StorageSystem!"
              << std::endl;
  }

  if (!cubeModelLoaded) {
    Mesh mesh = GenMeshCube(1.0f, 1.0f, 1.0f);
    cubeModel = LoadModelFromMesh(mesh);
    cubeModelLoaded = true;
  }
  std::cout << "BuildingSystem: Cube model loaded." << std::endl;
}

void BuildingSystem::shutdown() {
  m_buildTasks.clear();
  m_buildings.clear();
  m_blueprints.clear();
  if (cubeModelLoaded) {
    UnloadModel(cubeModel);
    cubeModelLoaded = false;
  }
  std::cout << "BuildingSystem shutdown." << std::endl;
}

void BuildingSystem::ensureBlueprintExists(const std::string &blueprintId) {
  auto it = m_blueprints.find(blueprintId);
  if (it != m_blueprints.end() && it->second)
    return;

  if (blueprintId.find("house_") == 0) {
    std::string sizeStr = blueprintId.substr(6);
    try {
      int size = std::stoi(sizeStr);
      if (size < 4)
        size = 4;

      int width = 2;
      int length = 2;

      if (size == 4) {
        width = 2;
        length = 2;
      } else if (size == 6) {
        width = 3;
        length = 2;
      } else if (size == 9) {
        width = 3;
        length = 3;
      } else {
        width = (int)std::sqrt(size);
        length = size / width;
      }

      std::string name =
          "House " + std::to_string(width) + "x" + std::to_string(length);
      auto house = std::make_unique<BuildingBlueprint>(
          blueprintId, name, BuildingCategory::RESIDENTIAL);

      float halfWidth = (float)width / 2.0f;
      float halfLength = (float)length / 2.0f;

      // Floors
      for (int x = 0; x < width; ++x) {
        for (int z = 0; z < length; ++z) {
          float posX = (x * 2.0f) - (width * 2.0f / 2.0f) + 1.0f;
          float posZ = (z * 2.0f) - (length * 2.0f / 2.0f) + 1.0f;
          house->addComponent({"floor", {posX, 0.0f, posZ}, 0.0f});
        }
      }

      // Walls
      for (int x = 0; x < width; ++x) {
        float posX = (x * 2.0f) - (width * 2.0f / 2.0f) + 1.0f;
        house->addComponent({"wall", {posX, 0.0f, -halfLength * 2.0f}, 0.0f});
      }
      for (int x = 0; x < width; ++x) {
        float posX = (x * 2.0f) - (width * 2.0f / 2.0f) + 1.0f;
        if (x == width / 2) {
          house->addComponent({"door", {posX, 0.0f, halfLength * 2.0f}, 0.0f});
        } else {
          house->addComponent({"wall", {posX, 0.0f, halfLength * 2.0f}, 0.0f});
        }
      }
      for (int z = 0; z < length; ++z) {
        float posZ = (z * 2.0f) - (length * 2.0f / 2.0f) + 1.0f;
        house->addComponent({"wall", {-halfWidth * 2.0f, 0.0f, posZ}, 90.0f});
      }
      for (int z = 0; z < length; ++z) {
        float posZ = (z * 2.0f) - (length * 2.0f / 2.0f) + 1.0f;
        house->addComponent({"wall", {halfWidth * 2.0f, 0.0f, posZ}, 90.0f});
      }

      float bedX = -halfWidth * 2.0f + 1.2f;
      float bedZ = -halfLength * 2.0f + 1.2f;
      house->addComponent({"bed", {bedX, 0.0f, bedZ}, 0.0f});

      house->addCost(Resources::ResourceType::Wood, size * 15 + 5);

      // FIX: Set correct physical size for the bounding box
      // Width/Length are in "grid units" (2.0f world units each)
      // Height is standard wall height (3.0f)
      house->setSize({(float)width * 2.0f, 3.0f, (float)length * 2.0f});

      // Update collision box to match new size (Centered at 0, local)
      float halfW = (float)width * 1.0f;
      float halfL = (float)length * 1.0f;
      house->setCollisionBox({{-halfW, 0.0f, -halfL}, {halfW, 3.0f, halfL}});

      registerBlueprint(std::move(house));
      std::cout << "BuildingSystem: Generated dynamic blueprint " << blueprintId
                << " with Size=" << width * 2 << "x" << length * 2 << std::endl;

    } catch (...) {
      std::cerr << "BuildingSystem: Failed to parse dynamic blueprint ID: "
                << blueprintId << std::endl;
    }
  }
}

void BuildingSystem::registerDefaultBlueprints() {
  auto wall = std::make_unique<BuildingBlueprint>("wall", "Wall",
                                                  BuildingCategory::STRUCTURE);
  wall->setModelPath("resources/models/wall.obj");
  wall->addCost(Resources::ResourceType::Wood, 2);
  wall->setSize({2.0f, 3.0f, 0.5f});
  wall->setCollisionBox({{-1.0f, 0.0f, -0.25f}, {1.0f, 3.0f, 0.25f}});
  registerBlueprint(std::move(wall));

  auto floor = std::make_unique<BuildingBlueprint>("floor", "Floor",
                                                   BuildingCategory::STRUCTURE);
  floor->setModelPath("resources/models/floor.obj");
  floor->addCost(Resources::ResourceType::Wood, 2);
  floor->setSize({2.0f, 0.1f, 2.0f});
  floor->setCollisionBox({{-1.0f, -0.1f, -1.0f}, {1.0f, 0.0f, 1.0f}});
  floor->setWalkable(true);
  registerBlueprint(std::move(floor));

  auto door = std::make_unique<BuildingBlueprint>("door", "Door",
                                                  BuildingCategory::STRUCTURE);
  door->setModelPath("resources/models/door.obj");
  door->addCost(Resources::ResourceType::Wood, 4);
  door->setSize({2.0f, 3.0f, 0.5f});
  door->setCollisionBox({{-1.0f, 0.0f, -0.25f}, {1.0f, 3.0f, 0.25f}});
  registerBlueprint(std::move(door));

  auto bed = std::make_unique<BuildingBlueprint>("bed", "Bed",
                                                 BuildingCategory::FURNITURE);
  bed->setModelPath("resources/models/bed.obj");
  bed->addCost(Resources::ResourceType::Wood, 5);
  bed->setSize({1.5f, 0.5f, 2.0f});
  bed->setCollisionBox({{-0.6f, 0.0f, -0.8f}, {0.6f, 0.4f, 0.8f}});
  registerBlueprint(std::move(bed));

  ensureBlueprintExists("house_4");
  ensureBlueprintExists("house_6");
  ensureBlueprintExists("house_9");

  auto sawmill = std::make_unique<BuildingBlueprint>("sawmill", "Sawmill", BuildingCategory::PRODUCTION);
  sawmill->addCost(Resources::ResourceType::Wood, 50);
  sawmill->setSize({3.0f, 3.2f, 3.0f});
  registerBlueprint(std::move(sawmill));

  auto blacksmith = std::make_unique<BuildingBlueprint>("blacksmith", "Blacksmith", BuildingCategory::PRODUCTION);
  blacksmith->addCost(Resources::ResourceType::Wood, 40);
  blacksmith->addCost(Resources::ResourceType::Metal, 20);
  blacksmith->setSize({3.0f, 3.2f, 3.0f});
  registerBlueprint(std::move(blacksmith));

  auto well = std::make_unique<BuildingBlueprint>("well", "Well", BuildingCategory::PRODUCTION);
  well->addCost(Resources::ResourceType::Wood, 20);
  well->addCost(Resources::ResourceType::Stone, 30);
  well->setSize({2.0f, 2.0f, 2.0f});
  registerBlueprint(std::move(well));

  auto stockpile = std::make_unique<BuildingBlueprint>(
      "stockpile", "Stockpile", BuildingCategory::STORAGE);
  stockpile->setModelPath("resources/models/floor.obj");
  stockpile->addCost(Resources::ResourceType::Wood, 5);
  stockpile->setSize({3.0f, 0.1f, 3.0f});
  stockpile->setCollisionBox({{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}});
  stockpile->setWalkable(true);
  registerBlueprint(std::move(stockpile));

  auto simpleStorage = std::make_unique<BuildingBlueprint>(
      "simple_storage", "Simple Storage", BuildingCategory::STORAGE);
  simpleStorage->setModelPath("resources/models/simple_storage.obj");
  simpleStorage->addCost(Resources::ResourceType::Wood, 10);
  simpleStorage->setSize({3.0f, 0.1f, 3.0f});
  simpleStorage->setCollisionBox({{-1.5f, 0.0f, -1.5f}, {1.5f, 0.1f, 1.5f}});
  registerBlueprint(std::move(simpleStorage));
}

void BuildingSystem::registerBlueprint(
    std::unique_ptr<BuildingBlueprint> blueprint) {
  // Try loading model, but don't fail if missing (will use fallback)
  // Better error handling in loadModel would be ideal, but here we assume
  // loadModel handles its own errors/defaults
  blueprint->loadModel();
  m_blueprints[blueprint->getId()] = std::move(blueprint);
}

void BuildingSystem::loadBlueprints(const std::string &filepath) {
  std::cout << "Loading blueprints from " << filepath << " (Mock)" << std::endl;
}

bool BuildingSystem::canBuild(const std::string &blueprintId,
                              Vector3 /*position*/) {
  const_cast<BuildingSystem *>(this)->ensureBlueprintExists(blueprintId);
  if (m_blueprints.find(blueprintId) == m_blueprints.end())
    return false;
  return true;
}

BuildTask *BuildingSystem::startBuilding(const std::string &blueprintId,
                                         Vector3 position, GameEntity *builder,
                                         float rotation, bool forceNoSnap,
                                         bool ignoreCollision, bool instant,
                                         bool *outSuccess) {
  // Using forceNoSnap and ignoreCollision just to silence warnings for now
  (void)forceNoSnap;
  (void)ignoreCollision;

  std::cout << "BuildingSystem: startBuilding called for " << blueprintId
            << " (instant=" << instant << ")" << std::endl;

  ensureBlueprintExists(blueprintId);

  auto it = m_blueprints.find(blueprintId);
  if (it == m_blueprints.end() || !it->second) {
    std::cerr << "BuildingSystem: Blueprint not found: " << blueprintId
              << std::endl;
    if (outSuccess)
      *outSuccess = false;
    return nullptr;
  }

  BuildingBlueprint *bp = it->second.get();
  std::cout << "BuildingSystem: Found blueprint " << blueprintId
            << ", components: " << bp->getComponents().size() << std::endl;

  // Handle composite blueprints (like houses) by creating individual tasks for
  // parts This allows modular construction (floors, walls) instead of one big
  // task
  if (!instant && !bp->getComponents().empty()) {
    std::cout << "BuildingSystem: Exploding composite blueprint " << blueprintId
              << " into parts." << std::endl;
    bool anyFail = false;
    for (const auto &comp : bp->getComponents()) {
      // Calculate world position and rotation for the component
      Vector3 rotatedOffset = Vector3RotateByAxisAngle(
          comp.localPosition, {0.0f, 1.0f, 0.0f}, rotation * DEG2RAD);
      Vector3 partPos = Vector3Add(position, rotatedOffset);
      float partRot = rotation + comp.localRotation;

      bool partSuccess = false;
      // Recursively start building the part (forcing no snap to ensure it stays
      // relative to parent) We pass 'true' for forceNoSnap because we
      // calculated the exact world position
      startBuilding(comp.blueprintId, partPos, builder, partRot, true,
                    ignoreCollision, instant, &partSuccess);
      if (!partSuccess) {
        std::cerr << "BuildingSystem: Failed to spawn part " << comp.blueprintId
                  << std::endl;
        anyFail = true;
      }
    }

    // CRITICAL FIX: Create a placeholder "Master" BuildingInstance for the
    // composite setup. This ensures ColonyAI sees that a "house" exists (or is
    // under construction) and stops trying to spawn infinite copies of it.
    auto placeholder =
        std::make_unique<BuildingInstance>(blueprintId, position, rotation);
    placeholder->setBuilt(false); // Mark as under construction
    placeholder->setVisible(false); // CRITICAL: Hide it! It's just a logical container.
    m_buildings.push_back(std::move(placeholder));

    if (outSuccess)
      *outSuccess = !anyFail;
    return nullptr; // Return null as there is no single task for the composite
  }

  if (instant) {

    auto building =
        std::make_unique<BuildingInstance>(blueprintId, position, rotation);

    building->setBlueprint(bp);

    building->setBuilt(true); // Fix: Instant buildings are built immediately

    if (builder) {
      if (auto settler = dynamic_cast<Settler *>(builder)) {
        building->setOwner(settler->getName());
      } else {
        building->setOwner(builder->getId());
      }
    } else {
      building->setOwner("Player");
    }

    if (m_storageSystem) {
      // Prioritize specific blueprints if needed, or general category
      if (blueprintId == "simple_storage" || blueprintId == "stockpile" ||
          bp->getCategory() == BuildingCategory::STORAGE) {
        std::string storageId =
            m_storageSystem->createStorage(StorageType::WAREHOUSE, "Colony");
        building->setStorageId(storageId);
        std::cout << "  Storage (Warehouse) created for instant building ("
                  << blueprintId << "): " << storageId << std::endl;
      } else if (bp->getCategory() == BuildingCategory::RESIDENTIAL) {
        std::string storageId =
            m_storageSystem->createStorage(StorageType::CHEST, "Colony");
        building->setStorageId(storageId);
        std::cout << "  Storage (Chest) created for instant house: "
                  << storageId << std::endl;
      }
    }

    if (blueprintId == "door") {
      auto doorObj =
          std::make_shared<Door>("Door", position, rotation, m_colony);
      building->setDoor(doorObj);
      if (m_interactionSystem) {
        m_interactionSystem->registerInteractableObject(doorObj.get());
      }
    } else if (blueprintId == "bed") {
      auto bedObj = std::make_shared<Bed>("Bed", position, rotation);
      building->setBed(bedObj);
      if (m_interactionSystem) {
        m_interactionSystem->registerInteractableObject(bedObj.get());
      }
    } else if (!bp->getComponents().empty()) {
      for (const auto &comp : bp->getComponents()) {
        Vector3 rotatedOffset = Vector3RotateByAxisAngle(
            comp.localPosition, {0.0f, 1.0f, 0.0f}, rotation * DEG2RAD);
        Vector3 compPos = Vector3Add(position, rotatedOffset);
        float compRot = rotation + comp.localRotation;

        if (comp.blueprintId == "door") {
          auto doorObj =
              std::make_shared<Door>("Door", compPos, compRot, m_colony);
          doorObj->setOpen(false);
          building->setDoor(doorObj);
          if (m_interactionSystem) {
            m_interactionSystem->registerInteractableObject(doorObj.get());
          }
        } else if (comp.blueprintId == "bed") {
          auto bedObj = std::make_shared<Bed>("Bed", compPos, compRot);
          building->setBed(bedObj);
          if (m_interactionSystem) {
            m_interactionSystem->registerInteractableObject(bedObj.get());
          }
        }
      }
    }

    // Ensure storage for the building
    EnsureStorageForBuildingInstance(building.get());

    // We need to release the unique_ptr ownership before pushing to vector, or
    // push directly Since startBuilding returns a BuildTask* (or nullptr if
    // instant), we don't need to return the building ptr. However, m_buildings
    // stores unique_ptr<BuildingInstance>. Ensure storage for the building
    EnsureStorageForBuildingInstance(building.get());

    m_buildings.push_back(std::move(building));

    std::cout << "BuildingSystem: Building added to list. Total buildings: "
              << m_buildings.size() << std::endl;

    if (outSuccess)
      *outSuccess = true;
    return nullptr;
  }

  auto task = std::make_unique<BuildTask>(bp, position, builder, rotation);
  BuildTask *rawPtr = task.get();

  if (builder) {
    if (auto settler = dynamic_cast<Settler *>(builder)) {
      rawPtr->addWorker(settler);
    }
  }

  m_buildTasks.push_back(std::move(task));

  if (outSuccess)
    *outSuccess = true;
  return rawPtr;
}

void BuildingSystem::update(float dt) {
  // Update visualizations for storage buildings periodically
  static float timer = 0.0f;
  timer += dt;
  if (timer > 1.0f) {
    timer = 0.0f;
    if (m_storageSystem) {
      // Ensure storage for all buildings (for pre-placed or loaded buildings)
      for (const auto &building : m_buildings) {
        EnsureStorageForBuildingInstance(building.get());
      }

      // Update visualizations for storage buildings
      for (const auto &building : m_buildings) {
        // Check if it's a storage
        std::string bpId = building->getBlueprintId();
        std::string bpName =
            building->getBlueprint() ? building->getBlueprint()->getName() : "";

        // Jeśli budynek jest magazynem (nawet jeśli w trakcie budowy, jeśli ma
        // już przypisane ID magazynu) Sprawdzamy szerzej, czy to magazyn, na
        // podstawie blueprintu lub kategorii
        bool isStorage = (bpId == "simple_storage" || bpId == "stockpile" ||
                          bpName == "storehouse" || bpName == "Stockpile" ||
                          bpName == "Simple Storage");

        if (!isStorage) {
          const BuildingBlueprint *bp = building->getBlueprint();
          if (bp && bp->getCategory() == BuildingCategory::STORAGE) {
            isStorage = true;
          }
        }

        // Jeśli building jest w trakcie budowy, ale ma już storageId (np.
        // instant build lub przypisane przy starcie) to chcemy zaktualizować
        // wizualizację. Jeśli building jest 'under construction' (progress <
        // 100), to normalnie może nie być w pełni funkcjonalny, ale jeśli ma
        // storageId, to znaczy że system magazynowy go obsługuje.

        if (isStorage) {
          std::string storageId = building->getStorageId();
          // std::cout << "[DEBUG] Found storage building " <<
          // building->getBlueprintId() << " ID=" << storageId << std::endl;

          if (!storageId.empty()) {
            auto *storage = m_storageSystem->getStorage(storageId);
            if (storage) {
              // Only update if content changed or periodically?
              // For now we update always to catch animations/changes quickly
              building->updateVisualStorage(storage->slots);
            } else {
              // std::cout << "[DEBUG] Storage instance not found for ID=" <<
              // storageId << std::endl;
            }
          } else {
            // std::cout << "[DEBUG] Storage building has empty storageId" <<
            // std::endl;
          }
        }
      }
    }
  }

  for (auto it = m_buildTasks.begin(); it != m_buildTasks.end();) {
    BuildTask *task = it->get();
    if (task->isCompleted()) {
      completeBuilding(task);
      it = m_buildTasks.erase(it);
    } else {
      ++it;
    }
  }
  for (auto &building : m_buildings) {
    building->update(dt);
    if (building->getDoor()) {
      building->getDoor()->update(dt);
    }
    if (building->getBed()) {
      building->getBed()->update(dt);
    }
  }
}

void BuildingSystem::completeBuilding(BuildTask *task) {
  // Notify workers that task is done
  for (auto *worker : task->getWorkers()) {
    if (worker)
      worker->ClearBuildTask();
  }

  auto building = std::make_unique<BuildingInstance>(
      task->getBlueprint()->getId(), task->getPosition(), task->getRotation());
  building->setBlueprint(task->getBlueprint());

  if (task->getBuilder()) {
    if (auto settler = dynamic_cast<Settler *>(task->getBuilder())) {
      building->setOwner(settler->getName());
    } else {
      building->setOwner(task->getBuilder()->getId());
    }
  } else {
    building->setOwner("Player");
  }

  if (m_storageSystem) {
    std::string bpId = task->getBlueprint()->getId();
    if (bpId == "simple_storage" ||
        task->getBlueprint()->getCategory() == BuildingCategory::STORAGE) {
      std::string storageId =
          m_storageSystem->createStorage(StorageType::WAREHOUSE, "Colony");
      building->setStorageId(storageId);
      std::cout << "  Storage (Warehouse) created for building (" << bpId
                << "): " << storageId << std::endl;
    } else if (task->getBlueprint()->getCategory() ==
               BuildingCategory::RESIDENTIAL) {
      std::string storageId =
          m_storageSystem->createStorage(StorageType::CHEST, "Colony");
      building->setStorageId(storageId);
      std::cout << "  Storage (Chest) created for house: " << storageId
                << std::endl;
    }
  }

  if (task->getBlueprint()->getId() == "door") {
    auto doorObj = std::make_shared<Door>("Door", task->getPosition(),
                                          task->getRotation(), m_colony);
    building->setDoor(doorObj);
    if (m_interactionSystem) {
      m_interactionSystem->registerInteractableObject(doorObj.get());
    }
  } else if (task->getBlueprint()->getId() == "bed") {
    auto bedObj =
        std::make_shared<Bed>("Bed", task->getPosition(), task->getRotation());
    building->setBed(bedObj);
    if (m_interactionSystem) {
      m_interactionSystem->registerInteractableObject(bedObj.get());
    }
  } else if (!task->getBlueprint()->getComponents().empty()) {
    float rotation = task->getRotation();
    Vector3 position = task->getPosition();

    for (const auto &comp : task->getBlueprint()->getComponents()) {
      Vector3 rotatedOffset = Vector3RotateByAxisAngle(
          comp.localPosition, {0.0f, 1.0f, 0.0f}, rotation * DEG2RAD);
      Vector3 compPos = Vector3Add(position, rotatedOffset);
      float compRot = rotation + comp.localRotation;

      if (comp.blueprintId == "door") {
        auto doorObj =
            std::make_shared<Door>("Door", compPos, compRot, m_colony);
        doorObj->setOpen(false);
        building->setDoor(doorObj);
        if (m_interactionSystem) {
          m_interactionSystem->registerInteractableObject(doorObj.get());
        }
      } else if (comp.blueprintId == "bed") {
        auto bedObj = std::make_shared<Bed>("Bed", compPos, compRot);
        building->setBed(bedObj);
        if (m_interactionSystem) {
          m_interactionSystem->registerInteractableObject(bedObj.get());
        }
      }
    }
  }

  m_buildings.push_back(std::move(building));
}

void BuildingSystem::cancelBuilding(BuildTask *task) {
  task->cancel();
  for (auto *worker : task->getWorkers()) {
    if (worker)
      worker->ClearBuildTask();
  }
}

void BuildingSystem::render() {
  for (const auto &building : m_buildings) {
    // CRITICAL: Skip invisible buildings (like composite placeholders)
    if (!building->isVisible()) continue;

    auto it = m_blueprints.find(building->getBlueprintId());
    if (it != m_blueprints.end() && it->second) {
      const auto &blueprint = it->second;
      if (!blueprint->getComponents().empty()) {
        for (const auto &comp : blueprint->getComponents()) {
          if (comp.blueprintId == "door" || comp.blueprintId == "bed") {
            if (comp.blueprintId == "door" && building->getDoor()) {
              building->getDoor()->render();
              continue;
            }
            if (comp.blueprintId == "bed" && building->getBed()) {
              building->getBed()->render();
              continue;
            }
          }

          auto compIt = m_blueprints.find(comp.blueprintId);
          if (compIt != m_blueprints.end() && compIt->second) {
            const auto &compBp = compIt->second;
            Model *model = compBp->getModel();

            Vector3 rotatedOffset =
                Vector3RotateByAxisAngle(comp.localPosition, {0.0f, 1.0f, 0.0f},
                                         building->getRotation() * DEG2RAD);
            Vector3 pos = Vector3Add(building->getPosition(), rotatedOffset);
            float rot = building->getRotation() + comp.localRotation;

            Color color = WHITE;
            if (comp.blueprintId == "wall" || isFloor(comp.blueprintId)) {
              color = BROWN;
            }

            // Fallback rendering if model not loaded or invalid
            if (model && model->meshCount > 0 && model->materialCount > 0) {
              DrawModelEx(*model, pos, {0.0f, 1.0f, 0.0f}, rot,
                          {1.0f, 1.0f, 1.0f}, color);
            } else {
              // Fallback rendering using CubeModel or Matrix Transforms
              Color fallbackColor = color;
              if (ColorEqual(color, WHITE))
                fallbackColor = GRAY;

              Vector3 fallbackPos = pos;
              fallbackPos.y += compBp->getSize().y / 2.0f;

              if (cubeModelLoaded) {
                DrawModelEx(cubeModel, fallbackPos, {0.0f, 1.0f, 0.0f}, rot,
                            compBp->getSize(), color);
              } else {
                // Use rlgl matrix transformations to draw rotated cube
                rlPushMatrix();
                rlTranslatef(fallbackPos.x, fallbackPos.y, fallbackPos.z);
                rlRotatef(rot, 0.0f, 1.0f, 0.0f);
                DrawCube(Vector3{0, 0, 0}, compBp->getSize().x,
                         compBp->getSize().y, compBp->getSize().z,
                         fallbackColor);
                rlPopMatrix();
              }

              // Draw Contours (Wires)
              if (comp.blueprintId == "wall" || isFloor(comp.blueprintId)) {
                rlPushMatrix();
                rlTranslatef(fallbackPos.x, fallbackPos.y, fallbackPos.z);
                rlRotatef(rot, 0.0f, 1.0f, 0.0f);
                DrawCubeWires(Vector3{0.0f, 0.0f, 0.0f}, compBp->getSize().x,
                              compBp->getSize().y, compBp->getSize().z, BLACK);
                rlPopMatrix();
              }
            }
          }
        }
      } else {
        // Simple building (no components)
        Model *model = blueprint->getModel();
        if (model && model->meshCount > 0) {
          DrawModelEx(*model, building->getPosition(), {0.0f, 1.0f, 0.0f},
                      building->getRotation(), {1.0f, 1.0f, 1.0f}, WHITE);
        } else {
          Vector3 size = blueprint->getSize();
          Vector3 pos = building->getPosition();
          pos.y += size.y / 2.0f;

          rlPushMatrix();
          rlTranslatef(pos.x, pos.y, pos.z);
          rlRotatef(building->getRotation(), 0.0f, 1.0f, 0.0f);
          DrawCube(Vector3{0, 0, 0}, size.x, size.y, size.z, GRAY);
          DrawCubeWires(Vector3{0, 0, 0}, size.x, size.y, size.z, DARKGRAY);
          rlPopMatrix();
        }
      }
    }

    if (m_storageSystem && !building->getStorageId().empty()) {
      renderStorageContents(building.get());
    }
  }

  for (const auto &task : m_buildTasks) {
    if (!task->isActive())
      continue;

    const auto &blueprint = task->getBlueprint();
    Vector3 pos = task->getPosition();
    float rotation = task->getRotation();

    float alpha = 0.5f + 0.3f * sinf(GetTime() * 2.0f);
    Color color = {200, 200, 200, (unsigned char)(alpha * 255)};

    // Draw Construction Site Outline (Yellow Box) - REMOVED for modular clutter
    // reduction BoundingBox bbox = task->getBoundingBox();
    // DrawBoundingBox(bbox, YELLOW);

    // Draw "Foundation" Grid - REMOVED
    // DrawGrid(10, 1.0f);
    /*
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y + 0.05f, pos.z);
    DrawPlane({0, 0, 0}, {blueprint->getSize().x, blueprint->getSize().z},
              {255, 255, 0, 50});
    rlPopMatrix();
    */

    // RENDER DEPOSITED RESOURCES
    const auto &collected = task->getCollectedResources();
    int itemCounter = 0;

    // Visualization settings
    float itemSize = 0.25f;
    float spacing = 0.35f;
    int itemsPerRow = 3;

    for (const auto &kv : collected) {
      std::string resName = kv.first;
      int count = kv.second;
      Color resColor = BROWN; // Default Wood
      if (resName == "Stone")
        resColor = GRAY;
      else if (resName == "Wood")
        resColor = {139, 69, 19, 255};
      else if (resName == "Gold")
        resColor = GOLD;

      for (int i = 0; i < count; ++i) {
        if (itemCounter > 50)
          break; // Limit to avoid clutter

        // Stack logic: itemsPerRow x itemsPerRow piles
        int itemsPerFloor = itemsPerRow * itemsPerRow;
        int stackIndex = itemCounter / itemsPerFloor; // Layer
        int floorIndex = itemCounter % itemsPerFloor; // Position in grid
        int row = floorIndex / itemsPerRow;
        int col = floorIndex % itemsPerRow;

        // Position relative to build center, but offset to side or corner?
        // Let's put them inside the bounding box but scattered.

        Vector3 itemPos = pos;
        // Offset to fit inside standard building footprint
        itemPos.x += (col - 1) * spacing;
        itemPos.z += (row - 1) * spacing;
        itemPos.y += stackIndex * itemSize + (itemSize / 2.0f);

        DrawCube(itemPos, itemSize, itemSize, itemSize, resColor);
        DrawCubeWires(itemPos, itemSize, itemSize, itemSize, BLACK);
        itemCounter++;
      }
    }

    // Draw Building Name
    // Simple 3D feedback for started build
    // Visual indicator (Box where text would be, or just the bars are enough if
    // clear) But let's actually make the bars better.

    // REMOVED: 3D Progress Bars (too cluttered with modular buildings)
    /*
    float barWidth = 2.0f;
    float barHeight = 0.2f;
    float barDepth = 0.1f;

    // 1. Material Delivery (Blue)
    Vector3 matBarPos = Vector3Add(pos, {0, 3.0f, 0});
    float matProgress = task->getMaterialProgress01();
    DrawCube(matBarPos, barWidth, barHeight, barDepth, BLACK); // BG
    if (matProgress > 0) {
      DrawCube(Vector3Add(matBarPos,
                          {(matProgress - 1.0f) * barWidth * 0.5f, 0, 0.01f}),
               matProgress * barWidth, barHeight * 0.9f, barDepth, BLUE);
    }

    // 2. Construction Progress (Green)
    Vector3 constBarPos = Vector3Add(pos, {0, 2.7f, 0});
    float constProgress =
        task->getProgress() / (blueprint->getBuildTime() * 100.0f);
    if (constProgress > 1.0f)
      constProgress = 1.0f;

    DrawCube(constBarPos, barWidth, barHeight, barDepth, BLACK); // BG
    if (constProgress > 0) {
      DrawCube(Vector3Add(constBarPos,
                          {(constProgress - 1.0f) * barWidth * 0.5f, 0, 0.01f}),
               constProgress * barWidth, barHeight * 0.9f, barDepth, GREEN);
    }
    */

    // Fallback rendering logic for tasks
    if (!blueprint->getComponents().empty()) {
      // Detailed view: Finished components are solid, others are ghost
      float totalWork = blueprint->getBuildTime() * 100.0f;
      int builtLimit = (int)((task->getProgress() / totalWork) *
                             (float)blueprint->getComponents().size());

      for (int i = 0; i < (int)blueprint->getComponents().size(); ++i) {
        const auto &comp = blueprint->getComponents()[i];
        bool isBuilt = (i < builtLimit);

        auto compIt = m_blueprints.find(comp.blueprintId);
        if (compIt != m_blueprints.end() && compIt->second) {
          const auto &compBp = compIt->second;
          Model *model = compBp->getModel();

          Vector3 rotatedOffset = Vector3RotateByAxisAngle(
              comp.localPosition, {0.0f, 1.0f, 0.0f}, rotation * DEG2RAD);
          Vector3 compPos = Vector3Add(pos, rotatedOffset);
          float compRot = rotation + comp.localRotation;

          // Solid or Ghost?
          Color renderColor = isBuilt ? WHITE : color;
          if (isBuilt &&
              (comp.blueprintId == "wall" || isFloor(comp.blueprintId)))
            renderColor = BROWN;

          // FIX: Handle transparency properly for ghost parts
          bool isGhost = !isBuilt;
          if (isGhost) {
             rlDisableDepthMask(); // Disable depth write for transparent ghosts
          }

          if (model && model->meshCount > 0 && model->materialCount > 0) {
            DrawModelEx(*model, compPos, {0.0f, 1.0f, 0.0f}, compRot,
                        {1.0f, 1.0f, 1.0f}, renderColor);
          } else {
            // Fallback cube for component
            Vector3 fallbackPos = compPos;
            fallbackPos.y += compBp->getSize().y / 2.0f;

            rlPushMatrix();
            rlTranslatef(fallbackPos.x, fallbackPos.y, fallbackPos.z);
            rlRotatef(compRot, 0.0f, 1.0f, 0.0f);
            DrawCube(Vector3{0, 0, 0}, compBp->getSize().x, compBp->getSize().y,
                     compBp->getSize().z, renderColor);
            rlPopMatrix();

            // Always draw wires for fallback visibility
            rlPushMatrix();
            rlTranslatef(fallbackPos.x, fallbackPos.y, fallbackPos.z);
            rlRotatef(compRot, 0.0f, 1.0f, 0.0f);
            DrawCubeWires(Vector3{0, 0, 0}, compBp->getSize().x,
                          compBp->getSize().y, compBp->getSize().z,
                          isBuilt ? BLACK : GREEN);
            rlPopMatrix();
          }

          if (isGhost) {
             rlEnableDepthMask(); // Re-enable depth write
          }
        }
      }
    } else {
      // Simple building (no components) - ALWAYS GHOST until finished
      rlDisableDepthMask(); // Fix for transparency Z-fighting

      if (blueprint->getModel() && blueprint->getModel()->meshCount > 0) {
        DrawModelEx(*blueprint->getModel(), pos, {0.0f, 1.0f, 0.0f}, rotation,
                    {1.0f, 1.0f, 1.0f}, color);
      } else {
        Vector3 size = blueprint->getSize();
        Vector3 renderPos = pos;
        renderPos.y += size.y / 2.0f;

        rlPushMatrix();
        rlTranslatef(renderPos.x, renderPos.y, renderPos.z);
        rlRotatef(rotation, 0.0f, 1.0f, 0.0f);
        DrawCube(Vector3{0, 0, 0}, size.x, size.y, size.z, color);
        rlPopMatrix();
      }
      rlEnableDepthMask();
    }
  }

  if (m_isPlanning && m_selectedBlueprintId != "") {
    renderPreview(m_selectedBlueprintId, m_previewPosition, 0.0f);
  }
}

// Rendering storage visualization
void BuildingSystem::renderStorageContents(BuildingInstance *building) {
  // Check if we have a storage ID
  if (building->getStorageId().empty())
    return;

  // Get terrain for height adjustment if needed
  Terrain *terrain = nullptr;
  if (GameEngine::getInstance().getSystem<GameSystem>()) {
    terrain = GameEngine::getInstance().getSystem<GameSystem>()->getTerrain();
  }

  const auto &visualSlots = building->getVisualSlots();
  // [DEBUG] Rendering storage contents disabled to reduce spam
  // std::cout << "[DEBUG] Rendering storage for " << building->getBlueprintId()
  //           << " (" << building->getStorageId() << ") - Visual Slots: " <<
  //           visualSlots.size() << std::endl;

  rlDisableBackfaceCulling();
  // Render visual slots stored in the building instance
  for (const auto &vSlot : visualSlots) {
    // Transform local position to world position
    Vector3 rotatedOffset =
        Vector3RotateByAxisAngle(vSlot.localPosition, {0.0f, 1.0f, 0.0f},
                                 building->getRotation() * DEG2RAD);
    Vector3 worldPos = Vector3Add(building->getPosition(), rotatedOffset);

    // Special handling for stockpiles to follow terrain
    if (building->getBlueprintId() == "stockpile" && terrain) {
      // For stockpiles, we ignore local Y relative to building and snap to
      // terrain + offset Use vSlot.localPosition.y as offset from ground
      float groundH = terrain->getInterpolatedHeightAt(worldPos.x, worldPos.z);
      worldPos.y = groundH + vSlot.localPosition.y;
    } else {
      // For buildings with floors, use the building's Y + local Y
      // No arbitrary +0.5f here, trust the updated VisualStorageSlot logic
      // But ensure building is not underground
    }

    Color color = RED; // Fallback
    switch (vSlot.type) {
    case Resources::ResourceType::Wood:
      color = BROWN;
      break;
    case Resources::ResourceType::Stone:
      color = GRAY;
      break;
    case Resources::ResourceType::Food:
      color = ORANGE;
      break;
    case Resources::ResourceType::Metal:
      color = LIGHTGRAY;
      break;
    case Resources::ResourceType::Gold:
      color = GOLD;
      break;
    case Resources::ResourceType::Water:
      color = BLUE;
      break;
    default:
      break;
    }

    rlPushMatrix();
    rlTranslatef(worldPos.x, worldPos.y, worldPos.z);
    rlRotatef(building->getRotation(), 0.0f, 1.0f, 0.0f);

    // Draw scaled cube at origin (since we translated)
    DrawCube(Vector3{0, 0, 0}, vSlot.scale.x, vSlot.scale.y, vSlot.scale.z,
             color);
    // DrawCubeWires(Vector3{0,0,0}, vSlot.scale.x, vSlot.scale.y,
    // vSlot.scale.z, YELLOW); // Less visual noise
    rlPopMatrix();
  }
  rlDisableBackfaceCulling();
}

void BuildingSystem::renderPreview(const std::string &blueprintId,
                                   Vector3 position, float rotation) {
  auto it = m_blueprints.find(blueprintId);
  if (it == m_blueprints.end() || !it->second)
    return;

  const auto &bp = it->second;

  Color previewColor = {0, 255, 0, 100};
  if (!canBuild(blueprintId, position)) {
    previewColor = {255, 0, 0, 100};
  }

  // Use component iteration for preview as well for accurate shape
  if (!bp->getComponents().empty()) {
    for (const auto &comp : bp->getComponents()) {
      auto compIt = m_blueprints.find(comp.blueprintId);
      if (compIt != m_blueprints.end() && compIt->second) {
        const auto &compBp = compIt->second;

        Vector3 rotatedOffset = Vector3RotateByAxisAngle(
            comp.localPosition, {0.0f, 1.0f, 0.0f}, rotation * DEG2RAD);
        Vector3 pos = Vector3Add(position, rotatedOffset);
        float rot = rotation + comp.localRotation;

        Vector3 size = compBp->getSize();
        Vector3 renderPos = pos;
        renderPos.y += size.y / 2.0f;

        rlPushMatrix();
        rlTranslatef(renderPos.x, renderPos.y, renderPos.z);
        rlRotatef(rot, 0.0f, 1.0f, 0.0f);
        DrawCube(Vector3{0, 0, 0}, size.x, size.y, size.z, previewColor);
        DrawCubeWires(Vector3{0, 0, 0}, size.x, size.y, size.z, DARKGREEN);
        rlPopMatrix();
      }
    }
  } else {
    // Fallback for simple blueprints
    Vector3 size = bp->getSize();
    Vector3 renderPos = position;
    renderPos.y += size.y / 2.0f;

    rlPushMatrix();
    rlTranslatef(renderPos.x, renderPos.y, renderPos.z);
    rlRotatef(rotation, 0.0f, 1.0f, 0.0f);
    DrawCube(Vector3{0, 0, 0}, size.x, size.y, size.z, previewColor);
    DrawCubeWires(Vector3{0, 0, 0}, size.x, size.y, size.z, DARKGREEN);
    rlPopMatrix();
  }
}

bool BuildingSystem::getBuildingAtRay(Ray ray,
                                      BuildingInstance **outBuilding) const {
  float closestDist = 10000.0f;
  BuildingInstance *hitBuilding = nullptr;

  for (const auto &building : m_buildings) {
    BoundingBox box = building->getBoundingBox();
    RayCollision collision = GetRayCollisionBox(ray, box);

    if (collision.hit && collision.distance < closestDist) {
      closestDist = collision.distance;
      hitBuilding = building.get();
    }
  }

  if (hitBuilding) {
    if (outBuilding)
      *outBuilding = hitBuilding;
    return true;
  }
  return false;
}

std::vector<BuildingBlueprint *>
BuildingSystem::getAvailableBlueprints() const {
  std::vector<BuildingBlueprint *> available;
  for (const auto &pair : m_blueprints) {
    if (pair.second) {
      available.push_back(pair.second.get());
    }
  }
  return available;
}

std::vector<BuildingInstance *>
BuildingSystem::getBuildingsInRange(Vector3 center, float radius) const {
  std::vector<BuildingInstance *> result;
  for (const auto &building : m_buildings) {
    if (Vector3Distance(building->getPosition(), center) <= radius) {
      result.push_back(building.get());
    }
  }
  return result;
}

std::vector<BuildingInstance *> BuildingSystem::getAllBuildings() const {
  std::vector<BuildingInstance *> result;
  for (const auto &building : m_buildings) {
    result.push_back(building.get());
  }
  return result;
}

BuildingInstance *BuildingSystem::getBuildingAt(Vector3 position) const {
  for (const auto &building : m_buildings) {
    if (Vector3Distance(building->getPosition(), position) < 0.5f) {
      return building.get();
    }
  }
  return nullptr;
}

BuildTask *BuildingSystem::getBuildTaskAt(Vector3 position,
                                          float radius) const {
  for (const auto &task : m_buildTasks) {
    if (Vector3Distance(task->getPosition(), position) <= radius) {
      return task.get();
    }
  }
  return nullptr;
}

int BuildingSystem::getPendingBuildCount(const std::string &blueprintId) const {
  int count = 0;
  for (const auto &task : m_buildTasks) {
    if (!task->isCompleted() && task->getBlueprint()->getId() == blueprintId) {
      count++;
    }
  }
  return count;
}

std::vector<BuildTask *> BuildingSystem::getActiveBuildTasks() const {
  std::vector<BuildTask *> activeTasks;
  for (const auto &task : m_buildTasks) {
    if (!task->isCompleted() && task->getState() != BuildState::CANCELLED) {
      activeTasks.push_back(task.get());
    }
  }
  return activeTasks;
}

void BuildingSystem::enablePlanningMode(bool enable) { m_isPlanning = enable; }

void BuildingSystem::setSelectedBlueprint(const std::string &blueprintId) {
  m_selectedBlueprintId = blueprintId;
}

void BuildingSystem::updatePreviewPosition(Vector3 pos) {
  m_previewPosition = pos;
}