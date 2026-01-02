#include <memory>

#include <vector>

#include <algorithm>

#include <string>

#include <map>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h> // Matrix rotation

#include "Settler.h"
#include "Terrain.h"
#include "Tree.h"

#include "BuildingInstance.h"

#include "Colony.h"

#include "Bed.h"

#include "ResourceNode.h"

#include "GatheringTask.h"

#include "NavigationGrid.h"

#include "Animal.h"

#include "BuildingTask.h"

#include "../core/GameEngine.h"
#include "Item.h"
#include "Projectile.h"

#include "../core/GameSystem.h"

#include "../systems/BuildingSystem.h"

#include "../systems/StorageSystem.h"

#include "../systems/SkillTypes.h"

#include "../components/EnergyComponent.h"
#include "../components/PositionComponent.h"
#include "../systems/CraftingSystem.h"

#include "../components/SkillsComponent.h"

#include "../components/StatsComponent.h"

#include "../components/PositionComponent.h"

#include "../components/InventoryComponent.h"
#include "raymath.h"
extern BuildingSystem *g_buildingSystem;
extern Colony *g_colony;

// Access global camera from main.cpp
extern Camera3D sceneCamera;

static void DrawProgressBar3D(Vector3 position, float progress, Color color) {
  Vector3 barPos = position;
  barPos.y += 1.8f; // Lowered from 2.5f to be more visible (above wood at 1.2f)

  // Background (Cube behaving as bar)
  DrawCube(barPos, 1.0f, 0.15f, 0.05f, BLACK);

  // Foreground
  if (progress > 0) {
    float width = 1.0f * progress;
    // Center aligned scaling, simple enough for feedback
    DrawCube(barPos, width, 0.16f, 0.06f,
             color); // Slightly thicker freq or just diff color
  }
}
Settler::Settler(const std::string &name, const Vector3 &pos,
                 SettlerProfession profession)

    : GameEntity(name), m_name(name), m_profession(profession),
      m_state(SettlerState::IDLE),

      m_inventory(name, 50.0f, this),

      m_stats(name, 100.0f, 100.0f, 100.0f, 100.0f),

      m_targetPosition(pos), m_moveSpeed(5.0f), m_rotation(0.0f),
      m_weaponSpeed(120.0f),

      m_currentBuildTask(nullptr), m_currentGatherTask(nullptr),
      m_targetStorage(nullptr), m_targetWorkshop(nullptr),

      m_gatherTimer(0.0f), m_gatherInterval(1.0f), m_isSelected(false),

      m_currentGatherBush(nullptr), m_currentTree(nullptr),
      m_assignedBed(nullptr), m_targetFoodBush(nullptr),

      m_eatingTimer(0.0f), m_craftingTimer(0.0f), m_currentPathIndex(0),
      m_aiSearchTimer((float)(rand() % 100) /
                      200.0f), // Random offset 0-0.5s to desync settlers
      m_isMovingToCriticalTarget(false), m_pendingReevaluation(false),
      m_sleepCooldownTimer(0.0f), m_eatingCooldownTimer(0.0f),
      m_sleepEnterThreshold(30.0f), m_sleepExitThreshold(80.0f),
      m_hungerEnterThreshold(40.0f), m_hungerExitThreshold(80.0f),
      m_isIndependentBuilder(false), m_myPrivateBuildTask(nullptr) {

  position = pos;
  addComponent(std::make_shared<PositionComponent>(position));
  addComponent(std::make_shared<EnergyComponent>());

  m_skills.addSkill(SkillType::WOODCUTTING, 1);
  m_skills.addSkill(SkillType::MINING, 1);
  m_skills.addSkill(SkillType::BUILDING, 1);
  m_skills.addSkill(SkillType::FARMING, 1);

  m_prevGatherWood = gatherWood;
  m_prevGatherStone = gatherStone;
  m_prevGatherFood = gatherFood;
  m_prevPerformBuilding = performBuilding;
  m_prevHuntAnimals = huntAnimals;
  m_prevCraftItems = craftItems;
  m_prevHaulToStorage = haulToStorage;
  m_prevTendCrops = tendCrops;
}
Settler::~Settler() {

  if (m_currentGatherTask)
    delete m_currentGatherTask;
}

Vector3 Settler::getMuzzlePosition() const {
  // Muzzle position relative to settler
  // Based on visual render:
  // Hand is at: Right 0.3, Up 1.5, Forward 0.5 (approx)
  // Weapon extends forward from hand.

  // Let's refine based on render():
  // Hand pivot: Right 0.3, Up ~1.5 (bodyY+0.6)
  // If aiming, arm is -90 deg pitch (horizontal).
  // Item attached: translated 0, -0.45, 0.2 from HAND PIVOT.
  // If Arm is horizontal forward:
  // Hand PIVOT is at (0.3, 1.5, 0) relative to body center.
  // Arm length 0.45 downward... wait.
  // Let's approximate a good "Gun Barrel" position.

  float rotRad = m_rotation * DEG2RAD;
  Vector3 forward = {sinf(rotRad), 0.0f, cosf(rotRad)};
  Vector3 right = {cosf(rotRad), 0.0f, -sinf(rotRad)};

  Vector3 muzzlePos = position;
  // Offset to Right Shoulder/Hand
  muzzlePos = Vector3Add(muzzlePos, Vector3Scale(right, 0.25f));
  // Height (Shoulder/Eye level)
  muzzlePos.y += 1.45f;
  // Forward (Length of arm + gun)
  muzzlePos = Vector3Add(muzzlePos, Vector3Scale(forward, 0.8f));

  return muzzlePos;
}

void Settler::shoot(Vector3 targetPos) {
  if (m_shootCooldownTimer > 0.0f)
    return;

  // Use precise muzzle position
  Vector3 muzzlePos = getMuzzlePosition();

  // Apply spread to target position
  float dist = Vector3Distance(muzzlePos, targetPos);

  // Reduced spread calculation
  // Previously: dist * m_weaponSpread.
  // Now m_weaponSpread is 0.005, so spread is much tighter.
  float spreadAmount = dist * m_weaponSpread;

  // Random spread offsets
  float spreadX = ((float)(rand() % 100) / 50.0f - 1.0f) * spreadAmount;
  float spreadY = ((float)(rand() % 100) / 50.0f - 1.0f) * spreadAmount;
  float spreadZ = ((float)(rand() % 100) / 50.0f - 1.0f) * spreadAmount;

  Vector3 finalTarget = Vector3Add(targetPos, {spreadX, spreadY, spreadZ});

  // Create projectile
  // Force alignment: Ensure direction vector is normalized in Projectile or
  // just pass start/end
  auto projectile = std::make_unique<Projectile>(muzzlePos, finalTarget,
                                                 m_weaponSpeed, m_weaponDamage);

  // Add to world
  Colony *colony = GameSystem::getColony();
  if (colony) {
    colony->addProjectile(std::move(projectile));
  }

  // Set cooldown
  m_shootCooldownTimer = 0.5f; // 2 shots per second (adjustable)

  // std::cout << "[COMBAT] " << m_name << " fired a shot!" << std::endl;
}

void Settler::useHeldItem(Vector3 targetPos) {
  if (m_shootCooldownTimer > 0.0f)
    return;
  if (m_attackAnimTimer > 0.0f)
    return; // Wait for animation

  std::string itemName = "";
  if (m_heldItem)
    itemName = m_heldItem->getDisplayName();

  // 1. Ranged Weapons
  if (itemName == "Sniper Rifle") {
    shoot(targetPos);
    return;
  }

  // 2. Melee Tools / Universal Swing
  m_attackAnimTimer = 1.0f;     // Start animation
  m_waitingToDealDamage = true; // Wait for hit frame

  // Check range/melee logic
  Colony *colony = GameSystem::getColony();
  Terrain *terrain = GameSystem::getTerrain();

  if (colony && terrain) {
    // Check Trees
    const auto &trees = terrain->getTrees();
    for (const auto &t : trees) {
      if (t->isActive() && !t->isStump()) {
        float d = Vector3Distance(
            targetPos, t->getPosition()); // Click point to tree center

        if (d < 1.5f) {
          // Also check if Settler is close enough to tree (Melee range)
          if (Vector3Distance(position, t->getPosition()) < 3.0f) {
            // Target locked for hit frame
            m_currentTree = t.get();
            return;
          }
        }
      }
    }
  }
}

void Settler::Update(
    float deltaTime,

    const std::vector<std::unique_ptr<Tree>> &trees,

    std::vector<WorldItem> &worldItems,

    const std::vector<Bush *> &bushes,

    const std::vector<BuildingInstance *> &buildings,

    const std::vector<std::unique_ptr<Animal>> &animals,

    const std::vector<std::unique_ptr<ResourceNode>> &resourceNodes) {
  if (!m_stats.isAlive())
    return;

  // Update Animation Timer (Global) - MUST BE BEFORE EARLY RETURN FOR PLAYER
  if (m_attackAnimTimer > 0.0f) {
    float prevTimer = m_attackAnimTimer;
    m_attackAnimTimer -=
        deltaTime * 1.5f; // Prędkość całkowita animacji (ok 0.66s)
    if (m_attackAnimTimer < 0.0f)
      m_attackAnimTimer = 0.0f;

    // LOGIKA OPÓŹNIONEGO DAMAGE (HIT FRAME)
    if (m_waitingToDealDamage && m_attackAnimTimer <= 0.4f &&
        prevTimer > 0.4f) {
      std::string itemName = m_heldItem ? m_heldItem->getDisplayName() : "";

      if (m_currentTree && !m_currentTree->isStump() &&
          Vector3Distance(position, m_currentTree->getPosition()) < 3.5f) {
        float damage = (itemName == "Stone Axe") ? 20.0f : 5.0f;
        std::cout << "[Settler] HIT FRAME: Chopping tree for " << damage
                  << " dmg." << std::endl;
        m_currentTree->harvest(damage);
      } else if (m_currentTargetAnimal && !m_currentTargetAnimal->isDead()) {
        float damage =
            (itemName == "Knife" || itemName == "Stone Knife") ? 50.0f : 10.0f;
        std::cout << "[Settler] HIT FRAME: Hitting animal for " << damage
                  << " dmg." << std::endl;
        m_currentTargetAnimal->takeDamage(damage);
      }
      m_waitingToDealDamage = false;
    }
  }

  // Update combat timers
  if (m_shootCooldownTimer > 0.0f) {
    m_shootCooldownTimer -= deltaTime;
  }

  // Smooth ADS transition
  float lerpSpeed = 10.0f;
  if (m_isScoping) {
    m_adsLerp += deltaTime * lerpSpeed;
    if (m_adsLerp > 1.0f)
      m_adsLerp = 1.0f;
  } else {
    m_adsLerp -= deltaTime * lerpSpeed;
    if (m_adsLerp < 0.0f)
      m_adsLerp = 0.0f;
  }

  // Default action update logic (rest of the file follows)
  // Skip AI logic if player-controlled
  if (m_isPlayerControlled) {
    // Force IDLE state to prevent AI actions from rendering (e.g. MINING)
    // only if we are not explicitly moving/waiting from player input
    if (m_state != SettlerState::MOVING && m_state != SettlerState::WAITING) {
      m_state = SettlerState::IDLE;
    }
    return; // Player handles movement/actions
  }

  m_stats.update(deltaTime);

  // Aktualizacja cooldownów snu i jedzenia
  if (m_sleepCooldownTimer > 0.0f)
    m_sleepCooldownTimer -= deltaTime;
  if (m_eatingCooldownTimer > 0.0f)
    m_eatingCooldownTimer -= deltaTime;

  if (CheckForJobFlagActivation()) {
    if (IsStateInterruptible()) {
      InterruptCurrentAction();
    } else {
      m_pendingReevaluation = true;
    }
  }
  bool isCriticalTask = (m_state == SettlerState::EATING) ||
                        (m_state == SettlerState::SLEEPING) ||
                        (m_state == SettlerState::SEARCHING_FOR_FOOD) ||
                        (m_state == SettlerState::MOVING_TO_BED) ||
                        (m_state == SettlerState::MOVING_TO_FOOD);

  if (!isCriticalTask && m_sleepCooldownTimer <= 0.0f &&
      m_stats.getCurrentEnergy() <= m_sleepEnterThreshold) {
    if (m_assignedBed) {
      m_state = SettlerState::MOVING_TO_BED;
      m_isMovingToCriticalTarget = true;
      MoveTo(m_assignedBed->getPosition());
    }
    // Jeśli nie ma przypisanego łóżka, nie przechodzimy w stan SLEEPING
    // (pozostajemy w bieżącym stanie)
  }
  if (!isCriticalTask && m_eatingCooldownTimer <= 0.0f &&
      m_state != SettlerState::SLEEPING &&
      m_state != SettlerState::MOVING_TO_BED &&
      m_stats.getCurrentHunger() <= m_hungerEnterThreshold) {
    m_state = SettlerState::SEARCHING_FOR_FOOD;
    m_isMovingToCriticalTarget = true;
    m_currentPath.clear();
  }

  switch (m_state) {
  case SettlerState::SEARCHING_FOR_FOOD:
    UpdateSearchingForFood(deltaTime, bushes);
    break;
  case SettlerState::MOVING_TO_FOOD:
    UpdateMovingToFood(deltaTime);
    break;
  case SettlerState::EATING:
    UpdateEating(deltaTime);
    break;
  case SettlerState::HAULING:
    UpdateHauling(deltaTime, buildings, worldItems);
    break;
  case SettlerState::HUNTING:
    UpdateHunting(deltaTime, animals, buildings, resourceNodes);
    break;
  case SettlerState::SKINNING:
    UpdateSkinning(deltaTime);
    break;
  case SettlerState::MOVING_TO_SKIN:
    UpdateMovingToSkin(deltaTime, buildings, resourceNodes);
    break;
  case SettlerState::GATHERING:
    UpdateGathering(deltaTime, worldItems, buildings);
    break;
  case SettlerState::CHOPPING:
    UpdateChopping(deltaTime);
    break;
  case SettlerState::MINING:
    UpdateMining(deltaTime);
    break;
  case SettlerState::MOVING_TO_BED:
    UpdateMovingToBed(deltaTime);
    break;
  case SettlerState::IDLE: {
    // 0. PRIORITY: Process Action Queue actions FIRST
    if (!m_actionQueue.empty()) {
      ExecuteNextAction();
      return;
    }

    // PRIORITY: If we have resources, check if ANY nearby blueprint needs them
    if (g_buildingSystem && !m_inventory.isEmpty()) {
      auto activeTasks = g_buildingSystem->getActiveBuildTasks();
      float bestDist = 30.0f; // Scan range
      BuildTask *targetTask = nullptr;

      // 1. Check private house first
      if (m_isIndependentBuilder && m_myPrivateBuildTask &&
          m_myPrivateBuildTask->isActive()) {
        auto missing = m_myPrivateBuildTask->getMissingResources();
        for (const auto &req : missing) {
          if (m_inventory.getResourceAmount(req.resourceType) > 0) {
            targetTask = m_myPrivateBuildTask;
            break;
          }
        }
      }

      // 2. Check nearest other blueprint
      if (!targetTask) {
        for (auto *task : activeTasks) {
          float d = Vector3Distance(position, task->getPosition());
          if (d < bestDist) {
            auto missing = task->getMissingResources();
            for (const auto &req : missing) {
              if (m_inventory.getResourceAmount(req.resourceType) > 0) {
                bestDist = d;
                targetTask = task;
                break;
              }
            }
          }
        }
      }

      if (targetTask) {
        std::cout << "[Settler] IDLE: Found nearby building task needing my "
                     "resources. Delivering to "
                  << targetTask->getBlueprint()->getName() << std::endl;
        targetTask->addWorker(this); // Register as worker
        assignBuildTask(targetTask);
        ProcessActiveBuildTask(deltaTime, buildings);
        return;
      }
    }

    // INDEPENDENT BUILDER LOGIC - PRIORITY CHECK
    if (m_isIndependentBuilder && m_myPrivateBuildTask && !m_currentBuildTask) {
      // Auto-assign private task
      if (m_myPrivateBuildTask->isActive()) {
        assignBuildTask(m_myPrivateBuildTask);
      }
    }

    // INDEPENDENT BUILDER LOGIC - HOUSE SEARCH
    if (m_isIndependentBuilder && !hasHouse) {
      if (!m_myPrivateBuildTask) {
        // Phase 1: Search for a build site
        if (g_buildingSystem) {
          std::string bpId = "house_4";
          if (preferredHouseSize > 4 && preferredHouseSize <= 6)
            bpId = "house_6";
          else if (preferredHouseSize > 6)
            bpId = "house_9";

          // We need a helper to find build position since it's in ColonyAI.
          // For now, let's use a simplified version or assume ColonyAI provides
          // findBuildPosition. Actually, findBuildPosition is public in
          // ColonyAI. But we can also just try some positions around the
          // current one.
          for (int i = 0; i < 10; ++i) {
            float angle = (float)rand() / (float)RAND_MAX * 2.0f * PI;
            float dist = 20.0f + (float)rand() / (float)RAND_MAX * 20.0f;
            Vector3 testPos = {position.x + cosf(angle) * dist, 0.0f,
                               position.z + sinf(angle) * dist};

            if (g_buildingSystem->canBuild(bpId, testPos)) {
              bool success = false;
              m_myPrivateBuildTask = g_buildingSystem->startBuilding(
                  bpId, testPos, this, 0.0f, false, false, false, &success);
              if (success && m_myPrivateBuildTask) {
                std::cout
                    << "[Settler] Independent: Started private build task for "
                    << bpId << " at (" << testPos.x << ", " << testPos.z << ")"
                    << std::endl;
                break;
              }
            }
          }
        }
      } else if (m_myPrivateBuildTask->getState() == BuildState::COMPLETED) {
        // Task completed!
        hasHouse = true;
        m_myPrivateBuildTask = nullptr;
        std::cout << "[Settler] Independent: House completed! I am no longer "
                     "homeless."
                  << std::endl;
      } else if (m_myPrivateBuildTask->getState() == BuildState::CANCELLED) {
        m_myPrivateBuildTask = nullptr;
      }
    }

    // 2. Process Current Build Task (if assigned)
    if (m_currentBuildTask && m_currentBuildTask->isActive()) {
      // Check if we have missing resources
      if (!m_currentBuildTask->hasAllResources()) {
        auto missing = m_currentBuildTask->getMissingResources();
        if (!missing.empty()) {
          std::string reqResource = missing[0].resourceType;
          int reqAmount = missing[0].amount;
          int hasAmount = m_inventory.getResourceAmount(reqResource);

          if (hasAmount >=
              std::min(reqAmount, 5)) { // Carry at least 5 or needed
            // Deliver
            ProcessActiveBuildTask(deltaTime, buildings);
            return;
          } else {
            // Fetch Resource (Logic similar to Crafting fetch)
            std::cout << "[Settler] BuildTask needs " << reqResource
                      << ". Fetching." << std::endl;

            if (reqResource == "Wood") {
              float minDist = 9999.0f;
              Tree *nearest = nullptr;
              for (const auto &t : trees) {
                if (t->isActive() && !t->isReserved() && !t->isStump()) {
                  float d = Vector3Distance(position, t->getPosition());
                  if (d < minDist) {
                    minDist = d;
                    nearest = t.get();
                  }
                }
              }
              if (nearest) {
                std::cout << "[Settler] Found available tree at distance "
                          << minDist << ". Assigning to chop." << std::endl;
                nearest->reserve(m_name);
                assignToChop(nearest);
                return;
              } else {
                std::cout
                    << "[Settler] Failed to find ANY available tree for wood!"
                    << std::endl;
              }
            } else if (reqResource == "Stone") {
              float minDist = 9999.0f;
              ResourceNode *nearest = nullptr;
              for (const auto &n : resourceNodes) {
                if (n->isActive() && !n->isReserved() &&
                    n->getResourceType() == Resources::ResourceType::Stone) {
                  float d = Vector3Distance(position, n->getPosition());
                  if (d < minDist) {
                    minDist = d;
                    nearest = n.get();
                  }
                }
              }
              if (nearest) {
                nearest->reserve(m_name);
                assignToMine(nearest);
                return;
              }
            }
          }
        }
      } else {
        // Start/Continue Building
        ProcessActiveBuildTask(deltaTime, buildings);
        return;
      }
    }

    // 3. Find New Tasks (if no current task)
    auto skills = m_skills.getAllSkills();
    std::vector<Skill> sortedSkills;
    for (const auto &pair : skills) {
      sortedSkills.push_back(pair.second);
    }
    std::sort(
        sortedSkills.begin(), sortedSkills.end(),
        [](const Skill &a, const Skill &b) { return a.priority > b.priority; });

    bool taskFound = false;
    for (const auto &skill : sortedSkills) {
      if (skill.type == SkillType::BUILDING && performBuilding) {
        if (g_buildingSystem) {
          auto buildTasks = g_buildingSystem->getActiveBuildTasks();
          if (!buildTasks.empty()) {
            float minDist = 1000.0f;
            BuildTask *bestTask = nullptr;

            // Strategy: Prefer tasks created by ME, then tasks with NO workers
            for (auto *task : buildTasks) {
              if (task->getBuilder() == this) {
                // My task! Priority #1
                bestTask = task;
                break; // Immediate take
              }

              if (task->getWorkerCount() == 0) {
                float d = Vector3Distance(position, task->getPosition());
                if (d < minDist) {
                  minDist = d;
                  bestTask = task;
                }
              }
            }

            if (bestTask) {
              assignBuildTask(bestTask);
              taskFound = true;
            }
          }
        }
      } else if (skill.type == SkillType::WOODCUTTING && gatherWood) {
        float minDist = 100.0f;
        Tree *nearest = nullptr;
        for (const auto &treePtr : trees) {
          Tree *tree = treePtr.get();
          if (tree && tree->isActive() && !tree->isReserved() &&
              !tree->isStump()) {
            float d = Vector3Distance(position, tree->getPosition());
            if (d < minDist) {
              minDist = d;
              nearest = tree;
            }
          }
        }
        if (nearest) {
          nearest->reserve(m_name);
          assignToChop(nearest);
          taskFound = true;
        }
      } else if (skill.type == SkillType::MINING && gatherStone) {
        float minDist = 5000.0f; // Zwiększony zasięg szukania kamienia
        ResourceNode *nearest = nullptr;
        for (const auto &node : resourceNodes) {
          if (node->isActive() && !node->isReserved()) {
            float d = Vector3Distance(position, node->getPosition());
            // std::cout << "DEBUG Stone: Node " << d << "m away. Active: " <<
            // node->isActive() << std::endl;
            if (d < minDist) {
              minDist = d;
              nearest = node.get();
            }
          }
        }
        if (nearest) {
          nearest->reserve(m_name);
          assignToMine(nearest);
          taskFound = true;
        }
      } else if (skill.type == SkillType::FARMING && gatherFood) {
        float minDist = 100.0f;
        Bush *nearest = nullptr;
        for (const auto &bush : bushes) {
          if (bush && bush->hasFruit) {
            float d = Vector3Distance(position, bush->position);
            if (d < minDist) {
              minDist = d;
              nearest = bush;
            }
          }
        }
        if (nearest) {
          m_currentGatherBush = nearest;
          MoveTo(nearest->position);
          m_state = SettlerState::GATHERING;
          m_gatherTimer = 0.0f;
          taskFound = true;
        }
      } else if (huntAnimals) {
        float minDist = 200.0f;
        Animal *nearest = nullptr;
        for (const auto &animPtr : animals) {
          if (animPtr && animPtr->isActive() && !animPtr->isDead()) {
            float d = Vector3Distance(position, animPtr->getPosition());
            if (d < minDist) {
              minDist = d;
              nearest = animPtr.get();
            }
          }
        }
        if (nearest) {
          m_currentTargetAnimal = nearest;
          m_state = SettlerState::HUNTING;
          m_huntingTimer = 0.0f;
          taskFound = true;
        }
      }
      // CRAFTING LOGIC (Simplified: Job Acquisition Only)
      else if (craftItems) {
        if (m_currentCraftTaskId == -1) {
          auto craftingSys =
              GameEngine::getInstance().getSystem<CraftingSystem>();
          if (craftingSys) {
            CraftingTask *task = craftingSys->getAvailableTask(m_name);
            if (task) {
              std::cout << "[Settler] Acquired Crafting Task " << task->taskId
                        << std::endl;
              m_currentCraftTaskId = task->taskId;
              taskFound = true;
            }
          }
        }
      }

      if (taskFound)
        break;
    }
  } break;
  case SettlerState::MOVING:
    UpdateMovement(deltaTime, trees, buildings, resourceNodes);
    // Continuous check for build range while moving
    if (m_currentBuildTask && m_currentBuildTask->isActive()) {
      ProcessActiveBuildTask(deltaTime, buildings);
    }
    break;
  case SettlerState::MOVING_TO_STORAGE:
    UpdateMovingToStorage(deltaTime, buildings, resourceNodes);
    break;
  case SettlerState::DEPOSITING:
    UpdateDepositing(deltaTime, buildings);
    break;
  case SettlerState::BUILDING:
    UpdateBuilding(deltaTime);
    break;
  case SettlerState::SLEEPING:
    UpdateSleeping(deltaTime);
    break;
  case SettlerState::PICKING_UP:
    UpdatePickingUp(deltaTime, worldItems, buildings);
    break;
  case SettlerState::CRAFTING:
    UpdateCrafting(deltaTime);
    break;
  case SettlerState::WAITING:
    m_gatherTimer -= deltaTime;
    if (m_gatherTimer <= 0.0f) {
      std::cout << "[Settler] Koniec oczekiwania. Ponawiam probe." << std::endl;
      m_state = SettlerState::IDLE;
      m_gatherTimer = 0.0f;
    }
    break;
  default:
    break;
  }
  auto posComp = getComponent<PositionComponent>();
  if (posComp)
    posComp->setPosition(position);
}

void Settler::render() {
  bool usingTool =
      (m_state == SettlerState::CHOPPING || m_state == SettlerState::MINING);
  // NEW RENDERER
  Color color = m_isSelected ? YELLOW : BLUE;
  Color skinColor = {255, 220, 177, 255}; // Light skin
  Color shirtColor = color;               // Profession color
  Color pantsColor = DARKBLUE;

  float animSpeed = 6.5f; // Reduced from 10.0f for smoother movement
  float limbSwing = 0.0f;
  float torsoBounce = 0.0f;
  float headBob = 0.0f;
  float currentTime = (float)GetTime();
  float breathing =
      sinf(currentTime * 1.5f) * 0.015f; // Slower, more subtle breathing

  if (m_state == SettlerState::MOVING ||
      m_state == SettlerState::MOVING_TO_STORAGE ||
      m_state == SettlerState::MOVING_TO_FOOD ||
      m_state == SettlerState::MOVING_TO_BED ||
      m_state == SettlerState::GATHERING || m_state == SettlerState::HAULING ||
      m_state == SettlerState::HUNTING ||
      m_state == SettlerState::MOVING_TO_SKIN) {
    limbSwing = sinf(currentTime * animSpeed) * 0.2f;
    // Use sin directly for torsoBounce to avoid "jerkiness" from fabsf
    torsoBounce = (sinf(currentTime * animSpeed) * 0.5f + 0.5f) * 0.07f;
    headBob = sinf(currentTime * animSpeed) * 0.02f;
  }

  // DEBUG: Draw line to target animal
  if (m_state == SettlerState::HUNTING && m_currentTargetAnimal) {
    Vector3 targetPos = m_currentTargetAnimal->getPosition();
    DrawLine3D(position, targetPos, RED);
    DrawSphere(targetPos, 0.2f, RED);

    // Debug Text - Console only (Camera not accessible here)
  }

  // Matrix Transformation for proper rotation
  rlPushMatrix();
  rlTranslatef(position.x, position.y, position.z);
  rlRotatef(m_rotation, 0.0f, 1.0f, 0.0f);

  // Draw relative to pivot (Feet at 0,0,0)
  // Lift body center by 0.5f
  float bodyY = 0.5f + torsoBounce + breathing;

  // Torso (at bodyY + 0.4)
  DrawCube({0, bodyY + 0.4f, 0}, 0.4f, 0.5f, 0.25f, shirtColor);

  // Head (at bodyY + 0.8 + headBob)
  Vector3 headPos = {0, bodyY + 0.82f + headBob, 0.02f};
  DrawCube(headPos, 0.25f, 0.25f, 0.25f, skinColor);

  // Eyes (blinking)
  bool isBlinking = (sinf(currentTime * 1.5f) > 0.95f);
  if (!isBlinking) {
    DrawCube({headPos.x - 0.07f, headPos.y + 0.05f, headPos.z + 0.12f}, 0.04f,
             0.04f, 0.02f, BLACK);
    DrawCube({headPos.x + 0.07f, headPos.y + 0.05f, headPos.z + 0.12f}, 0.04f,
             0.04f, 0.02f, BLACK);
  }

  // Backpack (if inventory is not empty)
  if (!m_inventory.isEmpty()) {
    DrawCube({0, bodyY + 0.4f, -0.18f}, 0.35f, 0.4f, 0.15f, DARKBROWN);
  }

  // Legs - freeze only when hunting and not moving (aiming)
  if (m_state == SettlerState::HUNTING && !isMoving()) {
    limbSwing = 0.0f;
  }
  DrawCube({-0.12f, bodyY - 0.2f, limbSwing}, 0.15f, 0.5f, 0.15f, pantsColor);
  DrawCube({0.12f, bodyY - 0.2f, -limbSwing}, 0.15f, 0.5f, 0.15f, pantsColor);

  // WEAPON CHECK FOR VISUALS
  bool hasSniperRifle = false;
  if (m_heldItem && m_heldItem->getDisplayName() == "Sniper Rifle") {
    hasSniperRifle = true;
  }

  // Precise Aiming Check: Must be Hunting, have Rifle, and Timer running
  // (meaning we stopped moving) OR Player Controlled + Ranged weapon
  bool isAiming = false;
  if (hasSniperRifle) {
    if (m_isPlayerControlled ||
        (m_state == SettlerState::HUNTING && m_huntingTimer > 0.05f)) {
      isAiming = true;
      limbSwing = 0.0f; // Freeze legs while aiming
    }
  }

  // Arms

  // Check if carrying Meat
  bool carryingMeat = false;
  for (const auto &slot : m_inventory.getItems()) {
    if (slot && slot->item && slot->item->getDisplayName() == "Raw Meat") {
      carryingMeat = true;
      break;
    }
  }

  // FPS VIEW: Raise arm if player controlled and has axe
  bool isHoldingAxe =
      (m_heldItem && m_heldItem->getDisplayName() == "Stone Axe");

  // Lewa ręka
  rlPushMatrix();
  // Pivot w barku (lewy)
  rlTranslatef(-0.3f, bodyY + 0.6f, 0.0f);

  float leftArmAngle = -limbSwing * 20.0f;

  if (isAiming) {
    // SNIPER STANCE: Left hand supports the barrel
    leftArmAngle = -90.0f;               // Raise horizontal
    rlRotatef(25.0f, 0.0f, 1.0f, 0.0f);  // Angle inward to support barrel
    rlRotatef(-10.0f, 0.0f, 0.0f, 1.0f); // Tilt slightly
  } else if (carryingMeat) {
    leftArmAngle = -30.0f + (sinf(currentTime * 10.0f) * 5.0f);
  }

  rlRotatef(leftArmAngle, 1.0f, 0.0f, 0.0f);

  // Rysowanie ręki
  DrawCube({0.0f, -0.2f, 0.0f}, 0.12f, 0.4f, 0.12f, skinColor);

  // Rysowanie mięsa w dłoni
  if (carryingMeat) {
    DrawCube({0.0f, -0.45f, 0.1f}, 0.25f, 0.25f, 0.25f, RED);
    DrawCubeWires({0.0f, -0.45f, 0.1f}, 0.25f, 0.25f, 0.25f, MAROON);
  }

  rlPopMatrix(); // Pop left arm matrix

  // Prawa ręka - ZŁOŻONA ANIMACJA 5 FAZ (Pivot w barku)
  rlPushMatrix();
  // 1. Pivot point (bark) - PODNIESIONY do 0.6f
  rlTranslatef(0.3f, bodyY + 0.6f, 0.0f);

  float armAngle = limbSwing * 20.0f;

  if (m_attackAnimTimer > 0.0f) {
    // SWING ANIMATION PRIORITY
    // 5 FAZ ANIMACJI (Timer 1.0 -> 0.0)
    if (m_attackAnimTimer > 0.8f) {
      // FAZA 1: Windup (1.0-0.8) - Unoszenie do tyłu
      float t = (1.0f - m_attackAnimTimer) / 0.2f; // 0->1
      armAngle = -45.0f * t;
    } else if (m_attackAnimTimer > 0.6f) {
      // FAZA 2: Hold (0.8-0.6) - Przytrzymanie w górze
      armAngle = -45.0f;
    } else if (m_attackAnimTimer > 0.4f) {
      // FAZA 3: Strike (0.6-0.4) - Szybkie uderzenie w przód
      float t = (0.8f - m_attackAnimTimer) / 0.2f; // 0->1
      // Interpolacja od -45 do +90
      armAngle = -45.0f + (135.0f * t);
    } else if (m_attackAnimTimer > 0.2f) {
      // FAZA 4: Follow though (0.4-0.2) - Powolny powrót z wychylenia
      float t = (0.4f - m_attackAnimTimer) / 0.2f; // 0->1
      armAngle = 90.0f - (90.0f * t);
    } else {
      // FAZA 5: Reset (0.2-0.0)
      armAngle = 0.0f;
    }
  } else if (isAiming) {
    armAngle = -90.0f;
    rlRotatef(-15.0f, 0.0f, 1.0f, 0.0f); // Angle inward
  } else if (isHoldingAxe && m_isPlayerControlled) {
    armAngle = -85.0f;                   // Ready pose
    rlRotatef(-15.0f, 0.0f, 1.0f, 0.0f); // Angle inward
  }

  rlRotatef(armAngle, 1.0f, 0.0f, 0.0f); // Obrót wokół osi X

  // Rysowanie ręki
  DrawCube({0.0f, -0.2f, 0.0f}, 0.12f, 0.4f, 0.12f, skinColor);

  // Axe / Sniper in RIGHT hand
  if (isHoldingAxe) {
    rlPushMatrix();
    rlTranslatef(0.0f, -0.45f, 0.1f);
    // Fixed: Rotation so the blade faces forward
    rlRotatef(45.0f, 0.0f, 1.0f, 0.0f);
    rlRotatef(90.0f, 1.0f, 0.0f, 0.0f); // Make it horizontal in hand

    DrawCube({0, 0, 0.3f}, 0.05f, 0.05f, 0.6f, BROWN);          // Handle
    DrawCube({0, 0.12f, 0.55f}, 0.05f, 0.25f, 0.15f, DARKGRAY); // Blade
    rlPopMatrix();
  } else if (m_heldItem) {
    rlPushMatrix();
    rlTranslatef(0.0f, -0.45f, 0.2f); // Attach to hand, slightly forward

    std::string hName = m_heldItem->getDisplayName();

    if (hName == "Sniper Rifle") {
      rlRotatef(90.0f, 1.0f, 0.0f, 0.0f); // Align with arm direction
      // Barrel
      DrawCube({0, 0, 0.4f}, 0.05f, 0.05f, 0.8f, BLACK);
      // Stock
      DrawCube({0, -0.1f, -0.2f}, 0.08f, 0.15f, 0.4f, BROWN);
      // Scope
      DrawCube({0, 0.08f, 0.1f}, 0.06f, 0.06f, 0.2f, DARKGRAY);
    } else {
      // Generic Item visual
      DrawCube({0, 0, 0}, 0.1f, 0.1f, 0.1f, WHITE);
    }
    rlPopMatrix();
  } else if (usingTool) {
    // Render Tool (Pickaxe or ephemeral Axe if not held)
    rlPushMatrix();
    rlTranslatef(0.0f, -0.45f, 0.23f);
    rlRotatef(90.0f, 1.0f, 0.0f, 0.0f);

    if (m_state == SettlerState::CHOPPING) {
      // Axe Visual (Ephemeral)
      DrawCube({0, 0, 0.2f}, 0.05f, 0.05f, 0.6f, BROWN);
      DrawCube({0, 0.05f, 0.45f}, 0.05f, 0.25f, 0.15f, DARKGRAY);
    } else if (m_state == SettlerState::MINING) {
      // Pickaxe
      DrawCube({0, 0, 0}, 0.05f, 0.05f, 0.4f, BROWN);        // Handle
      DrawCube({0, 0.05f, 0.15f}, 0.3f, 0.05f, 0.05f, GRAY); // Pick head
    }
    rlPopMatrix();
  }

  rlPopMatrix(); // Pop right arm matrix

  // Carry visual (Wood)
  const auto &items = m_inventory.getItems();
  bool hasWood = false;
  for (const auto &invItem : items) {
    if (invItem && invItem->item &&
        invItem->item->getItemType() == ItemType::RESOURCE) {
      ResourceItem *resItem = dynamic_cast<ResourceItem *>(invItem->item.get());
      if (resItem && resItem->getResourceType() == "Wood") {
        hasWood = true;
        break;
      }
    }
  }
  if (hasWood && m_state != SettlerState::MINING &&
      m_state != SettlerState::CHOPPING) {
    DrawCube({0, bodyY + 0.7f, 0}, 0.6f, 0.2f, 0.2f, BROWN); // On shoulder/head
  }
  rlPopMatrix(); // End rotation context

  // Progress Bar (Draw in World Space to face camera properly if billboarded,
  // or if we want it to rotate with settler, we could keep it inside.
  // Progress Bars usually float above.
  // Current implementation DrawProgressBar3D determines orientation?
  // Let's draw it in World Space for safety (outside matrix).
  // Progress Bar (Fill based on depletion)
  if (m_state == SettlerState::CHOPPING && m_currentTree &&
      m_currentTree->isActive()) {
    float progress = 1.0f - (m_currentTree->getWoodAmount() /
                             m_currentTree->getMaxWoodAmount());
    DrawProgressBar3D(position, progress, YELLOW);
  } else if (m_state == SettlerState::MINING && m_currentResourceNode &&
             m_currentResourceNode->isActive()) {
    // Prevent division by zero
    float maxAmount = (float)m_currentResourceNode->getMaxAmount();
    if (maxAmount > 0.0f) {
      float progress =
          1.0f - ((float)m_currentResourceNode->getCurrentAmount() / maxAmount);
      DrawProgressBar3D(position, progress, GRAY);
    }
  } else if (m_state == SettlerState::CRAFTING) {
    DrawProgressBar3D(position, m_craftingTimer / 5.0f, GREEN);
  } else if (m_state == SettlerState::SKINNING) {
    DrawProgressBar3D(position, m_skinningTimer / 2.0f, RED);
  }
}

// --- BUILD PROCESS HELPER ---
void Settler::ProcessActiveBuildTask(
    float deltaTime, const std::vector<BuildingInstance *> &buildings) {
  // 1. Validation
  if (!m_currentBuildTask || !m_currentBuildTask->isActive()) {
    std::cout << "[Settler] Build task invalid or finished. Clearing."
              << std::endl;
    clearBuildTask();
    m_state = SettlerState::IDLE;
    return;
  }

  // 1.5 Get Bounding Box early
  BoundingBox buildBox = m_currentBuildTask->getBoundingBox();

  // 2. Check Resources
  if (!m_currentBuildTask->hasAllResources()) {
    auto missing = m_currentBuildTask->getMissingResources();
    if (!missing.empty()) {
      ResourceRequirement req = missing[0]; // Describe first missing resource

      // Check inventory
      int currentAmount = m_inventory.getResourceAmount(req.resourceType);

      if (currentAmount > 0) {
        // Have some resource -> Deliver it
        // Expanded interaction range for delivery
        bool isInRange = CheckCollisionBoxSphere(buildBox, position, 4.5f);

        if (!isInRange) {
          MoveTo(m_currentBuildTask->getPosition());
          // m_state = SettlerState::MOVING;
        } else {
          // Add to task
          Stop(); // Stop moving if we were moving
          int amountToAdd = std::min(currentAmount, req.amount);

          std::cout << "[Settler] Delivering " << amountToAdd << " "
                    << req.resourceType << " to build site at "
                    << m_currentBuildTask->getPosition().x << ", "
                    << m_currentBuildTask->getPosition().z << std::endl;

          m_currentBuildTask->addResource(req.resourceType, amountToAdd);
          m_inventory.removeResource(req.resourceType, amountToAdd);
        }
      } else {
        // Don't have resource -> Gather it
        std::cout << "[Settler] Missing " << req.amount << " "
                  << req.resourceType << " for build. Switching to GATHER."
                  << std::endl;

        // Clear build task temporarily or just switch state?
        // We keep the assignment but switch state to seek resource.
        // WE MUST NOT clear m_currentBuildTask because we want to return to it.
        // However, our state machine might be simple.
        // Let's use IDLE logic to find resource but we need to know what to
        // look for. ACTUALLY: Let's create a GATHER task explicitly here.

        // Queue GATHER task
        if (req.resourceType == "Wood") {
          m_prevGatherWood =
              true; // Enable flag temporarily? No, better to force action.

          // Simple search (similar to IDLE logic but immediate)
          // We need access to trees... tricky from here without passing 'trees'
          // vector. But we can switch to IDLE and set a high priority flag? Or
          // we just rely on the fact that we are in IDLE usually when calling
          // this? ProcessActiveBuildTask is called from Update?

          // IF we are here, we are likely in IDLE or BUILDING state.
          // We can't easily search trees without the vector.
          // Revert state to IDLE and set 'gatherWood' flag?
          // But that changes global settings.

          // Alternative: Set a temporary "fetch mode".
          // For now, let's just Log and switch to IDLE, letting the NEW IDLE
          // logic handle checking 'm_currentBuildTask' needs. Wait, the IDLE
          // logic needs to be updated to check m_currentBuildTask requirements.

          // Let's Return "Need Resource" signal?
          // No, let's just set state to IDLE.
          // The IDLE loop (which has access to trees etc) will see we have a
          // BuildTask and check its requirements, then search for resources.

          m_state = SettlerState::IDLE;
        } else if (req.resourceType == "Stone") {
          m_state = SettlerState::IDLE;
        }
      }
    }
    return; // Don't build yet
  }

  // 3. Move to Site
  // Expanded interaction range for building to avoid getting stuck inside
  bool isInRange = CheckCollisionBoxSphere(buildBox, position, 5.0f);

  if (!isInRange) {
    // Only set movement target if we're actually far away
    MoveTo(m_currentBuildTask->getPosition());
    // m_state = SettlerState::MOVING; // logic inside MoveTo sets this (if path
    // found)
  } else {
    // Already in range - DO NOT call MoveTo to avoid pathfinding into building
    // center
    Stop();      // Ensure we stop moving
    clearPath(); // Clear any existing path

    // 4. Build
    m_state = SettlerState::BUILDING;
  }
}

InteractionResult Settler::interact(GameEntity *player) {

  (void)player;

  InteractionResult result;

  result.success = true;

  result.message = "Rozmowa z " + m_name;

  return result;
}
InteractionInfo Settler::getDisplayInfo() const {

  InteractionInfo info;

  info.objectName = m_name;

  info.objectDescription = "Osadnik";

  return info;
}
std::string Settler::GetStateString() const {
  switch (m_state) {

  case SettlerState::IDLE:
    return "Bezczynny";

  case SettlerState::MOVING:
    return "Idzie";

  case SettlerState::GATHERING:
    return "Zbiera";

  case SettlerState::CHOPPING:
    return "Rąbie";

  case SettlerState::MINING:
    return "Wydobywa";

  case SettlerState::BUILDING:
    return "Buduje";

  case SettlerState::SLEEPING:
    return "Śpi";

  case SettlerState::HUNTING:
    return "Poluje";

  case SettlerState::HAULING:
    return "Transportuje";

  case SettlerState::MOVING_TO_STORAGE:
    return "Idzie do magazynu";

  case SettlerState::DEPOSITING:
    return "Odkłada";

  case SettlerState::SEARCHING_FOR_FOOD:
    return "Szuka jedzenia";

  case SettlerState::MOVING_TO_FOOD:
    return "Idzie do jedzenia";

  case SettlerState::EATING:
    return "Je";

  case SettlerState::MOVING_TO_BED:
    return "Idzie spać";

  case SettlerState::PICKING_UP:
    return "Podnosi";

  case SettlerState::WANDER:
    return "Wędruje";

  case SettlerState::WAITING:
    return "Czeka";

  case SettlerState::CRAFTING:
    return "Tworzy";

  case SettlerState::SKINNING:
    return "Skoruje";

  case SettlerState::MOVING_TO_SKIN:
    return "Idzie do ciala";

  default:
    return "Nieznany";
  }

} // koniec GetStateString

std::string Settler::GetProfessionString() const {
  switch (m_profession) {
  case SettlerProfession::NONE:
    return "Brak";
  case SettlerProfession::BUILDER:
    return "Budowniczy";
  case SettlerProfession::GATHERER:
    return "Zbieracz";
  case SettlerProfession::HUNTER:
    return "Łowca";
  case SettlerProfession::CRAFTER:
    return "Rzemieślnik";
  default:
    return "Nieznany";
  }
}

void Settler::assignTask(TaskType type, GameEntity *target, Vector3 pos) {
  Action action;
  action.type = type;
  action.targetEntity = target;
  action.targetPosition = pos;
  m_actionQueue.push_back(action);
}

void Settler::clearTasks() {
  m_actionQueue.clear();
  m_state = SettlerState::IDLE;
}
void Settler::MoveTo(Vector3 destination) {
  // Sprawdź, czy cel się zmienił (z tolerancją) i czy mamy już ważną ścieżkę
  float distToLast = Vector3Distance(destination, m_lastPathTarget);
  if (distToLast < 0.1f && m_lastPathValid && !m_currentPath.empty()) {
    // Cel praktycznie ten sam, ścieżka już obliczona - użyj istniejącej
    std::cout
        << "[Settler] MoveTo: cel niezmieniony, używam istniejącej ścieżki."
        << std::endl;
  } else {
    // Oblicz nową ścieżkę za pomocą NavigationGrid
    NavigationGrid *grid = GameSystem::getNavigationGrid();
    if (grid) {
      std::vector<Vector3> path = grid->FindPath(position, destination);
      if (!path.empty()) {
        setPath(path);
        m_lastPathTarget = destination;
        m_lastPathValid = true;

        // IMMEDIATE FEEDBACK: Snap rotation to first step if possible
        if (path.size() > 1) {
          Vector3 dir = Vector3Subtract(path[1], position);
          m_rotation = atan2(dir.x, dir.z) * RAD2DEG;
        }

        std::cout << "[Settler] MoveTo: wyznaczono ścieżkę o długości "
                  << path.size() << " do (" << destination.x << ","
                  << destination.y << "," << destination.z << ")" << std::endl;
      } else {
        // Brak ścieżki (może cel nieosiągalny)
        float dist = Vector3Distance(position, destination);
        if (dist > 2.0f) {
          std::cout << "[Settler] MoveTo: brak ścieżki i cel daleko (" << dist
                    << "). Próba ruchu bezpośredniego (ryzyko clippingu)."
                    << std::endl;
          clearPath();
          m_lastPathValid = false;
          // NIE return - pozwól na próbę ruchu bezpośredniego, żeby settler
          // mógł się zbliżyć W najgorszym przypadku zadziała collision
          // detection
        } else {
          // Mały dystans - doprecyzowanie pozycji
          std::cout << "[Settler] MoveTo: brak ścieżki ale cel blisko. Ruch "
                       "bezpośredni."
                    << std::endl;
          clearPath();
          m_lastPathValid = false;
        }
      }
    } else {
      std::cout << "[Settler] MoveTo: brak NavigationGrid, ruch bezpośredni."
                << std::endl;
      clearPath();
      m_lastPathValid = false;
    }
  }

  m_targetPosition = destination;
  // Nie zmieniaj stanu jeśli już jest w stanie krytycznym (MOVING_TO_BED,
  // MOVING_TO_FOOD, MOVING_TO_STORAGE)
  if (m_state != SettlerState::MOVING_TO_BED &&
      m_state != SettlerState::MOVING_TO_FOOD &&
      m_state != SettlerState::MOVING_TO_STORAGE) {
    m_state = SettlerState::MOVING;
  }
}
void Settler::Stop() {

  m_state = SettlerState::IDLE;

  m_currentPath.clear();
}
void Settler::setPath(const std::vector<Vector3> &newPath) {

  m_currentPath = newPath;

  m_currentPathIndex = 0;
}
SavedTask Settler::serializeCurrentTask() const {

  SavedTask saved;

  saved.type = TaskType::WAIT;

  saved.targetPosition = m_targetPosition;

  saved.targetEntityId = -1;

  saved.duration = 0.0f;

  saved.hasTarget = false;

  return saved;
}
void Settler::deserializeTask(const SavedTask &savedTask) { (void)savedTask; }
float Settler::getDistanceTo(Vector3 point) const {

  return Vector3Distance(position, point);
}
bool Settler::isAtPosition(Vector3 pos, float tolerance) const {

  return Vector3Distance(position, pos) < tolerance;
}
void Settler::updateNeeds(float deltaTime) { m_stats.update(deltaTime); }
bool Settler::needsFood() const {

  return m_stats.getCurrentHunger() < m_stats.getFoodSearchThreshold();
}
bool Settler::needsSleep() const { return m_stats.isExhausted(); }
void Settler::takeDamage(float damage) {

  float current = m_stats.getCurrentHealth();

  m_stats.setHealth(current - damage);
}
void Settler::assignBed(BuildingInstance *bed) { m_assignedBed = bed; }
void Settler::assignBuildTask(BuildTask *task) {

  m_currentBuildTask = task;

  if (task) {

    MoveTo(task->getPosition());

    m_state = SettlerState::MOVING;
  }
}
void Settler::clearBuildTask() { m_currentBuildTask = nullptr; }
void Settler::assignToChop(GameEntity *tree) {

  if (tree) {

    // Immediate State Update for Persistence checks
    m_currentTree = dynamic_cast<Tree *>(tree);

    clearTasks();

    Action move = Action::Move(tree->getPosition());

    m_actionQueue.push_back(move);

    Action chop;

    chop.type = TaskType::CHOP_TREE;

    chop.targetEntity = tree;

    m_actionQueue.push_back(chop);

    ExecuteNextAction();
  }
}
void Settler::assignToMine(GameEntity *rock) {

  if (rock) {

    clearTasks();

    Action move = Action::Move(rock->getPosition());

    m_actionQueue.push_back(move);

    Action mine;

    mine.type = TaskType::MINE_ROCK;

    mine.targetEntity = rock;

    m_actionQueue.push_back(mine);

    ExecuteNextAction();
  }
}
void Settler::forceGatherTarget(GameEntity *target) {

  if (target) {

    clearTasks();

    Action move = Action::Move(target->getPosition());

    m_actionQueue.push_back(move);

    Action interact;

    interact.targetEntity = target;

    Tree *tree = dynamic_cast<Tree *>(target);

    ResourceNode *rock = dynamic_cast<ResourceNode *>(target);

    if (tree) {

      interact.type = TaskType::CHOP_TREE;

    } else if (rock) {

      interact.type = TaskType::MINE_ROCK;

    } else {

      interact.type = TaskType::GATHER;
    }

    m_actionQueue.push_back(interact);

    ExecuteNextAction();
  }
}
void Settler::ExecuteNextAction() {
  if (m_actionQueue.empty())
    return;

  Action &action = m_actionQueue.front();
  bool shouldPop = true; // Default to pop, override if action needs to persist

  switch (action.type) {
  case TaskType::MOVE:
    MoveTo(action.targetPosition);
    break;

  case TaskType::CHOP_TREE:
    if (action.targetEntity) {
      m_currentTree = dynamic_cast<Tree *>(action.targetEntity);
      if (m_currentTree) {
        float dist = Vector3Distance(position, m_currentTree->getPosition());
        if (dist > 2.0f) {
          MoveTo(m_currentTree->getPosition());
          m_state = SettlerState::MOVING;
          shouldPop = false; // Keep task in queue until we arrive
        } else {
          m_state = SettlerState::CHOPPING;
          m_gatherTimer = 0.0f;
        }
      } else {
        m_state = SettlerState::IDLE;
      }
    }
    break;

  case TaskType::MINE_ROCK:
    if (action.targetEntity) {
      m_currentResourceNode = dynamic_cast<ResourceNode *>(action.targetEntity);
      if (m_currentResourceNode) {
        float dist =
            Vector3Distance(position, m_currentResourceNode->getPosition());
        if (dist > 2.0f) {
          MoveTo(m_currentResourceNode->getPosition());
          m_state = SettlerState::MOVING;
          shouldPop = false; // Keep task in queue until we arrive
        } else {
          m_state = SettlerState::MINING;
          m_gatherTimer = 0.0f;
        }
      } else {
        m_state = SettlerState::IDLE;
      }
    }
    break;

  case TaskType::WAIT:
    m_state = SettlerState::WAITING;
    break;

  case TaskType::PICKUP:
    MoveTo(action.targetPosition);
    m_state = SettlerState::PICKING_UP;
    break;

  case TaskType::GATHER:
    if (action.targetEntity) {
      if (Tree *tree = dynamic_cast<Tree *>(action.targetEntity)) {
        m_currentTree = tree;
        float dist = Vector3Distance(position, tree->getPosition());
        if (dist > 2.0f) {
          MoveTo(tree->getPosition());
          m_state = SettlerState::MOVING;
          shouldPop = false; // Keep task in queue
        } else {
          m_state = SettlerState::CHOPPING;
          m_gatherTimer = 0.0f;
          std::cout << "[Settler] ExecuteNextAction: GATHER -> CHOPPING tree"
                    << std::endl;
        }
      } else if (ResourceNode *rock =
                     dynamic_cast<ResourceNode *>(action.targetEntity)) {
        m_currentResourceNode = rock;
        float dist = Vector3Distance(position, rock->getPosition());
        if (dist > 2.0f) {
          std::cout
              << "[Settler] ExecuteNextAction: GATHER stone, moving (dist="
              << dist << ")" << std::endl;
          MoveTo(rock->getPosition());
          m_state = SettlerState::MOVING;
          shouldPop = false; // Keep task in queue
        } else {
          std::cout << "[Settler] ExecuteNextAction: GATHER stone close, "
                       "starting MINING"
                    << std::endl;
          m_state = SettlerState::MINING;
          m_gatherTimer = 0.0f;
        }
      } else {
        m_state = SettlerState::IDLE;
        std::cout << "[Settler] ExecuteNextAction: GATHER unknown target, idle"
                  << std::endl;
      }
    } else {
      m_state = SettlerState::IDLE;
      std::cout << "[Settler] ExecuteNextAction: GATHER no target, idle"
                << std::endl;
    }
    break;

  default:
    break;
  }

  if (shouldPop) {
    m_actionQueue.pop_front();
  }
}
void Settler::UpdateMovement(
    float deltaTime, const std::vector<std::unique_ptr<Tree>> &trees,
    const std::vector<BuildingInstance *> &buildings,
    const std::vector<std::unique_ptr<ResourceNode>> &resourceNodes) {
  (void)trees;
  (void)buildings;
  // Jeśli mamy ścieżkę, poruszaj się po waypointach
  if (!m_currentPath.empty() &&
      m_currentPathIndex < (int)m_currentPath.size()) {
    Vector3 waypoint = m_currentPath[m_currentPathIndex];
    Vector3 direction = Vector3Subtract(waypoint, position);
    float distance = Vector3Length(direction);
    if (distance < 0.3f) { // osiągnięto waypoint
      m_currentPathIndex++;
      if (m_currentPathIndex >= (int)m_currentPath.size()) {
        // dotarliśmy do końca ścieżki
        m_currentPath.clear();
        m_currentPathIndex = 0;
        // przejdź do celu końcowego (m_targetPosition) bezpośrednio
        // (poniższa logika sprawdzi odległość do m_targetPosition)
      } else {
        // przejdź do następnego waypointa
        return;
      }
    } else {
      direction = Vector3Normalize(direction);

      // Rotation (Smooth)
      float targetAngle = atan2(direction.x, direction.z) * RAD2DEG;
      float angleDiff = targetAngle - m_rotation;
      while (angleDiff > 180)
        angleDiff -= 360;
      while (angleDiff < -180)
        angleDiff += 360;
      m_rotation += angleDiff * 15.0f * deltaTime;

      Vector3 movement = Vector3Scale(direction, m_moveSpeed * deltaTime);
      position = Vector3Add(position, movement);
      return;
    }
  }
  // Bez ścieżki lub po jej zakończeniu: ruch bezpośredni do celu
  Vector3 direction = Vector3Subtract(m_targetPosition, position);
  float distance = Vector3Length(direction);

  // FIX: Wall Clipping Prevention - Poluzowano dystans dla pniaków/zasobów
  if (m_currentPath.empty() && distance > 10.0f) {
    std::cout << "[Settler] No path to target (" << distance
              << "m away). Stopping to avoid wall clip." << std::endl;
    m_state = SettlerState::IDLE;
    return;
  }
  if (distance < 0.5f) {
    // Jeśli szliśmy do warsztatu, przełącz na crafting
    if (m_state == SettlerState::MOVING && m_targetWorkshop &&
        m_currentCraftTaskId != -1) {
      m_state = SettlerState::CRAFTING;
      m_craftingTimer = 0.0f;
      return;
    }

    m_state = SettlerState::IDLE;
    m_isMovingToCriticalTarget = false;
    return;
  }
  direction = Vector3Normalize(direction);
  // Smooth Rotation
  float targetAngle = atan2(direction.x, direction.z) * RAD2DEG;
  float angleDiff = targetAngle - m_rotation;
  while (angleDiff > 180)
    angleDiff -= 360;
  while (angleDiff < -180)
    angleDiff += 360;
  m_rotation += angleDiff * 15.0f * deltaTime;

  Vector3 movement = Vector3Scale(direction, m_moveSpeed * deltaTime);
  Vector3 nextPos = Vector3Add(position, movement);

  // TREE COLLISION CHECK (ALL TREES, not just stumps)
  for (const auto &tree : trees) {
    if (tree && tree->isActive()) { // Check ALL active trees
      BoundingBox treeBox = tree->getBoundingBox();
      if (CheckCollisionBoxSphere(treeBox, nextPos, 0.4f)) {
        std::cout << "[Settler] Blocked by tree." << std::endl;
        m_state = SettlerState::IDLE;
        m_currentPath.clear();
        return;
      }
    }
  }

  // BUILDING COLLISION CHECK
  if (g_buildingSystem) {
    auto buildingsNearby = g_buildingSystem->getBuildingsInRange(nextPos, 2.0f);
    for (auto *b : buildingsNearby) {
      if (b->getBlueprintId() == "floor")
        continue;
      if (b->CheckCollision(nextPos, 0.4f)) {
        // Collision detected! Try sliding or stop.
        // Simple stop for now:
        m_state = SettlerState::IDLE;
        m_currentPath.clear();
        std::cout << "[Settler] Movement blocked by building. Stopping."
                  << std::endl;
        return;
      }
    }
  }

  // RESOURCE NODE (KAMIENIE) COLLISION CHECK
  for (const auto &node : resourceNodes) {
    if (node && !node->isDepleted()) {
      BoundingBox nodeBox = node->getBoundingBox();
      if (CheckCollisionBoxSphere(nodeBox, nextPos, 0.4f)) {
        std::cout << "[Settler] Blocked by resource node (stone)." << std::endl;
        m_state = SettlerState::IDLE;
        m_currentPath.clear();
        return;
      }
    }
  }

  // DOOR COLLISION CHECK (only closed doors block)
  if (g_colony) {
    const auto &doors = g_colony->getDoors();
    for (const auto *door : doors) {
      if (door && !door->isOpen()) { // Tylko zamknięte drzwi blokują
        BoundingBox doorBox = door->getBoundingBox();
        if (CheckCollisionBoxSphere(doorBox, nextPos, 0.4f)) {
          std::cout << "[Settler] Blocked by closed door." << std::endl;
          m_state = SettlerState::IDLE;
          m_currentPath.clear();
          return;
        }
      }
    }
  }

  position = nextPos;
}
void Settler::UpdateGathering(
    float deltaTime, std::vector<WorldItem> &worldItems,
    const std::vector<BuildingInstance *> &buildings) {
  (void)worldItems;
  (void)buildings;
  m_gatherTimer += deltaTime;
  if (m_gatherTimer >= m_gatherInterval) {
    // Logika zbierania...
  }
}
void Settler::UpdateBuilding(float deltaTime) {
  if (!m_currentBuildTask) {
    m_state = SettlerState::IDLE;
    return;
  }
  m_currentBuildTask->advanceConstruction(deltaTime * 10.0f);
  if (m_currentBuildTask->isCompleted()) {
    // Zadanie ukończone, zwolnij zadanie
    m_currentBuildTask = nullptr;
    m_state = SettlerState::IDLE;
  }
}
void Settler::UpdateSleeping(float deltaTime) {
  // Zmniejsz cooldown snu (jeśli aktywny)
  if (m_sleepCooldownTimer > 0.0f) {
    m_sleepCooldownTimer -= deltaTime;
    if (m_sleepCooldownTimer < 0.0f)
      m_sleepCooldownTimer = 0.0f;
  }
  // Jeśli nie ma przypisanego łóżka, natychmiast wyjdź ze snu
  if (!m_assignedBed) {
    m_state = SettlerState::IDLE;
    return;
  }

  // Sprawdź, czy osadnik jest fizycznie na łóżku
  Vector3 bedPos = m_assignedBed->getPosition();
  float distanceToBed = Vector3Distance(position, bedPos);
  const float onBedRadius = 0.55f;
  if (distanceToBed > onBedRadius) {
    // Osadnik nie jest na łóżku - przerwij sen i idź do łóżka
    m_state = SettlerState::MOVING_TO_BED;
    MoveTo(bedPos);
    return;
  }

  // Osadnik jest na łóżku - regeneruj energię
  m_stats.modifyEnergy(deltaTime * 5.0f);

  // Wyjdź ze snu, gdy energia przekroczy próg wyjścia (histereza)
  if (m_stats.getCurrentEnergy() >= m_sleepExitThreshold) {
    std::cout << "[Settler] " << m_name
              << " obudził się z energią: " << m_stats.getCurrentEnergy()
              << std::endl;
    m_state = SettlerState::IDLE;
    // Ustaw cooldown, aby uniknąć natychmiastowego powrotu do snu
    m_sleepCooldownTimer = 30.0f; // 30 sekund
  }
}
void Settler::UpdateMovingToStorage(
    float deltaTime, const std::vector<BuildingInstance *> &buildings,
    const std::vector<std::unique_ptr<ResourceNode>> &resourceNodes) {
  (void)buildings;
  // Walidacja celu
  if (std::isnan(m_targetPosition.x) || std::isnan(m_targetPosition.y) ||
      std::isnan(m_targetPosition.z) ||
      std::abs(m_targetPosition.x) > 10000.0f ||
      std::abs(m_targetPosition.z) > 10000.0f) {
    std::cerr
        << "[Settler] CRITICAL: UpdateMovingToStorage - nieprawidłowy cel: ("
        << m_targetPosition.x << ", " << m_targetPosition.y << ", "
        << m_targetPosition.z << ")" << std::endl;
    Stop();
    m_state = SettlerState::IDLE;
    return;
  }
  Vector3 direction = Vector3Subtract(m_targetPosition, position);
  float distance = Vector3Length(direction);

  // Epsilon dla dotarcia - ZWIĘKSZONY dystans (3.0f) aby nie wchodzić do
  // magazynu
  if (distance < 3.0f) {
    Stop(); // Zatrzymaj się przed wejściem
    std::cout << "[Settler] Dotarłem do magazynu (dystans < 3.0)." << std::endl;

    // CZY TO JEST MISJA CRAFTINGOWA? (Pobranie surowców)
    if (m_currentCraftTaskId != -1) {
      std::cout << "[Settler] To misja craftingowa - próba podjęcia surowców."
                << std::endl;
      auto storageSys = GameEngine::getInstance().getSystem<StorageSystem>();
      if (storageSys && m_targetStorage) {
        // Sprawdź czego brakuje w ekwipunku
        bool hasWood = false;
        bool hasStone = false;
        const auto &items = m_inventory.getItems();
        for (const auto &invItem : items) {
          if (invItem && invItem->item) {
            auto *res = dynamic_cast<ResourceItem *>(invItem->item.get());
            if (res) {
              if (res->getResourceType() == "Wood")
                hasWood = true;
              if (res->getResourceType() == "Stone")
                hasStone = true;
            }
          }
        }

        // Pobierz brakujące (hardcoded for Knife for now: 1 Wood, 1 Stone)
        if (!hasStone) {
          int taken = storageSys->removeResourceFromStorage(
              m_targetStorage->getStorageId(), m_name,
              Resources::ResourceType::Stone, 1);
          if (taken > 0) {
            m_inventory.addItem(std::make_unique<ResourceItem>(
                "Stone", "Stone Rock", "Raw stone."));
            std::cout << "[Settler] Pobrałem Stone z magazynu." << std::endl;
          }
        }

        // Opcjonalnie Wood, jeśli też tu jest i go brakuje
        if (!hasWood) {
          int taken = storageSys->removeResourceFromStorage(
              m_targetStorage->getStorageId(), m_name,
              Resources::ResourceType::Wood, 1);
          if (taken > 0) {
            m_inventory.addItem(std::make_unique<ResourceItem>(
                "Wood", "Wood Log", "Freshly chopped wood."));
            std::cout << "[Settler] Pobrałem Wood z magazynu." << std::endl;
          }
        }
      }

      m_state = SettlerState::IDLE; // Wróć do IDLE, aby Update wykrył, że mamy
                                    // surowce (lub nie) i kontynuował
      return;
    }

    std::cout << "[Settler] Przechodzę do deponowania." << std::endl;
    m_state = SettlerState::DEPOSITING;
    return;
  }

  // Dodatkowe logowanie co 1s
  static float lastMoveLog = 0.0f;
  float currentTimeMov = (float)GetTime();
  if (currentTimeMov - lastMoveLog > 1.0f) {
    std::cout << "[Settler] MovingToStorage: pos=(" << position.x << ","
              << position.y << "," << position.z << ") target=("
              << m_targetPosition.x << "," << m_targetPosition.y << ","
              << m_targetPosition.z << ") dist=" << distance << std::endl;
    lastMoveLog = currentTimeMov;
  }

  // Use centralized movement logic to respect pathfinding and walls
  UpdateMovement(deltaTime, {}, buildings, resourceNodes);
}
void Settler::UpdateDepositing(
    float deltaTime, const std::vector<BuildingInstance *> &buildings) {

  (void)deltaTime;

  if (!m_targetStorage || !m_targetStorage->isBuilt()) {

    m_state = SettlerState::IDLE;

    return;
  }

  std::string storageId = m_targetStorage->getStorageId();

  if (storageId.empty()) {

    m_state = SettlerState::IDLE;

    return;
  }

  auto storageSys = GameEngine::getInstance().getSystem<StorageSystem>();

  if (!storageSys) {

    m_state = SettlerState::IDLE;

    return;
  }

  std::cout << "[Settler] Rozpoczynam deponowanie w magazynie " << storageId
            << std::endl;

  std::vector<std::pair<int, int>> itemsToRemove;

  const auto &items = m_inventory.getItems();

  bool anyDeposited = false;

  for (size_t i = 0; i < items.size(); ++i) {

    const auto &invItem = items[i];

    if (!invItem || !invItem->item)
      continue;

    if (invItem->item->getItemType() == ItemType::RESOURCE ||
        invItem->item->getItemType() == ItemType::CONSUMABLE) {
      Resources::ResourceType type = Resources::ResourceType::None;
      std::string typeStr = "";

      if (invItem->item->getItemType() == ItemType::RESOURCE) {
        ResourceItem *resItem =
            dynamic_cast<ResourceItem *>(invItem->item.get());
        if (resItem) {
          typeStr = resItem->getResourceType();
          if (typeStr == "Wood")
            type = Resources::ResourceType::Wood;
          else if (typeStr == "Stone")
            type = Resources::ResourceType::Stone;
          else if (typeStr == "Food")
            type = Resources::ResourceType::Food;
          else if (typeStr == "Metal")
            type = Resources::ResourceType::Metal;
          else if (typeStr == "Gold")
            type = Resources::ResourceType::Gold;
        }
      } else if (invItem->item->getItemType() == ItemType::CONSUMABLE) {
        typeStr = invItem->item->getDisplayName();
        if (typeStr == "Raw Meat" || typeStr == "Cooked Meat" ||
            typeStr == "Berry" || typeStr == "Food") {
          type = Resources::ResourceType::Food;
        } else if (typeStr == "Water") {
          type = Resources::ResourceType::Water;
        }
      }

      if (type != Resources::ResourceType::None) {
        int32_t added = storageSys->addResourceToStorage(
            storageId, m_name, type, invItem->quantity);

        if (added > 0) {
          itemsToRemove.push_back({(int)i, added});
          anyDeposited = true;
          std::cout << "[Settler] Zdeponowano " << added << " jedn. " << typeStr
                    << " (Typ: " << (int)type << ") do magazynu " << storageId
                    << std::endl;
        } else {
          // std::cout << "[Settler] Magazyn pełny dla " << typeStr <<
          // std::endl;
        }
      }
    }
  }

  // Usuń zdeponowane itemy z ekwipunku

  for (int i = (int)itemsToRemove.size() - 1; i >= 0; --i) {

    m_inventory.extractItem(itemsToRemove[i].first, itemsToRemove[i].second);
  }

  // Sprawdź czy pozostały jeszcze zasoby w ekwipunku
  bool hasRemainingResources = false;
  for (const auto &invItem : items) {
    if (invItem && invItem->item &&
        invItem->item->getItemType() == ItemType::RESOURCE) {
      hasRemainingResources = true;
      break;
    }
  }
  if (hasRemainingResources) {
    // Nie udało się zdeponować wszystkich zasobów (magazyn pełny)
    std::cout << "[Settler] Magazyn " << storageId
              << " nie przyjął wszystkich zasobów. Oznaczam jako ignorowany i "
                 "szukam innego."
              << std::endl;
    ignoreStorage(storageId);
    BuildingInstance *newStorage = FindNearestStorage(buildings);
    if (newStorage) {
      std::cout << "[Settler] Znaleziono alternatywny magazyn: "
                << newStorage->getStorageId() << std::endl;
      m_targetStorage = newStorage;
      MoveTo(newStorage->getPosition());
      m_state = SettlerState::MOVING_TO_STORAGE;
      return;
    } else {
      std::cout
          << "[Settler] Brak dostępnych magazynów. Upuszczam zasoby na ziemię."
          << std::endl;
      // Awaryjne upuszczenie zasobów
      const auto &invItems = m_inventory.getItems();
      for (size_t i = 0; i < invItems.size(); ++i) {
        const auto &invItem = invItems[i];
        if (!invItem || !invItem->item ||
            invItem->item->getItemType() != ItemType::RESOURCE)
          continue;
        ResourceItem *resItem =
            dynamic_cast<ResourceItem *>(invItem->item.get());
        if (!resItem)
          continue;
        for (int q = 0; q < invItem->quantity; ++q) {
          auto droppedItem = std::make_unique<ResourceItem>(
              resItem->getResourceType(), resItem->getDisplayName(),
              resItem->getDescription());
          if (GameEngine::dropItemCallback) {
            Vector3 dropPos = position;
            dropPos.x += (float)(rand() % 10 - 5) * 0.1f;
            dropPos.z += (float)(rand() % 10 - 5) * 0.1f;
            dropPos.y = 0.5f;
            GameEngine::dropItemCallback(dropPos, droppedItem.release());
          }
        }
      }
      m_inventory.clear();
      m_state = SettlerState::IDLE;
      return;
    }
  } // koniec if (hasRemainingResources)
  // Jeśli nie ma już zasobów, zakończ deponowanie
  if (anyDeposited) {
    std::cout << "[Settler] Wszystkie zasoby zdeponowane. Wracam do IDLE."
              << std::endl;
  } else {
    std::cout << "[Settler] Nie zdeponowano żadnych zasobów (magazyn pełny). "
                 "Szukam innego magazynu."
              << std::endl;
    ignoreStorage(storageId);
    BuildingInstance *newStorage = FindNearestStorage(buildings);
    if (newStorage) {
      std::cout << "[Settler] Znaleziono alternatywny magazyn: "
                << newStorage->getStorageId() << std::endl;
      m_targetStorage = newStorage;
      MoveTo(newStorage->getPosition());
      m_state = SettlerState::MOVING_TO_STORAGE;
      return;
    } else {
      std::cout << "[Settler] Brak dostępnych magazynów. Przechodzę w stan "
                   "oczekiwania (1s)."
                << std::endl;
      m_state = SettlerState::WAITING;
      m_gatherTimer = 1.0f;
      return;
    }
  }
  m_state = SettlerState::IDLE;
}
void Settler::UpdatePickingUp(
    float deltaTime, std::vector<WorldItem> &worldItems,
    const std::vector<BuildingInstance *> &buildings) {

  float minDist = 2.0f;
  int pickedIndex = -1;
  int nearestItemIndex = -1;
  float nearestItemDist = 9999.0f;

  // Scan for pickable items
  for (size_t i = 0; i < worldItems.size(); ++i) {
    if (worldItems[i].pendingRemoval)
      continue;
    if (!worldItems[i].item ||
        (worldItems[i].item->getItemType() != ItemType::RESOURCE &&
         worldItems[i].item->getItemType() != ItemType::CONSUMABLE))
      continue;

    float d = Vector3Distance(position, worldItems[i].position);

    // Track nearest item
    if (d < nearestItemDist) {
      nearestItemDist = d;
      nearestItemIndex = (int)i;
    }

    // Attempt pickup
    if (d < minDist) {
      if (m_inventory.addItem(std::move(worldItems[i].item))) {
        worldItems[i].pendingRemoval = true;
        pickedIndex = (int)i;
        std::cout << "[Settler] SUCCESS! Picked up item. Inventory count: "
                  << m_inventory.getItemCount() << std::endl;
        break; // Picked up one item, stop loop
      }
    }
  }

  // Logic after scan
  if (pickedIndex != -1) {
    // SUCCESS: Picked up item
    // Note: item unique_ptr is moved, so we rely on context or generic log
    std::cout << "[Settler] Picked up item. Returning to IDLE logic."
              << std::endl;

    // FAILSAFE: If we are crafting, immediately update crafting logic to
    // prevent stalling
    if (m_currentCraftTaskId != -1) {
      std::cout << "[Settler] Picked up item while crafting. Re-evaluating "
                   "crafting needs..."
                << std::endl;
      // Don't just go IDLE, check needs immediately in next frame logic.
      // Setting IDLE is correct because Update() will call UpdateIdle() or
      // UpdateCrafting(). But we want to ensure we don't pick up something else
      // or get distracted.
      m_state = SettlerState::IDLE;
    } else if ((m_currentBuildTask && m_currentBuildTask->isActive()) ||
               (m_myPrivateBuildTask && m_myPrivateBuildTask->isActive())) {
      // PRIORITY: BUILDING
      // If we are building, don't haul to storage. Go IDLE so Update() can see
      // we have resources and trigger ProcessActiveBuildTask (or fetch more if
      // needed).
      std::cout << "[Settler] Item picked up. Priority: BUILDING (Active "
                   "Task). Returning to IDLE."
                << std::endl;
      m_state = SettlerState::IDLE;
    } else {
      // Normal mode: Haul to storage
      BuildingInstance *storage = FindNearestStorage(buildings);
      if (storage) {
        m_targetStorage = storage;
        MoveTo(storage->getPosition());
        m_state = SettlerState::MOVING_TO_STORAGE;
      } else {
        std::cout << "[Settler] No storage found, dropping item." << std::endl;
        m_inventory.clear(); // Drop logic simplified for now
        m_state = SettlerState::IDLE;
      }
    }
  } else {
    // NO PICKUP YET
    if (nearestItemIndex != -1) {
      // ... tracking logic ...
    } else {
      // No items found at all?
      // Only log periodically to avoid spam
      static float noItemLog = 0.0f;
      float currentTimePick = (float)GetTime();
      if (currentTimePick - noItemLog > 2.0f) {
        std::cout << "[Settler] Walking to stones but NO item found in range ("
                  << worldItems.size() << " world items)." << std::endl;
        noItemLog = currentTimePick;
      }
    }
    if (nearestItemIndex != -1) {
      // Found item but too far? Move to it!
      Vector3 target = worldItems[nearestItemIndex].position;
      // Update target position to the item's location
      m_targetPosition = target;

      // Move towards it
      Vector3 direction = Vector3Subtract(m_targetPosition, position);
      float dist = Vector3Length(direction);

      // LOGGING: Movement attempt
      static float debugLogTimer = 0.0f;
      debugLogTimer += deltaTime;
      if (debugLogTimer > 1.0f) {
        std::cout << "[Settler] Tracking item at " << target.x << ","
                  << target.z << " (dist: " << dist << ")" << std::endl;
        debugLogTimer = 0.0f;
      }

      if (dist > 0.1f) {
        direction = Vector3Normalize(direction);
        position = Vector3Add(position,
                              Vector3Scale(direction, m_moveSpeed * deltaTime));

        // Rotation
        float targetAngle = atan2(direction.x, direction.z) * RAD2DEG;
        float angleDiff = targetAngle - m_rotation;
        while (angleDiff > 180)
          angleDiff -= 360;
        while (angleDiff < -180)
          angleDiff += 360;
        m_rotation += angleDiff * 5.0f * deltaTime;
      }
    } else {
      // LOGGING: No item found
      static float debugLogTimer2 = 0.0f;
      debugLogTimer2 += deltaTime;
      if (debugLogTimer2 > 1.0f) {
        std::cout
            << "[Settler] No pickable items found nearby. Moving to target "
            << m_targetPosition.x << "," << m_targetPosition.z << std::endl;
        debugLogTimer2 = 0.0f;
      }

      // If we are at the target (old tree pos) and still see nothing...
      // Increment timer to allow for spawn delay/physics
      m_gatherTimer += deltaTime;

      float distToTarget = Vector3Distance(position, m_targetPosition);
      if (distToTarget < 0.5f) {
        if (m_gatherTimer > 2.0f) {
          std::cout << "[Settler] Reached target but found no item after 2s. "
                       "Giving up and returning to IDLE."
                    << std::endl;
          m_state = SettlerState::IDLE;
        } else {
          // Wait / Look around
          // Optional: Rotate or small random movement?
          // For now just wait.
        }
      } else {
        // Continue moving to target (maybe item is there)
        Vector3 direction = Vector3Subtract(m_targetPosition, position);
        direction = Vector3Normalize(direction);
        position = Vector3Add(position,
                              Vector3Scale(direction, m_moveSpeed * deltaTime));
      }
    }
  }
}

void Settler::UpdateSearchingForFood(float deltaTime,
                                     const std::vector<Bush *> &bushes) {

  (void)deltaTime;

  Bush *nearestFood = FindNearestFood(bushes);

  if (nearestFood) {

    m_targetFoodBush = nearestFood;

    MoveTo(nearestFood->position);

    m_state = SettlerState::MOVING_TO_FOOD;

    m_isMovingToCriticalTarget = true;

  } else {

    m_state = SettlerState::IDLE;

    m_isMovingToCriticalTarget = false;
  }
}
void Settler::UpdateMovingToFood(float deltaTime) {

  Vector3 direction = Vector3Subtract(m_targetPosition, position);

  float distance = Vector3Length(direction);

  if (distance < 0.5f) {

    m_state = SettlerState::EATING;

    m_isMovingToCriticalTarget = false;

    return;
  }

  direction = Vector3Normalize(direction);

  Vector3 movement = Vector3Scale(direction, m_moveSpeed * deltaTime);

  position = Vector3Add(position, movement);
}
void Settler::UpdateEating(float deltaTime) {

  m_eatingTimer += deltaTime;

  if (m_eatingTimer >= 2.0f) {

    m_eatingTimer = 0.0f;

    m_stats.setHunger(100.0f);

    m_state = SettlerState::IDLE;
  }
}
void Settler::UpdateCrafting(float deltaTime) {
  if (m_currentCraftTaskId == -1) {
    m_state = SettlerState::IDLE;
    return;
  }

  m_craftingTimer += deltaTime;
  // Estimated craft time 5.0s (consistent with progress bar)
  if (m_craftingTimer >= 5.0f) {
    std::cout << "[Settler] Crafting complete! Task ID: "
              << m_currentCraftTaskId << std::endl;

    // Complete logic via CraftingSystem
    // Note: CraftingSystem should handle item creation.
    auto craftingSys = GameEngine::getInstance().getSystem<CraftingSystem>();
    if (craftingSys) {
      // completeTask returns created item if any
      auto item = craftingSys->completeTask(m_currentCraftTaskId);
      std::cout << "[DEBUG CRAFT] completeTask returned item: "
                << (item ? "YES" : "NO") << std::endl;
      if (item) {
        // Sprawdź czy ręka wolna
        bool handFree = isHandFree();
        std::cout << "[DEBUG CRAFT] isHandFree: " << handFree
                  << ", m_heldItem before: " << (m_heldItem ? "EXISTS" : "NULL")
                  << std::endl;
        if (handFree) {
          std::cout << "[Settler] Craft Knife finished -> equipped in hand"
                    << std::endl;
          setHeldItem(std::move(item));
          std::cout << "[DEBUG CRAFT] After setHeldItem, m_heldItem: "
                    << (m_heldItem ? "EXISTS" : "NULL") << std::endl;
        } else {
          // Ręka zajęta - dodaj do ekwipunku
          if (m_inventory.addItem(std::move(item))) {
            std::cout << "[Settler] Craft finished -> added to inventory"
                      << std::endl;
          } else {
            std::cout
                << "[Settler] Craft finished -> inventory full -> dropping"
                << std::endl;
            // Drop item? For now just log usage
          }
        }
      } else {
        std::cout << "[DEBUG CRAFT] completeTask returned nullptr!"
                  << std::endl;
      }
    }

    // Remove ingredients
    m_inventory.removeResource("Wood", 1);
    m_inventory.removeResource("Stone", 1);

    // Reset state
    m_currentCraftTaskId = -1;
    m_craftingTimer = 0.0f;
    m_state = SettlerState::IDLE;

    std::cout << "[Settler] Ingredients consumed. Transitioning to IDLE."
              << std::endl;
  }
}

float Settler::GetCraftingProgress01() const {
  if (m_state != SettlerState::CRAFTING)
    return 0.0f;
  // Uproszczony czas craftingu = 3.0s (zgodnie z UpdateCrafting)
  return fminf(m_craftingTimer / 3.0f, 1.0f);
}
BuildingInstance *
Settler::FindNearestStorage(const std::vector<BuildingInstance *> &buildings) {
  static float lastDebugLogTime = -1.0f;
  BuildingInstance *nearest = nullptr;
  float minDst = 10000.0f;
  auto storageSys = GameEngine::getInstance().getSystem<StorageSystem>();
  if (!storageSys) {
    std::cout << "[Settler] Brak systemu magazynowego!" << std::endl;
    return nullptr;
  }
  // Pomocnicza lambda do sprawdzania poprawności pozycji
  auto isValidPosition = [](const Vector3 &pos) -> bool {
    return !std::isnan(pos.x) && !std::isnan(pos.y) && !std::isnan(pos.z) &&
           !std::isinf(pos.x) && !std::isinf(pos.y) && !std::isinf(pos.z) &&
           std::abs(pos.x) < 10000.0f && std::abs(pos.y) < 10000.0f &&
           std::abs(pos.z) < 10000.0f;
  };
  // Diagnostyka: wypisz liczbę budynków i szczegóły (throttle 1s)
  float currentTimeNear = (float)GetTime(); // funkcja z raylib
  bool shouldLog =
      (currentTimeNear - lastDebugLogTime > 1.0f) || (lastDebugLogTime < 0);
  if (shouldLog) {
    std::cout << "[Settler] DEBUG FindNearestStorage: buildingsTotal = "
              << buildings.size() << std::endl;
    int idx = 0;
    for (auto *b : buildings) {
      if (!b) {
        std::cout << "  [" << idx << "] nullptr" << std::endl;
        idx++;
        continue;
      }
      std::string storageId = b->getStorageId();
      std::string blueprintId = b->getBlueprintId();
      bool built = b->isBuilt();
      BuildingCategory category = BuildingCategory::STRUCTURE;
      if (b->getBlueprint()) {
        category = b->getBlueprint()->getCategory();
      }
      std::string categoryStr = "UNKNOWN";
      switch (category) {
      case BuildingCategory::STORAGE:
        categoryStr = "STORAGE";
        break;
      case BuildingCategory::RESIDENTIAL:
        categoryStr = "RESIDENTIAL";
        break;
      case BuildingCategory::PRODUCTION:
        categoryStr = "PRODUCTION";
        break;
      case BuildingCategory::FURNITURE:
        categoryStr = "FURNITURE";
        break;
      default:
        categoryStr = "STRUCTURE";
        break;
      }
      Vector3 pos = b->getPosition();
      bool validPos = isValidPosition(pos);
      std::cout << "  [" << idx << "] id=" << storageId
                << " blueprint=" << blueprintId << " built=" << built
                << " category=" << categoryStr << " pos=(" << pos.x << ","
                << pos.y << "," << pos.z << ")"
                << " valid=" << (validPos ? "true" : "false") << std::endl;
      idx++;
      idx++;
    }
    lastDebugLogTime = currentTimeNear;
  }
  // Etap 1: Szukaj budynku z kategorią STORAGE i canAddResource == true
  for (auto *b : buildings) {
    if (!b) {
      if (shouldLog) {
        std::cout << "[Settler] DEBUG: Odrzucono nullptr" << std::endl;
      }
      continue;
    }
    std::string storageId = b->getStorageId();
    std::string blueprintId = b->getBlueprintId();

    // Sprawdź czy zbudowany (z wyjątkiem stockpile) - użyj substring
    bool isStockpile = (blueprintId.find("stockpile") != std::string::npos);
    if (!b->isBuilt() && !isStockpile) {
      if (shouldLog) {
        std::cout << "[Settler] DEBUG: Odrzucono " << storageId << " ("
                  << blueprintId << ") - !isBuilt()" << std::endl;
      }
      continue;
    }
    if (storageId.empty()) {
      if (shouldLog) {
        std::cout << "[Settler] DEBUG: Odrzucono " << blueprintId
                  << " - brak/invalid storageId" << std::endl;
      }
      continue;
    }

    // Sprawdź kategorię budynku
    BuildingCategory category = BuildingCategory::STRUCTURE;
    if (b->getBlueprint()) {
      category = b->getBlueprint()->getCategory();
    }

    // Tylko STORAGE w etapie 1, ale stockpile traktuj jako STORAGE niezależnie
    // od kategorii
    if (category != BuildingCategory::STORAGE && !isStockpile) {
      if (shouldLog) {
        std::cout << "[Settler] DEBUG: Odrzucono " << storageId << " ("
                  << blueprintId << ") - kategoria "
                  << static_cast<int>(category) << " nie STORAGE" << std::endl;
      }
      continue;
    }

    // Sprawdź czy magazyn jest ignorowany
    if (isStorageIgnored(storageId)) {
      if (shouldLog) {
        std::cout << "[Settler] DEBUG: Odrzucono " << storageId
                  << " - ignorowany (lista ignored)." << std::endl;
      }
      continue;
    }

    // Sprawdź pojemność
    bool canAcceptAny = false;
    const auto &items = m_inventory.getItems();

    for (const auto &invItem : items) {
      if (!invItem || !invItem->item ||
          invItem->item->getItemType() != ItemType::RESOURCE)
        continue;
      ResourceItem *resItem = dynamic_cast<ResourceItem *>(invItem->item.get());
      if (!resItem)
        continue;
      std::string typeStr = resItem->getResourceType();
      Resources::ResourceType type = Resources::ResourceType::None;
      if (typeStr == "Wood")
        type = Resources::ResourceType::Wood;
      else if (typeStr == "Stone")
        type = Resources::ResourceType::Stone;
      else if (typeStr == "Food")
        type = Resources::ResourceType::Food;
      else if (typeStr == "Metal")
        type = Resources::ResourceType::Metal;
      else if (typeStr == "Gold")
        type = Resources::ResourceType::Gold;

      if (type != Resources::ResourceType::None) {
        // Log parametrów canAddResource
        std::cout
            << "[Settler] DEBUG: Sprawdzanie canAddResource dla storageId="
            << storageId << ", resourceType=" << typeStr
            << ", quantity=" << invItem->quantity << std::endl;
        if (storageSys->canAddResource(storageId, type, invItem->quantity)) {
          canAcceptAny = true;
          break;
        } else {
          std::cout << "[Settler] DEBUG: Odrzucono - canAddResource zwróciło "
                       "false dla "
                    << typeStr << std::endl;
        }
      } else {
        // Zasób unknown
        std::cout << "[Settler] WARNING: Nieznany typ zasobu: " << typeStr
                  << std::endl;
        // TODO: Rozważyć obsługę nieznanych zasobów lub walidację typów
        // Na razie odrzucamy, ale logujemy ostrzeżenie
      }
    }

    // Jeśli nie mamy żadnych zasobów (np. jeszcze nie podnieśliśmy), to załóżmy
    // że magazyn może przyjąć cokolwiek (sprawdź wolne sloty)
    if (!canAcceptAny && items.empty()) {
      StorageSystem::StorageInstance *storage =
          storageSys->getStorage(storageId);
      if (storage && storage->getFreeSlots() > 0) {
        canAcceptAny = true;
      } else {
        if (shouldLog) {
          std::cout << "[Settler] DEBUG: Odrzucono " << storageId
                    << " - brak wolnych slotów." << std::endl;
        }
        continue;
      }
    }

    if (!canAcceptAny) {
      if (shouldLog) {
        std::cout << "[Settler] DEBUG: Odrzucono " << storageId
                  << " - storageSys->canAddResource(...) == false dla "
                     "wszystkich zasobów."
                  << std::endl;
      }
      continue;
    }

    // Sprawdź czy pozycja jest prawidłowa
    Vector3 buildingPos = b->getPosition();
    if (!isValidPosition(buildingPos)) {
      if (shouldLog) {
        std::cout << "[Settler] DEBUG: Odrzucono " << storageId
                  << " - nieprawidłowa pozycja (" << buildingPos.x << ", "
                  << buildingPos.y << ", " << buildingPos.z << ")" << std::endl;
      }
      continue;
    }

    // Wszystkie warunki spełnione – oblicz odległość
    float d = Vector3Distance(position, buildingPos);
    if (shouldLog) {
      std::cout << "[Settler] DEBUG: Kandydat STORAGE " << storageId << " ("
                << blueprintId << ") zaakceptowany, "
                << "settlerPos=(" << position.x << "," << position.y << ","
                << position.z << "), "
                << "targetPos=(" << buildingPos.x << "," << buildingPos.y << ","
                << buildingPos.z << "), "
                << "odległość = " << d << std::endl;
    }
    if (d < minDst) {
      minDst = d;
      nearest = b;
    }
  }

  if (nearest) {
    std::cout << "[Settler] Znaleziono magazyn (kategoria STORAGE): "
              << nearest->getStorageId() << " w odległości " << minDst
              << std::endl;
    return nearest;
  }

  // Etap 2: fallback - dowolny budynek (nie RESIDENTIAL) gdzie canAddResource
  // == true
  std::cout << "[Settler] Nie znaleziono magazynu kategorii STORAGE. "
               "Przechodzę do fallback (dowolny budynek)."
            << std::endl;
  nearest = nullptr;
  minDst = 10000.0f;

  for (auto *b : buildings) {
    if (!b) {
      if (shouldLog) {
        std::cout << "[Settler] DEBUG: Odrzucono fallback nullptr" << std::endl;
      }
      continue;
    }
    std::string storageId = b->getStorageId();
    std::string blueprintId = b->getBlueprintId();
    bool isStockpile = (blueprintId == "stockpile");

    // Dla stockpile ignorujemy isBuilt() - użyj substring
    if (!b->isBuilt() && !isStockpile) {
      if (shouldLog) {
        std::cout << "[Settler] DEBUG: Odrzucono fallback " << storageId << " ("
                  << blueprintId << ") - !isBuilt()" << std::endl;
      }
      continue;
    }
    if (storageId.empty()) {
      if (shouldLog) {
        std::cout << "[Settler] DEBUG: Odrzucono fallback " << blueprintId
                  << " - brak/invalid storageId" << std::endl;
      }
      continue;
    }

    BuildingCategory category = BuildingCategory::STRUCTURE;
    if (b->getBlueprint()) {
      category = b->getBlueprint()->getCategory();
    }

    // Nie odrzucamy RESIDENTIAL - decyduje canAddResource.

    // Odrzuć jeśli ignorowany
    if (isStorageIgnored(storageId)) {
      if (shouldLog) {
        std::cout << "[Settler] DEBUG: Odrzucono fallback " << storageId
                  << " - ignorowany (lista ignored)." << std::endl;
      }
      continue;
    }

    // Sprawdź pojemność
    bool canAcceptAny = false;
    const auto &items = m_inventory.getItems();

    for (const auto &invItem : items) {
      if (!invItem || !invItem->item ||
          (invItem->item->getItemType() != ItemType::RESOURCE &&
           invItem->item->getItemType() != ItemType::CONSUMABLE))
        continue;

      // Handle Resources
      if (invItem->item->getItemType() == ItemType::RESOURCE) {
        ResourceItem *resItem =
            dynamic_cast<ResourceItem *>(invItem->item.get());
        if (!resItem)
          continue;
        std::string typeStr = resItem->getResourceType();
        Resources::ResourceType type = Resources::ResourceType::None;
        if (typeStr == "Wood")
          type = Resources::ResourceType::Wood;
        else if (typeStr == "Stone")
          type = Resources::ResourceType::Stone;
        else if (typeStr == "Food")
          type = Resources::ResourceType::Food;
        else if (typeStr == "Metal")
          type = Resources::ResourceType::Metal;
        else if (typeStr == "Gold")
          type = Resources::ResourceType::Gold;

        if (type != Resources::ResourceType::None) {
          if (storageSys->canAddResource(storageId, type, invItem->quantity)) {
            canAcceptAny = true;
            break;
          }
        }
      }
      // Handle Consumables (Food/Meat)
      else if (invItem->item->getItemType() == ItemType::CONSUMABLE) {
        // For now, assume storehouses accept food if they have slots.
        // Or use specific Food type if available in StorageSystem.
        // We'll rely on slot check inside storageSys or just generic slot check
        // below. Check if storage allows 'Food' resource type mapping?
        // Simplest: Check if storage has "Food" capacity or free slots.
        if (storageSys->canAddResource(storageId, Resources::ResourceType::Food,
                                       invItem->quantity)) {
          canAcceptAny = true;
          break;
        }
      }
    }

    if (!canAcceptAny && items.empty()) {
      StorageSystem::StorageInstance *storage =
          storageSys->getStorage(storageId);
      if (storage && storage->getFreeSlots() > 0) {
        canAcceptAny = true;
      } else {
        if (shouldLog) {
          std::cout << "[Settler] DEBUG: Odrzucono fallback " << storageId
                    << " - brak wolnych slotów." << std::endl;
        }
        continue;
      }
    }

    if (!canAcceptAny) {
      if (shouldLog) {
        std::cout << "[Settler] DEBUG: Odrzucono fallback " << storageId
                  << " - storageSys->canAddResource(...) == false dla "
                     "wszystkich zasobów."
                  << std::endl;
      }
      continue;
    }

    // Sprawdź czy pozycja jest prawidłowa
    Vector3 buildingPos = b->getPosition();
    if (!isValidPosition(buildingPos)) {
      if (shouldLog) {
        std::cout << "[Settler] DEBUG: Odrzucono fallback " << storageId
                  << " - nieprawidłowa pozycja (" << buildingPos.x << ", "
                  << buildingPos.y << ", " << buildingPos.z << ")" << std::endl;
      }
      continue;
    }
    float d = Vector3Distance(position, buildingPos);
    if (shouldLog) {
      std::cout << "[Settler] Kandydat fallback " << storageId << " ("
                << blueprintId << ") zaakceptowany, "
                << "settlerPos=(" << position.x << "," << position.y << ","
                << position.z << "), "
                << "targetPos=(" << buildingPos.x << "," << buildingPos.y << ","
                << buildingPos.z << "), "
                << "odległość = " << d << std::endl;
    }
    if (d < minDst) {
      minDst = d;
      nearest = b;
    }
  }

  if (nearest) {
    std::cout << "[Settler] Znaleziono magazyn (fallback): "
              << nearest->getStorageId() << " w odległości " << minDst
              << std::endl;
  } else {
    std::cout << "[Settler] Nie znaleziono żadnego dostępnego magazynu (ani "
                 "STORAGE, ani fallback)."
              << std::endl;
    // Wymuszony log diagnostyczny przed zwróceniem nullptr
    std::cout << "[Settler] DIAGNOSTIC: buildingsTotal = " << buildings.size()
              << std::endl;
    int stockpileCount = 0;
    for (size_t i = 0; i < buildings.size() && i < 20; ++i) {
      auto *b = buildings[i];
      if (!b) {
        std::cout << "  [" << i << "] nullptr" << std::endl;
        continue;
      }
      std::string storageId = b->getStorageId();
      std::string blueprintId = b->getBlueprintId();
      bool built = b->isBuilt();
      BuildingCategory category = BuildingCategory::STRUCTURE;
      if (b->getBlueprint()) {
        category = b->getBlueprint()->getCategory();
      }
      std::string categoryStr = "UNKNOWN";
      switch (category) {
      case BuildingCategory::STORAGE:
        categoryStr = "STORAGE";
        break;
      case BuildingCategory::RESIDENTIAL:
        categoryStr = "RESIDENTIAL";
        break;
      case BuildingCategory::PRODUCTION:
        categoryStr = "PRODUCTION";
        break;
      case BuildingCategory::FURNITURE:
        categoryStr = "FURNITURE";
        break;
      default:
        categoryStr = "STRUCTURE";
        break;
      }
      Vector3 pos = b->getPosition();
      std::cout << "  [" << i << "] id=" << storageId
                << " blueprint=" << blueprintId << " built=" << built
                << " category=" << categoryStr << " pos=(" << pos.x << ","
                << pos.y << "," << pos.z << ")" << std::endl;
      // Sprawdź czy blueprintId zawiera "stockpile"
      if (blueprintId.find("stockpile") != std::string::npos) {
        stockpileCount++;
      }
    }
    std::cout << "[Settler] DIAGNOSTIC: stockpileCount = " << stockpileCount
              << std::endl;
  }
  return nearest;
}

BuildingInstance *
Settler::FindNearestWorkshop(const std::vector<BuildingInstance *> &buildings) {

  BuildingInstance *nearest = nullptr;

  float minDst = 10000.0f;

  for (auto *b : buildings) {
    if (!b || !b->isBuilt())
      continue;
    // Sprawdź czy to workshop (po ID blueprintu lub kategorii)
    // Zakładamy, że workshop to np. "workshop" lub kategoria CRAFTING (jeśli
    // dodamy) Na razie sprawdzamy blueprint ID ALE nie mam wglądu w listę
    // blueprintów. Załóżmy, że każdy budynek może być warsztatem jeśli ma
    // odpowiednie capability. Dla testu: każdy budynek o ID zawierającym
    // "workshop" lub po prostu dowolny budynek jeśli nie ma workshopu? Nie,
    // muszę znaleźć konkretny. Użyjmy "stockpile" jako warsztatu tymczasowo,
    // lub "house" jeśli nie ma workshopu. Ale lepiej: dodajmy blueprint
    // workshopu później. Tutaj szukamy "simple_workshop" (przykładowa nazwa).
    // Dla testu: szukamy 'simple_storage' też jako workshopu (multitool)

    bool isWorkshop =
        (b->getBlueprintId() == "simple_storage"); // HACK for testing

    if (isWorkshop) {
      float d = Vector3Distance(position, b->getPosition());
      if (d < minDst) {
        minDst = d;
        nearest = b;
      }
    }
  }
  return nearest;
}
Bush *Settler::FindNearestFood(const std::vector<Bush *> &bushes) {

  Bush *nearest = nullptr;

  float minDst = 10000.0f;

  for (auto *b : bushes) {

    if (b->hasFruit) {

      float d = Vector3Distance(position, b->position);

      if (d < minDst) {

        minDst = d;

        nearest = b;
      }
    }
  }

  return nearest;
}
void Settler::UpdateChopping(float deltaTime) {

  if (!m_currentTree || !m_currentTree->isActive()) {

    m_state = SettlerState::IDLE;

    return;
  }

  if (m_pendingReevaluation) {

    if (m_currentTree) {

      m_currentTree->releaseReservation();
    }

    m_pendingReevaluation = false;

    InterruptCurrentAction();

    return;
  }

  // OBLICZ ROTACJĘ W KIERUNKU DRZEWA
  Vector3 treePos = m_currentTree->getPosition();
  Vector3 dirToTree = Vector3Subtract(treePos, position);
  if (Vector3Length(dirToTree) > 0.01f) {
    dirToTree = Vector3Normalize(dirToTree);
    float targetAngle = atan2f(dirToTree.x, dirToTree.z) * RAD2DEG;
    float angleDiff = targetAngle - m_rotation;
    while (angleDiff > 180)
      angleDiff -= 360;
    while (angleDiff < -180)
      angleDiff += 360;
    m_rotation += angleDiff * 15.0f * deltaTime; // Smooth rotation
  }

  m_gatherTimer += deltaTime;

  if (m_gatherTimer >= 1.0f) {

    m_gatherTimer = 0.0f;

    if (CheckForJobFlagActivation()) {

      if (m_currentTree) {

        m_currentTree->releaseReservation();
      }

      InterruptCurrentAction();

      return;
    }

    // AXE BONUS
    float chopPower = 10.0f;
    if (m_heldItem && m_heldItem->getDisplayName() == "Stone Axe") {
      chopPower = 20.0f;
    }

    float woodAmount = m_currentTree->harvest(chopPower);

    (void)woodAmount;

    if (m_currentTree->isStump()) {

      auto woodItem = std::make_unique<ResourceItem>("Wood", "Wood Log",
                                                     "Freshly chopped wood.");

      // No manual drop in Settler - Tree handles it via harvest()->chopDown()

      // AUTO-PICKUP logic
      // Always transition to PICKING_UP after chopping to ensure we grab the
      // log. Logic in UpdatePickingUp will invoke storage search if needed.
      m_state = SettlerState::PICKING_UP;
      m_gatherTimer = 0.0f; // Reset timer for pickup grace period
      m_targetPosition = m_currentTree->getPosition();
      std::cout << "[Settler] Tree chopped -> transitioning to PICKING_UP near "
                << m_targetPosition.x << "," << m_targetPosition.z << std::endl;

      m_currentTree->releaseReservation();
      m_currentTree = nullptr;
    }
  }
}

void Settler::UpdateMining(float deltaTime) {
  if (!m_currentResourceNode || !m_currentResourceNode->isActive()) {
    m_state = SettlerState::IDLE;
    return;
  }

  if (m_pendingReevaluation) {
    if (m_currentResourceNode) {
      m_currentResourceNode->releaseReservation();
    }
    m_pendingReevaluation = false;
    InterruptCurrentAction();
    return;
  }

  // OBLICZ ROTACJĘ W KIERUNKU KAMIENIA
  Vector3 nodePos = m_currentResourceNode->getPosition();
  Vector3 dirToNode = Vector3Subtract(nodePos, position);
  if (Vector3Length(dirToNode) > 0.01f) {
    dirToNode = Vector3Normalize(dirToNode);
    float targetAngle = atan2f(dirToNode.x, dirToNode.z) * RAD2DEG;
    float angleDiff = targetAngle - m_rotation;
    while (angleDiff > 180)
      angleDiff -= 360;
    while (angleDiff < -180)
      angleDiff += 360;
    m_rotation += angleDiff * 15.0f * deltaTime; // Smooth rotation
  }

  m_gatherTimer += deltaTime;
  if (m_gatherTimer >= 1.0f) {
    m_gatherTimer = 0.0f;

    if (CheckForJobFlagActivation()) {
      if (m_currentResourceNode) {
        m_currentResourceNode->releaseReservation();
      }
      InterruptCurrentAction();
      return;
    }

    float minedAmount = m_currentResourceNode->harvest(10.0f);
    (void)minedAmount;
    // Debug logging for mining progress
    // std::cout << "[Settler] Mining tick. Remaining: " <<
    // m_currentResourceNode->getCurrentAmount() << std::endl;

    if (m_currentResourceNode->isDepleted()) {
      m_currentResourceNode->releaseReservation();

      // AUTO-PICKUP logic for Mining
      std::cout << "[Settler] Node depleted. Checking pickup conditions..."
                << std::endl;
      if (m_currentCraftTaskId != -1 || gatherStone) {
        m_state = SettlerState::PICKING_UP;
        m_gatherTimer = 0.0f; // Reset timer
        m_targetPosition = m_currentResourceNode->getPosition();
        std::cout << "[Settler] Node mined -> transitioning to PICKING_UP near "
                  << m_targetPosition.x << "," << m_targetPosition.z
                  << std::endl;
      } else {
        m_state = SettlerState::IDLE;
        std::cout
            << "[Settler] Node mined -> returning to IDLE (no pickup need?)"
            << std::endl;
      }

      m_currentResourceNode = nullptr;
    }
  }
}
void Settler::UpdateMovingToBed(float deltaTime) {
  // Jeśli mamy ścieżkę, poruszaj się po waypointach
  if (!m_currentPath.empty() &&
      m_currentPathIndex < (int)m_currentPath.size()) {
    Vector3 waypoint = m_currentPath[m_currentPathIndex];
    Vector3 direction = Vector3Subtract(waypoint, position);
    float distance = Vector3Length(direction);
    if (distance < 0.3f) { // osiągnięto waypoint
      m_currentPathIndex++;
      if (m_currentPathIndex >= (int)m_currentPath.size()) {
        // dotarliśmy do końca ścieżki
        m_currentPath.clear();
        m_currentPathIndex = 0;
        // przejdź do celu końcowego (m_targetPosition) bezpośrednio
        // (poniższa logika sprawdzi odległość do m_targetPosition)
      } else {
        // przejdź do następnego waypointa
        return;
      }
    } else {
      direction = Vector3Normalize(direction);
      Vector3 movement = Vector3Scale(direction, m_moveSpeed * deltaTime);
      position = Vector3Add(position, movement);
      return;
    }
  }
  // Bez ścieżki lub po jej zakończeniu: ruch bezpośredni do celu
  Vector3 direction = Vector3Subtract(m_targetPosition, position);
  float distance = Vector3Length(direction);
  if (distance < 0.5f) {
    m_state = SettlerState::SLEEPING;
    m_isMovingToCriticalTarget = false;
    return;
  }
  direction = Vector3Normalize(direction);
  Vector3 movement = Vector3Scale(direction, m_moveSpeed * deltaTime);
  position = Vector3Add(position, movement);
}
void Settler::CraftTool(const std::string &toolName) { (void)toolName; }
bool Settler::PickupItem(Item *item) {
  if (!item)
    return false;

  // Auto-equip if hand is empty and it's an equipment/tool item
  if (m_heldItem == nullptr && (item->getItemType() == ItemType::TOOL ||
                                item->getItemType() == ItemType::EQUIPMENT)) {
    // Create unique_ptr properly if we have raw pointer ownership... which is
    // tricky here. Usually PickupItem takes raw pointer from world? Looking at
    // signature: bool PickupItem(Item* item); If we just add to inventory,
    // inventory takes a COPY or moves it?
    // InventoryComponent::addItem(unique_ptr).
    // So this method signature is dangerous if it doesn't take ownership.

    // Let's assume this helper is called when we have the item.
    // Wait, UpdatePickingUp calls extract from colony?
    // Let's check UpdatePickingUp logic below.
    return false; // See UpdatePickingUp
  }
  // ...
  return false;
}
void Settler::DropItem(int slotIndex) { (void)slotIndex; }
void Settler::ignoreStorage(const std::string &storageId) {

  m_ignoredStorages.push_back(storageId);
}
bool Settler::isStorageIgnored(const std::string &storageId) const {

  for (const auto &id : m_ignoredStorages) {

    if (id == storageId)
      return true;
  }

  return false;
}
void Settler::ClearIgnoredStorages() { m_ignoredStorages.clear(); }
Vector3 Settler::myHousePos() const { return {0, 0, 0}; }
bool Settler::IsStateInterruptible() const {

  switch (m_state) {

  case SettlerState::IDLE:

  case SettlerState::WANDER:

  case SettlerState::WAITING:

    return true;

  case SettlerState::MOVING:

    return !m_isMovingToCriticalTarget;

  case SettlerState::EATING:

  case SettlerState::SLEEPING:

  case SettlerState::SEARCHING_FOR_FOOD:

  case SettlerState::MOVING_TO_FOOD:

  case SettlerState::MOVING_TO_BED:

  case SettlerState::CHOPPING:

  case SettlerState::BUILDING:

  case SettlerState::DEPOSITING:

  case SettlerState::CRAFTING:

  case SettlerState::GATHERING:

  case SettlerState::MINING:

  case SettlerState::HAULING:

  case SettlerState::PICKING_UP:

  case SettlerState::HUNTING:

  case SettlerState::MOVING_TO_STORAGE:

  case SettlerState::SKINNING:

  default:

    return false;
  }
}
bool Settler::CheckForJobFlagActivation() {

  bool flagJustActivated = false;

  if (gatherWood && !m_prevGatherWood)
    flagJustActivated = true;

  if (gatherStone && !m_prevGatherStone)
    flagJustActivated = true;

  if (gatherFood && !m_prevGatherFood)
    flagJustActivated = true;

  if (performBuilding && !m_prevPerformBuilding)
    flagJustActivated = true;

  if (huntAnimals && !m_prevHuntAnimals)
    flagJustActivated = true;

  if (craftItems && !m_prevCraftItems)
    flagJustActivated = true;

  if (haulToStorage && !m_prevHaulToStorage)
    flagJustActivated = true;

  if (tendCrops && !m_prevTendCrops)
    flagJustActivated = true;

  m_prevGatherWood = gatherWood;
  m_prevGatherStone = gatherStone;
  m_prevGatherFood = gatherFood;
  m_prevPerformBuilding = performBuilding;
  m_prevHuntAnimals = huntAnimals;
  m_prevCraftItems = craftItems;
  m_prevHaulToStorage = haulToStorage;
  m_prevTendCrops = tendCrops;

  return flagJustActivated;
}
void Settler::InterruptCurrentAction() {

  m_actionQueue.clear();

  m_currentPath.clear();

  m_currentPathIndex = 0;

  if (m_currentTree) {
    m_currentTree->releaseReservation();
    m_currentTree = nullptr;
  }

  m_currentGatherBush = nullptr;

  m_targetStorage = nullptr;

  m_targetFoodBush = nullptr;

  m_gatherTimer = 0.0f;

  m_isMovingToCriticalTarget = false;

  m_pendingReevaluation = false;

  // Hunting cleanup
  m_currentTargetAnimal = nullptr;
  m_attackCount = 0;
  m_huntingTimer = 0.0f;

  m_state = SettlerState::IDLE;
}
void Settler::OnJobConfigurationChanged() {

  m_prevGatherWood = gatherWood;

  m_prevGatherStone = gatherStone;

  m_prevGatherFood = gatherFood;

  m_prevPerformBuilding = performBuilding;

  m_prevHuntAnimals = huntAnimals;

  m_prevCraftItems = craftItems;

  m_prevHaulToStorage = haulToStorage;

  m_prevTendCrops = tendCrops;

  bool isSurvivalState = (m_state == SettlerState::EATING) ||
                         (m_state == SettlerState::SLEEPING) ||
                         (m_state == SettlerState::MOVING_TO_BED) ||
                         (m_state == SettlerState::MOVING_TO_FOOD);

  if (isSurvivalState) {
    m_pendingReevaluation = true;
    return;
  }

  if (m_state == SettlerState::CHOPPING && m_currentTree) {
    m_currentTree->releaseReservation();
  }

  InterruptCurrentAction();
}
void Settler::UpdateHauling(float deltaTime,
                            const std::vector<BuildingInstance *> &buildings,
                            std::vector<WorldItem> &worldItems) {
  (void)deltaTime;
  (void)buildings;

  // OPTIMIZATION: Only search every few frames or if not already picking up
  static float searchCooldown = 0.0f;
  searchCooldown -= deltaTime;
  if (searchCooldown > 0.0f)
    return;
  searchCooldown = 1.0f; // Search for items to haul once per second

  float minDist = 15.0f; // Reduced haul search radius for optimization
  WorldItem *targetItem = nullptr;

  for (auto &item : worldItems) {
    if (item.pendingRemoval)
      continue;
    if (!item.item)
      continue;
    if (item.item->getItemType() == ItemType::RESOURCE) {
      float d = Vector3Distance(position, item.position);
      if (d < minDist) {
        minDist = d;
        targetItem = &item;
      }
    }
  }

  if (targetItem) {
    MoveTo(targetItem->position);
    m_state = SettlerState::PICKING_UP;
  }
}

void Settler::UpdateHunting(
    float deltaTime, const std::vector<std::unique_ptr<Animal>> &animals,
    const std::vector<BuildingInstance *> &buildings,
    const std::vector<std::unique_ptr<ResourceNode>> &resourceNodes) {
  (void)buildings; // Może być użyte później dla pathfinding

  // Sprawdź czy przerwano zadanie (job flag deactivated)
  if (m_pendingReevaluation || !huntAnimals) {
    if (m_currentTargetAnimal) {
      m_currentTargetAnimal = nullptr;
    }
    m_pendingReevaluation = false;
    InterruptCurrentAction();
    return;
  }

  // Jeśli nie ma celu, znajdź najbliższe zwierzę
  if (!m_currentTargetAnimal) {
    Animal *nearest = nullptr;
    float minDist = 50.0f; // Promień wyszukiwania

    for (const auto &animalPtr : animals) {
      Animal *animal = animalPtr.get();
      if (!animal || !animal->isActive() || animal->isDead())
        continue;

      float dist = Vector3Distance(position, animal->getPosition());
      if (dist < minDist) {
        minDist = dist;
        nearest = animal;
      }
    }

    if (nearest) {
      m_currentTargetAnimal = nearest;
      m_attackCount = 0;
      m_targetPosition = nearest->getPosition();
      std::cout << "[Settler] " << m_name << " found prey: "
                << (nearest->getType() == AnimalType::RABBIT ? "Rabbit"
                                                             : "Deer")
                << " at distance " << minDist << std::endl;
    } else {
      // Brak zwierząt w zasięgu
      m_state = SettlerState::IDLE;
      std::cout << "[Settler] " << m_name << " - no animals found, going IDLE"
                << std::endl;
      m_state = SettlerState::IDLE;
      return;
    }
  }

  // VALIDATE CURRENT TARGET POINTER
  if (m_currentTargetAnimal) {
    bool stillExists = false;
    for (const auto &animalPtr : animals) {
      Animal *animal = animalPtr.get();
      if (animal == m_currentTargetAnimal) {
        stillExists = true;
        break;
      }
    }
    if (!stillExists) {
      std::cout << "[Settler] Target animal invalid/deleted. Forgetting."
                << std::endl;
      m_currentTargetAnimal = nullptr;
      m_state = SettlerState::IDLE;
      return;
    }
  }

  // Sprawdź czy zwierzę jeszcze żyje LUB czy jest to martwe ciało do
  // oskórowania If target is dead, we want to proceed to skinning IF state is
  // HUNTING. But if we are just looking for target, we verify isActive. If dead
  // AND !skinned, it isActive()==true.
  if (!m_currentTargetAnimal || !m_currentTargetAnimal->isActive()) {
    std::cout << "[Settler] Target animal disappeared/removed." << std::endl;
    m_state = SettlerState::IDLE;
    m_currentTargetAnimal = nullptr;
    return;
  }

  // DEBUG: Check for zero position ghost
  if (Vector3Length(m_currentTargetAnimal->getPosition()) < 0.1f) {
    // Suspect ghost at 0,0,0
    std::cout << "[Settler] Ignoring target at 0,0,0 (Ghost)." << std::endl;
    m_currentTargetAnimal = nullptr;
    return;
  }

  // If already dead (body), move to skinning logic
  if (m_currentTargetAnimal->isDead()) {
    // Move to body if far
    float d = Vector3Distance(position, m_currentTargetAnimal->getPosition());
    if (d > 1.0f) {
      MoveTo(m_currentTargetAnimal->getPosition());
      // Fix: Set state to MOVING_TO_SKIN so we don't go IDLE on arrival
      m_state = SettlerState::MOVING_TO_SKIN;
      return;
    } else {
      m_state = SettlerState::SKINNING;
      m_skinningTimer = 0.0f;
      std::cout << "[Settler] Arrived at body. Starting skinning." << std::endl;
      return;
    }
  }

  // Podążaj za zwierzęciem (aktualizuj cel)
  Vector3 animalPos = m_currentTargetAnimal->getPosition();
  float distToAnimal = Vector3Distance(position, animalPos);

  const float attackRange = 1.0f; // Atak z bliska (ale > detection 0.8f)

  // OBLICZ ROTACJĘ W KIERUNKU ZWIERZĘCIA
  Vector3 dirToAnimal = Vector3Subtract(animalPos, position);
  if (Vector3Length(dirToAnimal) > 0.01f) {
    dirToAnimal = Vector3Normalize(dirToAnimal);
    float targetAngle = atan2f(dirToAnimal.x, dirToAnimal.z) * RAD2DEG;
    float angleDiff = targetAngle - m_rotation;
    while (angleDiff > 180)
      angleDiff -= 360;
    while (angleDiff < -180)
      angleDiff += 360;
    m_rotation += angleDiff * 15.0f * deltaTime; // Smooth rotation
  }

  // ZAWSZE inkrementuj timer (ładuj atak podczas biegu)
  m_huntingTimer += deltaTime;

  // Debug log for distance
  static float logTimer = 0.0f;
  logTimer += deltaTime;
  if (logTimer > 1.0f) {
    Vector3 tPos = m_currentTargetAnimal->getPosition();
    std::cout << "[DEBUG HUNT] Target: "
              << (m_currentTargetAnimal->getType() == AnimalType::RABBIT
                      ? "Rabbit"
                      : "Deer")
              << " | Pos: (" << tPos.x << ", " << tPos.y << ", " << tPos.z
              << ")"
              << " | Dist: " << distToAnimal
              << " | Active: " << m_currentTargetAnimal->isActive()
              << " | Dead: " << m_currentTargetAnimal->isDead()
              << " | Skinned: "
              << m_currentTargetAnimal->isSkinned() // Need getter?
              << std::endl;
    logTimer = 0.0f;
  }

  // WEAPON CHECK
  bool hasSniperRifle = false;
  if (m_heldItem && m_heldItem->getDisplayName() == "Sniper Rifle") {
    hasSniperRifle = true;
  }

  float currentAttackRange = hasSniperRifle ? 15.0f : 1.0f;

  // LINE OF SIGHT CHECK (Sniper Only)
  bool hasLineOfSight = true;
  if (hasSniperRifle && distToAnimal <= currentAttackRange) {
    // Only check LOS if within range (optimization)
    Ray ray;
    ray.position = {position.x, position.y + 1.5f, position.z}; // Eye level
    Vector3 targetCenter = {animalPos.x, animalPos.y + 0.3f,
                            animalPos.z}; // Animal center estimate
    Vector3 diff = Vector3Subtract(targetCenter, ray.position);
    ray.direction = Vector3Normalize(diff);
    float distToTarget = Vector3Length(diff);

    for (const auto *building : buildings) {
      if (!building)
        continue;
      // Ignore storage ghosting?
      // If user says "don't shoot through walls", we MUST check storage.
      // But if we are IN storage (ghosting), we can't shoot out?
      // Let's assume hitting simple_storage blocks the shot.

      RayCollision col = GetRayCollisionBox(ray, building->getBoundingBox());
      if (col.hit) {
        // Check if hit is closer than target (and not behind)
        if (col.distance < distToTarget - 0.5f) { // -0.5 tolerance
          hasLineOfSight = false;
          break;
        }
      }
    }
  }

  if (distToAnimal > currentAttackRange ||
      (hasSniperRifle && !hasLineOfSight)) {
    // MOVEMENT PHASE: Use Pathfinding to reach target or better position

    // Recalculate path if target moved significantly (optimization to avoid
    // pathfinding every frame)
    if (Vector3Distance(m_targetPosition, animalPos) > 1.0f || !hasPath()) {
      MoveTo(animalPos);
      // MoveTo sets m_targetPosition to animalPos
    }

    // Use standard movement logic (respects walls, NavGrid)
    UpdateMovement(deltaTime, {}, buildings, resourceNodes);

    // Explicitly un-stick if stuck? UpdateMovement handles it slightly.
    // Also ensure rotation follows path, not locked to animal yet.

    // RESET AIM IF MOVING
    m_huntingTimer = 0.0f;
    m_attackAnimTimer = 0.0f; // Reset attack sequence
    m_waitingToDealDamage = false;

  } else {
    // AIMING / ATTACK PHASE
    // We are in range and have LOS.

    // Ensure we stop moving
    clearPath(); // Stop following path
    // m_state remains HUNTING.

    float attackDelay = hasSniperRifle ? 2.0f : 1.5f;

    // ... Rest of logic stays same ...
    if (m_huntingTimer >= attackDelay) {
      // TRIGGER ANIMACJI / STRZAŁU
      if (m_attackAnimTimer == 0.0f) {
        m_attackAnimTimer = 1.0f;
        m_waitingToDealDamage = true;

        m_huntingTimer = 0.0f;
        m_attackCount++;

        if (hasSniperRifle) {
          std::cout << "[Settler] " << m_name << " FIRES Sniper Rifle at "
                    << distToAnimal << "m!" << std::endl;
        } else {
          // Melee logic
        }
      }

      // Sprawdź czy zwierzę umarło
      if (m_currentTargetAnimal->isDead() && !m_waitingToDealDamage) {
        std::cout << "[Settler] Target eliminated. Moving to body for skinning."
                  << std::endl;
        m_targetPosition = m_currentTargetAnimal->getPosition();
        if (Vector3Distance(position, m_targetPosition) <= 1.0f) {
          m_state = SettlerState::SKINNING;
          m_skinningTimer = 0.0f;
        } else {
          MoveTo(m_targetPosition);
          m_state = SettlerState::MOVING_TO_SKIN;
        }
        m_attackCount = 0;
        m_huntingTimer = 0.0f;
        return;
      }
    } else {
      // AIMING PHASE (Sniper) or CHARGING (Melee)
      // Increment Timer (Aiming duration)
      m_huntingTimer += deltaTime;

      if (hasSniperRifle) {
        // Rotate to target
        Vector3 dir = Vector3Subtract(animalPos, position);
        float ang = atan2(dir.x, dir.z) * RAD2DEG;

        // Smooth rotation or Snap? Snap is fine for aiming.
        m_rotation = ang;
      }
    }
  }
} // End UpdateHunting

void Settler::UpdateMovingToSkin(
    float deltaTime, const std::vector<BuildingInstance *> &buildings,
    const std::vector<std::unique_ptr<ResourceNode>> &resourceNodes) {
  if (!m_currentTargetAnimal) {
    m_state = SettlerState::IDLE;
    return;
  }

  // Check if close enough
  if (Vector3Distance(position, m_currentTargetAnimal->getPosition()) <= 1.0f) {
    // Stop movement first (resets to IDLE)
    Stop();

    // THEN set state to SKINNING
    m_state = SettlerState::SKINNING;
    m_skinningTimer = 0.0f;
    std::cout << "[Settler] Arrived at body (MovingToSkin). Starting skinning."
              << std::endl;
    return;
  }

  // Update movement logic (path follow)
  UpdateMovement(deltaTime, {}, buildings, resourceNodes);

  // Also re-check state, because UpdateMovement might switch to IDLE if path
  // finished but we are slightly off? But Stop() sets IDLE. If UpdateMovement
  // finishes path, it calls Stop(). So we need to catch that.
  if (m_state == SettlerState::IDLE) {
    // If we went IDLE but still have target, maybe we are stuck or arrived?
    if (m_currentTargetAnimal &&
        Vector3Distance(position, m_currentTargetAnimal->getPosition()) <=
            1.5f) {
      m_state = SettlerState::SKINNING;
      m_skinningTimer = 0.0f;
    } else {
      // Failed to reach?
      std::cout << "[Settler] Failed to reach body (stuck?). Resetting."
                << std::endl;
      m_currentTargetAnimal = nullptr;
    }
  } else {
    // Force state back to MOVING_TO_SKIN in case UpdateMovement set it to
    // MOVING? UpdateMovement DOES NOT check m_state to set it MOVING. It
    // assumes it is called. It only sets IDLE on finish. So we are good, just
    // ensure we restart loop as MOVING_TO_SKIN if not finished.
    if (m_state != SettlerState::SKINNING)
      m_state = SettlerState::MOVING_TO_SKIN;
  }
}

void Settler::UpdateSkinning(float deltaTime) {
  if (!m_currentTargetAnimal) {
    m_state = SettlerState::IDLE;
    return;
  }

  // Check if body still exists (didn't disappear/removed by system?)
  // Note: Colony checks isActive. We modified isActive to return true if dead
  // but not skinned. However, we should be careful.

  m_skinningTimer += deltaTime;

  // Visuals: Maybe rotate settler to look at body?
  Vector3 targetPos = m_currentTargetAnimal->getPosition();
  Vector3 dir = Vector3Subtract(targetPos, position);
  if (Vector3Length(dir) > 0.01f) {
    dir = Vector3Normalize(dir);
    float targetAngle = atan2f(dir.x, dir.z) * RAD2DEG;
    float angleDiff = targetAngle - m_rotation;
    while (angleDiff > 180)
      angleDiff -= 360;
    while (angleDiff < -180)
      angleDiff += 360;
    m_rotation += angleDiff * 5.0f * deltaTime;
  }

  if (m_skinningTimer >= 2.0f) { // 2 seconds to skin
    std::cout << "[Settler] Skinning complete." << std::endl;

    // Mark animal as skinned (so it disappears from world)
    m_currentTargetAnimal->setSkinned(true);

    // Add meat to INVENTORY directly
    auto meatItem = std::make_unique<ConsumableItem>(
        "Raw Meat", "Fresh raw meat from hunt.");
    m_inventory.addItem(std::move(meatItem));
    std::cout << "[Settler] Collected Raw Meat in inventory." << std::endl;

    // Cleanup
    m_currentTargetAnimal = nullptr;
    m_state = SettlerState::IDLE;

    // Optional: Trigger Haul check immediately?
    // Logic in IDLE loop will pick up 'haulToStorage' task if we have
    // resources/food.
  }
}

// Player control methods implementation
void Settler::setPlayerControlled(bool controlled) {
  m_isPlayerControlled = controlled;
  if (controlled) {
    // Clear AI tasks when player takes control
    clearTasks();
    clearPath();
    m_state = SettlerState::IDLE;
    std::cout << "[Settler] " << m_name
              << " is now under player control (AI disabled)" << std::endl;
  } else {
    std::cout << "[Settler] " << m_name
              << " released from player control (AI re-enabled)" << std::endl;
  }
}

void Settler::setRotationFromMouse(float yaw) {
  m_rotation = yaw * RAD2DEG; // Silnik i rlRotatef oczekują stopni
}

Vector3 Settler::getForwardVector() const {
  float angleRad = m_rotation * DEG2RAD;
  // Raylib convention: 0 degrees is -Z (if sin/cos are used this way)
  // Based on original movement logic: forward = {-sin(rot), 0, -cos(rot)}
  return Vector3{-sinf(angleRad), 0.0f, -cosf(angleRad)};
}

void Settler::renderFPS() {
  // Render for the player settler in FPS
  bool hasSniperRifle =
      (m_heldItem && m_heldItem->getDisplayName() == "Sniper Rifle");
  bool hasAxe = (m_heldItem && m_heldItem->getDisplayName() == "Stone Axe");

  if (!hasSniperRifle && !hasAxe)
    return;

  Color skinColor = {255, 220, 177, 255};

  rlPushMatrix();

  // Position weapon in front of camera
  Vector3 forward = Vector3Normalize(
      Vector3Subtract(sceneCamera.target, sceneCamera.position));
  Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, {0, 1, 0}));
  Vector3 up = Vector3CrossProduct(right, forward);

  float adsFactor = m_adsLerp;
  float swingAnim = 0.0f;

  // Melee Swing Animation in FPS
  if (hasAxe && m_attackAnimTimer > 0.0f) {
    if (m_attackAnimTimer > 0.5f) {                  // Windup & Strike
      swingAnim = (1.0f - m_attackAnimTimer) * 2.0f; // 0 -> 1
    } else {                                         // Follow through & Reset
      swingAnim = m_attackAnimTimer * 2.0f;          // 1 -> 0
    }
  }

  // OFFSETS TUNING
  float rightOffset = (hasSniperRifle ? 0.35f : 0.45f) * (1.0f - adsFactor) -
                      (swingAnim * 0.2f);
  float upOffset = (hasSniperRifle ? -0.35f : -0.4f) * (1.0f - adsFactor) +
                   (-0.08f * adsFactor) - (swingAnim * 0.3f);
  float forwardOffset = (hasSniperRifle ? 0.7f : 0.5f) * (1.0f - adsFactor) +
                        (0.8f * adsFactor) + (swingAnim * 0.4f);

  Vector3 weaponBase =
      Vector3Add(sceneCamera.position, Vector3Scale(forward, forwardOffset));
  weaponBase = Vector3Add(weaponBase, Vector3Scale(up, upOffset));
  weaponBase = Vector3Add(weaponBase, Vector3Scale(right, rightOffset));

  rlTranslatef(weaponBase.x, weaponBase.y, weaponBase.z);

  // Rotate to match camera direction
  float yaw = atan2f(forward.x, forward.z) * RAD2DEG;
  float pitch = -asinf(forward.y) * RAD2DEG;

  rlRotatef(yaw, 0, 1, 0);
  rlRotatef(pitch - (swingAnim * 45.0f), 1, 0, 0);
  rlRotatef(swingAnim * 20.0f, 0, 0, 1);

  if (hasSniperRifle) {
    // Draw Rifle
    DrawCube({0, 0, 0.45f}, 0.05f, 0.05f, 1.0f, BLACK);
    DrawCube({0, -0.1f, -0.4f}, 0.08f, 0.15f, 0.6f, BROWN);

    // Hollow Scope
    float sw = 0.06f, sh = 0.06f, sd = 0.3f, st = 0.01f;
    Vector3 scopePos = {0, 0.08f, 0.1f};
    DrawCube({scopePos.x, scopePos.y + sh / 2, scopePos.z}, sw, st, sd,
             DARKGRAY);
    DrawCube({scopePos.x, scopePos.y - sh / 2, scopePos.z}, sw, st, sd,
             DARKGRAY);
    DrawCube({scopePos.x - sw / 2, scopePos.y, scopePos.z}, st, sh, sd,
             DARKGRAY);
    DrawCube({scopePos.x + sw / 2, scopePos.y, scopePos.z}, st, sh, sd,
             DARKGRAY);
  } else if (hasAxe) {
    // Draw Axe in FPS
    rlRotatef(90, 0, 1, 0); // Orient perpendicular to view
    DrawCube({0, 0, 0}, 0.5f, 0.04f, 0.04f, BROWN);          // Trzonek
    DrawCube({0.2f, 0.1f, 0}, 0.12f, 0.2f, 0.04f, DARKGRAY); // Ostrze
  }

  // Draw Hands (simplified for FPS)
  rlPushMatrix();
  rlTranslatef(0.1f, -0.1f, -0.2f);
  rlRotatef(-70, 1, 0, 0);
  DrawCube({0, 0, 0}, 0.08f, 0.3f, 0.08f, skinColor);
  rlPopMatrix();

  rlPopMatrix();
}
