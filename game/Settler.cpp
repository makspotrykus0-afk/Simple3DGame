#include "Settler.h"
#include "Tree.h"
#include "BuildingInstance.h"
#include "Colony.h" 
#include "../core/GameEngine.h"
#include "../core/GameSystem.h" // Added for GameSystem::getNavigationGrid
#include "../systems/BuildingSystem.h"
#include "../systems/StorageSystem.h"
#include "raymath.h"
#include "NavigationGrid.h"
#include <iostream>
#include <algorithm>
#include <string>

// Externs or access to systems
extern BuildingSystem* g_buildingSystem; 
// extern NavigationGrid* g_navigationGrid; // REMOVED: Using GameSystem::getNavigationGrid()

Settler::Settler(const std::string& name, const Vector3& pos, SettlerProfession profession)
    : GameEntity(name), m_name(name), m_profession(profession), m_state(SettlerState::IDLE),
      m_inventory(name, 50.0f), // Capacity 50
      m_stats(name, 100.0f, 100.0f, 100.0f, 100.0f),
      m_targetPosition(pos), m_moveSpeed(5.0f), m_rotation(0.0f),
      m_currentBuildTask(nullptr), m_currentGatherTask(nullptr), m_targetStorage(nullptr),
      m_gatherTimer(0.0f), m_gatherInterval(1.0f), m_isSelected(false) {
    
    position = pos;
    
    // Components
    addComponent(std::make_shared<PositionComponent>(position));
    // We could add inventory component to entity, but we have it as member. 
    // Ideally it should be a component in the list.
    
    // Start with some skills
    m_skills.addSkillXP(SkillType::BUILDING, 0.0f);
    m_skills.addSkillXP(SkillType::WOODCUTTING, 0);
}

Settler::~Settler() {
    if (m_currentGatherTask) delete m_currentGatherTask;
}

void Settler::render() {
    // Simple capsule/cylinder render
    Vector3 pos = position;
    pos.y += 1.0f; // Center
    
    Color color = BLUE;
    if (m_isSelected) color = GREEN;
    if (m_profession == SettlerProfession::BUILDER) color = YELLOW;
    if (m_profession == SettlerProfession::GATHERER) color = BROWN;
    
    DrawCapsule(Vector3{pos.x, pos.y - 0.8f, pos.z}, Vector3{pos.x, pos.y + 0.8f, pos.z}, 0.4f, 8, 8, color);
    
    // Draw face/direction
    Vector3 forward = { 0, 0, 1 };
    forward = Vector3RotateByAxisAngle(forward, {0, 1, 0}, m_rotation * DEG2RAD);
    Vector3 facePos = Vector3Add(pos, Vector3Scale(forward, 0.4f));
    facePos.y += 0.5f;
    DrawSphere(facePos, 0.1f, WHITE);
    
    // State text above head
    Vector3 textPos = pos;
    textPos.y += 1.5f;
    
    // Helper to draw 3D text (using default raylib 2D in 3D context or billboard)
    // For now, assume 2D overlay in main loop or skip specific text here
}

InteractionResult Settler::interact(GameEntity* player) {
    (void)player;
    return { true, "Hello, I am " + m_name + "." };
}

InteractionInfo Settler::getDisplayInfo() const {
    InteractionInfo info;
    info.objectName = m_name;
    info.objectDescription = "A settler of the colony.";
    info.availableActions.push_back(GetStateString());
    return info;
}

std::string Settler::GetStateString() const {
    switch (m_state) {
        case SettlerState::IDLE: return "Idle";
        case SettlerState::MOVING: return "Moving";
        case SettlerState::GATHERING: return "Gathering";
        case SettlerState::BUILDING: return "Building";
        case SettlerState::SLEEPING: return "Sleeping";
        case SettlerState::HUNTING: return "Hunting";
        case SettlerState::HAULING: return "Hauling";
        case SettlerState::MOVING_TO_STORAGE: return "Moving to Storage";
        case SettlerState::DEPOSITING: return "Depositing";
        default: return "Unknown";
    }
}

void Settler::Update(float deltaTime, const std::vector<std::unique_ptr<Tree>>& trees, std::vector<WorldItem>& worldItems, const std::vector<Bush*>& bushes, const std::vector<BuildingInstance*>& buildings, const std::vector<std::unique_ptr<Animal>>& animals) {
    // Suppress unused warnings
    (void)worldItems;
    (void)bushes;
    (void)animals;

    // Update stats
    m_stats.update(deltaTime);
    
    // State Machine
    static SettlerState lastState = m_state;
    if (m_state != lastState) {
        std::cout << "Settler " << m_name << " STATE CHANGE: "
                  << (int)lastState << " -> " << (int)m_state << std::endl;
        lastState = m_state;
    }

    switch (m_state) {
        case SettlerState::IDLE:
            // AI Logic for idle
            if (m_profession == SettlerProfession::GATHERER && !m_currentGatherTask) {
                // Check if inventory is full first
                if (m_inventory.isFull()) {
                    BuildingInstance* storage = FindNearestStorage(buildings);
                    if (storage) {
                        m_targetStorage = storage;
                        MoveTo(storage->getPosition());
                        m_state = SettlerState::MOVING_TO_STORAGE;
                    }
                } else {
                    // Find nearest tree
                    float minDist = 100.0f;
                    Tree* nearest = nullptr;
                    for (const auto& tree : trees) {
                        if (tree->isActive() && !tree->isReserved() && !tree->isStump()) {
                            float d = Vector3Distance(position, tree->getPosition());
                            if (d < minDist) {
                                minDist = d;
                                nearest = tree.get();
                            }
                        }
                    }
                    if (nearest) {
                        AssignToChop(nearest);
                    }
                }
            }
            break;
            
        case SettlerState::MOVING:
            UpdateMovement(deltaTime, trees, buildings);
            break;
            
        case SettlerState::GATHERING:
            UpdateGathering(deltaTime, worldItems, buildings);
            break;
            
        case SettlerState::BUILDING:
            UpdateBuilding(deltaTime);
            break;
            
        case SettlerState::SLEEPING:
            UpdateSleeping(deltaTime);
            break;

        case SettlerState::HAULING:
            UpdateHauling(deltaTime, buildings, worldItems);
            break;
            
        case SettlerState::MOVING_TO_STORAGE:
            UpdateMovingToStorage(deltaTime, buildings);
            break;
            
        case SettlerState::DEPOSITING:
            UpdateDepositing(deltaTime);
            break;
            
        default:
            break;
    }
    
    // Sync position
    auto posComp = getComponent<PositionComponent>();
    if (posComp) posComp->setPosition(position);
    
    // Set action state string for UI
    actionState = GetStateString();
}

void Settler::MoveTo(Vector3 destination) {
    // Hysteresis: If we are already DEPOSITING and close enough to storage, don't switch back to MOVING unless target changed significantly
    if (m_state == SettlerState::DEPOSITING && m_targetStorage) {
        float dist = Vector3Distance(position, m_targetStorage->getPosition());
        float hysteresisThreshold = 3.5f; // Larger than arrival threshold (2.5f)
        
        // If destination is essentially the storage position
        if (Vector3Distance(destination, m_targetStorage->getPosition()) < 1.0f) {
             if (dist < hysteresisThreshold) {
                 return; // Stay in DEPOSITING
             }
        }
    }

    m_targetPosition = destination;
    m_state = SettlerState::MOVING;
    
    // Pathfinding
    NavigationGrid* grid = GameSystem::getNavigationGrid();
    if (grid) {
        m_currentPath = grid->FindPath(position, m_targetPosition);
        m_currentPathIndex = 0;
    } else {
        m_currentPath.clear();
        m_currentPath.push_back(m_targetPosition);
        m_currentPathIndex = 0;
    }
}

void Settler::Stop() {
    m_state = SettlerState::IDLE;
    m_currentPath.clear();
}

void Settler::UpdateMovement(float deltaTime, const std::vector<std::unique_ptr<Tree>>& trees, const std::vector<BuildingInstance*>& buildings) {
    (void)trees;
    (void)buildings;

    if (m_currentPath.empty()) {
        // Reached end or no path
        float dist = Vector3Distance(position, m_targetPosition);
        float arrivalThreshold = 0.5f;
        
        // Increase threshold if target is a storage building to account for size
        if (m_state == SettlerState::MOVING_TO_STORAGE && m_targetStorage) {
            if (m_targetStorage->getBlueprintId() == "simple_storage") {
                arrivalThreshold = 2.5f; // Adjusted for 3x3 size (half size 1.5 + some margin)
            } else {
                arrivalThreshold = 2.0f; // General storage buffer
            }
        }

        if (dist < arrivalThreshold) {
             // Arrived
             // If we were moving to a task, switch state
             if (m_currentGatherTask && m_currentGatherTask->getState() == GatheringTask::GatheringState::MOVING_TO_TARGET) {
                 m_state = SettlerState::GATHERING;
                 m_currentGatherTask->setState(GatheringTask::GatheringState::GATHERING);
             } else if (m_currentBuildTask) {
                 m_state = SettlerState::BUILDING;
             } else if (m_targetStorage) { // Moving to storage
                 m_state = SettlerState::DEPOSITING;
                 // m_targetStorage = nullptr; // Do NOT clear target ref, we need it for depositing
             } else {
                 m_state = SettlerState::IDLE;
             }
        } else {
             // Move directly if close
             Vector3 direction = Vector3Subtract(m_targetPosition, position);
             direction = Vector3Normalize(direction);
             position = Vector3Add(position, Vector3Scale(direction, m_moveSpeed * deltaTime));
             
             // Face direction
             m_rotation = atan2f(direction.x, direction.z) * RAD2DEG;
        }
        return;
    }
    
    // Follow path
    Vector3 nextPos = m_currentPath[m_currentPathIndex];
    
    // Check if close to next node
    if (Vector3Distance(position, nextPos) < 0.2f) {
        m_currentPathIndex++;
        if (m_currentPathIndex >= m_currentPath.size()) {
            m_currentPath.clear();
            return;
        }
        nextPos = m_currentPath[m_currentPathIndex];
    }
    
    Vector3 direction = Vector3Subtract(nextPos, position);
    direction = Vector3Normalize(direction);
    
    // Rotation
    float targetAngle = atan2f(direction.x, direction.z) * RAD2DEG;
    m_rotation = Lerp(m_rotation, targetAngle, 5.0f * deltaTime); // Smooth turn
    
    // Move
    Vector3 nextStep = Vector3Add(position, Vector3Scale(direction, m_moveSpeed * deltaTime));
    
    // Simple local avoidance (optional, if not relying purely on grid)
    // For now, trust the grid/path
    position = nextStep;
}

void Settler::assignTask(TaskType type, GameEntity* target, Vector3 pos) {
    // Stop current task
    Stop();
    
    if (type == TaskType::MOVE) {
        MoveTo(pos);
    } else if (type == TaskType::BUILD) {
        // Try to find build task near pos
        if (g_buildingSystem) {
            BuildTask* task = g_buildingSystem->getBuildTaskAt(pos, 2.0f);
            if (task) {
                AssignBuildTask(task);
            } else {
                MoveTo(pos);
            }
        }
    } else if (type == TaskType::GATHER) {
        AssignToChop(target);
    }
}

void Settler::AssignBuildTask(BuildTask* task) {
    if (!task) return;
    m_currentBuildTask = task;
    MoveTo(task->getPosition());
}

void Settler::UpdateBuilding(float deltaTime) {
    if (!m_currentBuildTask) {
        m_state = SettlerState::IDLE;
        return;
    }
    
    if (Vector3Distance(position, m_currentBuildTask->getPosition()) > 3.0f) {
        // Re-evaluate move if too far
        MoveTo(m_currentBuildTask->getPosition());
        return;
    }
    
    // Construct
    if (m_currentBuildTask->isCompleted()) {
        m_currentBuildTask = nullptr;
        m_state = SettlerState::IDLE;
        return;
    }
    
    // Add progress
    float buildPower = 10.0f + (m_skills.getSkillLevel(SkillType::BUILDING) * 2.0f);
    m_currentBuildTask->advanceConstruction(buildPower * deltaTime);
}

void Settler::AssignToChop(GameEntity* tree) {
    if (!tree) return;
    
    // Create gathering task
    if (m_currentGatherTask) {
        delete m_currentGatherTask;
    }
    // Create gathering task
    if (m_currentGatherTask) {
        delete m_currentGatherTask;
    }
    m_currentGatherTask = new GatheringTask(tree);
    m_currentGatherTask->setState(GatheringTask::GatheringState::MOVING_TO_TARGET);
    
    // Reserve tree
    Tree* t = dynamic_cast<Tree*>(tree);
    if (t) {
        t->reserve(m_name);
    }
    
    // Start moving
    MoveTo(tree->getPosition());
}
void Settler::ForceGatherTarget(GameEntity* target) {
    if (!target) return;
    
    // Clear any existing tasks
    Stop();
    if (m_currentGatherTask) {
        delete m_currentGatherTask;
        m_currentGatherTask = nullptr;
    }
    if (m_currentBuildTask) {
        m_currentBuildTask = nullptr;
    }
    
    // For now, only support Trees for gathering
    Tree* tree = dynamic_cast<Tree*>(target);
    if (tree) {
        m_profession = SettlerProfession::GATHERER; // Force profession if needed, or just do it
        m_targetStorage = nullptr; // Clear storage target
        m_state = SettlerState::IDLE; // Reset state briefly to ensure clean transition
        AssignToChop(tree);
        
        // Force state update to ensure we are moving immediately and UI updates
        if (m_currentGatherTask) {
            m_state = SettlerState::MOVING;
            // Reset timers
            m_gatherTimer = 0.0f;
        }
    } else {
        // Maybe handle other resource types later (Bush, Rock)
        MoveTo(target->getPosition());
    }
}

void Settler::UpdateGathering(float deltaTime, std::vector<WorldItem>& worldItems, const std::vector<BuildingInstance*>& buildings) {
    (void)worldItems; // Unused

    if (!m_currentGatherTask) {
        m_state = SettlerState::IDLE;
        return;
    }
    
    GameEntity* target = m_currentGatherTask->getTarget();
    Tree* treeTarget = dynamic_cast<Tree*>(target);
    
    if (!target || (treeTarget && !treeTarget->isActive())) {
        // Target gone
        if (treeTarget) treeTarget->releaseReservation();
        delete m_currentGatherTask;
        m_currentGatherTask = nullptr;
        m_state = SettlerState::IDLE;
        return;
    }
    
    // Check distance
    float dist = Vector3Distance(position, target->getPosition());
    float interactRange = 2.0f; 
    
    if (dist > interactRange) {
        // Should be moving
        MoveTo(target->getPosition());
        return;
    }
    
    // We are close, gather
    // Face target
    Vector3 direction = Vector3Subtract(target->getPosition(), position);
    m_rotation = atan2f(direction.x, direction.z) * RAD2DEG;
    
    m_gatherTimer += deltaTime;
    if (m_gatherTimer >= m_gatherInterval) {
        m_gatherTimer = 0.0f;
        
        // Harvest
        if (treeTarget) {
            float gatherSkill = m_skills.getSkillLevel(SkillType::WOODCUTTING);
            float harvestAmount = 10.0f + (gatherSkill * 2.0f); // Increased base and multiplier for faster gathering
            
            float actualHarvested = treeTarget->harvest(harvestAmount);
            
            if (actualHarvested > 0) {
                // Create item and add to inventory
                std::unique_ptr<Item> woodItem = std::make_unique<ResourceItem>("Wood", "Wood Log", "A resource for building.");
                if (!m_inventory.addItem(std::move(woodItem), (int)actualHarvested)) {
                    // Inventory full!
                    printf("Inventory full! Moving to storage.\n");
                    
                    // Release tree reservation so others can take it
                    treeTarget->releaseReservation();
                    
                    // Switch state to deposit
                    BuildingInstance* storage = FindNearestStorage(buildings);
                    if (storage) {
                        m_targetStorage = storage;
                        MoveTo(storage->getPosition());
                        m_state = SettlerState::MOVING_TO_STORAGE; // Override MoveTo's MOVING state
                    } else {
                        // No storage? Just drop items or Idle?
                        printf("No storage found!\n");
                        m_state = SettlerState::IDLE; // Or just stand there full
                    }
                    return; 
                }
                
                // Gain XP
                m_skills.addSkillXP(SkillType::WOODCUTTING, actualHarvested * 2.0f);
            }
            
            // If tree depleted
            if (treeTarget->isStump() || treeTarget->getWoodAmount() <= 0) {
                 treeTarget->releaseReservation();
                 delete m_currentGatherTask;
                 m_currentGatherTask = nullptr;
                 
                 // If we have items, maybe go deposit even if not full? 
                 // Or just find next tree.
                 // Let's find next tree (IDLE logic will pick it up)
                 m_state = SettlerState::IDLE;
            }
        }
    }
}

void Settler::UpdateMovingToStorage(float deltaTime, const std::vector<BuildingInstance*>& buildings) {
    // Verify target still exists
    bool exists = false;
    if (m_targetStorage) {
         for(auto b : buildings) if(b == m_targetStorage) exists = true;
    }
    
    if (!m_targetStorage || !exists) {
         m_targetStorage = FindNearestStorage(buildings);
         if (!m_targetStorage) {
             m_state = SettlerState::IDLE;
             return;
         }
    }

    float dist = Vector3Distance(position, m_targetStorage->getPosition());
    float arrivalThreshold = 2.5f;
    if (m_targetStorage->getBlueprintId() != "simple_storage") arrivalThreshold = 2.0f;

    // Check if close enough to deposit
    if (dist < arrivalThreshold) {
        std::cout << "Settler " << m_name << " Arrived at Storage. Dist: " << dist << " Threshold: " << arrivalThreshold << std::endl;
        m_state = SettlerState::DEPOSITING;
        m_currentPath.clear();
        return;
    } else {
         // Debug log to see why not arriving
         // std::cout << "Settler " << m_name << " Moving to Storage. Dist: " << dist << " Threshold: " << arrivalThreshold << std::endl;
    }
    
    // If not close enough, ensure we are moving towards it
    if (Vector3Distance(m_targetPosition, m_targetStorage->getPosition()) > 0.5f || m_currentPath.empty()) {
        MoveTo(m_targetStorage->getPosition());
        m_state = SettlerState::MOVING_TO_STORAGE; // Force state back to MOVING_TO_STORAGE as MoveTo sets MOVING
    }

    UpdateMovement(deltaTime, {}, buildings);
}

void Settler::UpdateDepositing(float deltaTime) {
    (void)deltaTime;

    if (!m_targetStorage) {
        m_state = SettlerState::IDLE;
        return;
    }

    // === LAZY REPAIR CHECK START ===
    std::string storageId = m_targetStorage->getStorageId();
    if (storageId.empty()) {
        std::cout << "[Settler] UpdateDepositing: Detected empty storageId. Attempting lazy repair." << std::endl;
        
        auto* storageSystem = GameEngine::getInstance().getSystem<StorageSystem>();
        if (storageSystem) {
            // Assuming createStorage signature is (StorageType type, const std::string& owner)
            // Since previous attempt with (StorageType, int) failed.
            // We don't have explicit capacity param in one overload? Or maybe it's owner.
            // Checking error log: "cannot convert argument 2 from 'int' to 'const std::string &'"
            // It expects an owner string as second arg.
            std::string newId = storageSystem->createStorage(StorageType::WAREHOUSE, "Colony");
            if (!newId.empty()) {
                m_targetStorage->setStorageId(newId);
                std::cout << "[Settler] Lazy repair SUCCESS. Assigned new StorageID: " << newId << std::endl;
                // Update local variable
                storageId = newId;
            } else {
                 std::cout << "[Settler] Lazy repair FAILED. Could not create storage." << std::endl;
                 // Drop items to break loop
                 m_inventory.clear();
                 m_state = SettlerState::IDLE;
                 m_targetStorage = nullptr;
                 return;
            }
        } else {
            std::cout << "[Settler] Lazy repair FAILED. StorageSystem not found." << std::endl;
             // Drop items to break loop
             m_inventory.clear();
             m_state = SettlerState::IDLE;
             m_targetStorage = nullptr;
             return;
        }
    }
    // === LAZY REPAIR CHECK END ===

    float dist = Vector3Distance(position, m_targetStorage->getPosition());
    float arrivalThreshold = 2.5f;
    if (m_targetStorage->getBlueprintId() != "simple_storage") arrivalThreshold = 2.0f;
    float hysteresisBuffer = 1.5f;

    // Only move back if significantly far away
    if (dist > arrivalThreshold + hysteresisBuffer) {
        std::cout << "Settler " << m_name << " Too far from storage during deposit. Dist: " << dist
                  << " Allowed: " << (arrivalThreshold + hysteresisBuffer) << " -> Resetting to MOVING" << std::endl;
        m_state = SettlerState::MOVING_TO_STORAGE;
        return;
    }

    auto* storageSystem = GameEngine::getInstance().getSystem<StorageSystem>();
    if (!storageSystem) {
        m_state = SettlerState::IDLE;
        return;
    }

    // Double check (should be fixed by lazy repair above)
    storageId = m_targetStorage->getStorageId();
    if (storageId.empty()) {
         m_state = SettlerState::IDLE;
         m_targetStorage = nullptr;
         return;
    }

    const auto& items = m_inventory.getItems();
    if (items.empty()) {
        m_state = SettlerState::IDLE;
        m_targetStorage = nullptr;
        return;
    }

    bool anyDeposited = false;
    bool inventoryFullFailure = false;
    
    // Simple logic: try to deposit everything that is a resource
    for (const auto& invItem : items) {
        if (invItem && invItem->item) {
            Resources::ResourceType type = Resources::ResourceType::None;
            std::string name = invItem->item->getDisplayName();
            
            // Robust mapping for game resource names (e.g. "Wood Log", "Raw Meat")
            if (name == "Wood" || name == "Wood Log") type = Resources::ResourceType::Wood;
            else if (name == "Stone" || name == "Stone Chunk") type = Resources::ResourceType::Stone;
            else if (name == "Metal" || name == "Iron Ore" || name == "Metal Ore") type = Resources::ResourceType::Metal;
            else if (name == "Gold" || name == "Gold Ore") type = Resources::ResourceType::Gold;
            else if (name == "Food" || name == "Raw Meat" || name == "Meat" || name == "Berries") type = Resources::ResourceType::Food;

            std::cout << "Settler " << m_name << " trying to deposit " << name
                      << " (" << (int)type << ") amount: " << invItem->quantity
                      << " to " << storageId << std::endl;

            if (type != Resources::ResourceType::None) {
                int amount = invItem->quantity;
                int added = storageSystem->addResourceToStorage(storageId, "Colony", type, amount);
                
                std::cout << "  -> Result: added " << added << "/" << amount << std::endl;

                if (added > 0) {
                    anyDeposited = true;
                } else {
                    inventoryFullFailure = true;
                    std::cout << "  -> Failed to add item (Full or Error)" << std::endl;
                }
            } else {
                std::cout << "  -> Item type mapping failed for " << name << std::endl;
            }
        }
    }
    
    if (anyDeposited) {
         m_inventory.clear();
         m_state = SettlerState::IDLE;
         m_targetStorage = nullptr;
    } else if (inventoryFullFailure) {
         // Storage full or failure, drop items to avoid infinite loop (State Thrashing Fix)
         std::cout << "Settler " << m_name << ": Storage full or invalid. Clearing inventory to prevent infinite loop." << std::endl;
         
         // Fallback: Clear inventory to ensure settler becomes free
         m_inventory.clear();
         
         m_state = SettlerState::IDLE;
         m_targetStorage = nullptr;
    } else {
        // No deposit-able items found
        std::cout << "Settler " << m_name << ": No deposit-able items found (Type mismatch?), resetting to IDLE but might loop if full." << std::endl;
        m_state = SettlerState::IDLE;
        m_targetStorage = nullptr;
    }
}

BuildingInstance* Settler::FindNearestStorage(const std::vector<BuildingInstance*>& buildings) {
    BuildingInstance* nearest = nullptr;
    float minDist = 10000.0f;
    
    for (auto* b : buildings) {
        // Check if building is storage.
        const BuildingBlueprint* bp = b->getBlueprint();
        bool isStorage = false;
        
        if (bp) {
            if (bp->getCategory() == BuildingCategory::STORAGE) isStorage = true;
            if (bp->getName() == "storehouse" || bp->getName() == "Warehouse" ||
                bp->getName() == "Stockpile" || bp->getName() == "Town Hall") isStorage = true;
        }
        
        if (b->getBlueprintId() == "simple_storage") isStorage = true;
        
        if (isStorage) {
            float d = Vector3Distance(position, b->getPosition());
            if (d < minDist) {
                minDist = d;
                nearest = b;
            }
        }
    }
    
    if (nearest) {
        // === LAZY REPAIR CHECK START ===
        if (nearest->getStorageId().empty()) {
             std::cout << "[Settler] FindNearestStorage: Found storage but ID is empty. Attempting lazy repair for blueprint: " << nearest->getBlueprintId() << std::endl;
             auto* storageSystem = GameEngine::getInstance().getSystem<StorageSystem>();
             if (storageSystem) {
                 std::string newId = storageSystem->createStorage(StorageType::WAREHOUSE, "Colony");
                 if (!newId.empty()) {
                     nearest->setStorageId(newId);
                     std::cout << "[Settler] Lazy repair SUCCESS. Assigned new StorageID: " << newId << std::endl;
                 } else {
                     std::cout << "[Settler] Lazy repair FAILED. Could not create storage." << std::endl;
                 }
             }
        }
        // === LAZY REPAIR CHECK END ===
        
        std::cout << "FindNearestStorage for " << m_name << ": Found "
                  << nearest->getBlueprintId() << " at dist " << minDist
                  << " StorageID: [" << nearest->getStorageId() << "]" << std::endl;
    } else {
        std::cout << "FindNearestStorage for " << m_name << ": NONE FOUND" << std::endl;
    }
    
    return nearest;
}

void Settler::UpdateHauling(float deltaTime, const std::vector<BuildingInstance*>& buildings, std::vector<WorldItem>& worldItems) {
    (void)deltaTime;
    (void)buildings;
    (void)worldItems;
    // Fallback simple hauling (picking up loose items)
    // ... (Existing logic or simplified)
    m_state = SettlerState::IDLE;
}

void Settler::UpdateSleeping(float deltaTime) {
    m_stats.modifyEnergy(20.0f * deltaTime);
    if (m_stats.getCurrentEnergy() >= 99.0f) {
        m_state = SettlerState::IDLE;
        // Teleport outside house
        if (m_assignedBed) {
            if (auto door = m_assignedBed->getDoor()) {
                position = door->getPosition();
            } else {
                position = Vector3Add(m_assignedBed->getPosition(), {2.0f, 0, 0});
            }
        }
    }
}

// Housing
void Settler::assignBed(BuildingInstance* bed) {
    m_assignedBed = bed;
}

Vector3 Settler::myHousePos() const {
    if (m_assignedBed) return m_assignedBed->getPosition();
    return {0,0,0};
}

// Inventory stubs
bool Settler::PickupItem(Item* item) {
    if (!item) return false;
    auto clonedItem = item->clone();
    return m_inventory.addItem(std::move(clonedItem));
}

void Settler::DropItem(int slotIndex) {
    m_inventory.dropItem(slotIndex, 1);
}
