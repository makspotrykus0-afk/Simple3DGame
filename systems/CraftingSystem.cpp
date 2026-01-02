#include "CraftingSystem.h"
#include "../core/GameEngine.h"
#include "../core/GameSystem.h"
#include "../game/Colony.h"
#include "StorageSystem.h"
#include "../systems/BuildingSystem.h"
#include "../systems/ResourceTypes.h"
#include "../game/Terrain.h"
#include "../game/ResourceNode.h"
#include <iostream>
#include <algorithm>
#include <unordered_set>

extern BuildingSystem* g_buildingSystem;

// Helper to convert string to ResourceType
Resources::ResourceType StringToResourceType(const std::string& typeStr) {
    if (typeStr == "Wood") return Resources::ResourceType::Wood;
    if (typeStr == "Stone") return Resources::ResourceType::Stone;
    if (typeStr == "Food") return Resources::ResourceType::Food;
    if (typeStr == "Metal") return Resources::ResourceType::Metal;
    if (typeStr == "Gold") return Resources::ResourceType::Gold;
    return Resources::ResourceType::None;
}

// Constructor is defined in header or uses default

void CraftingSystem::initialize() {
    // Rejestracja podstawowych receptur
    // Kamienny nóż (Stone Knife)
    CraftingRecipe stoneKnife;
    stoneKnife.id = "stone_knife";
    stoneKnife.name = "Stone Knife";
    stoneKnife.description = "A primitive knife made of stone.";
    stoneKnife.craftingTime = 3.0f;
    stoneKnife.resultItemId = "Stone Knife";
    stoneKnife.resultType = ItemType::TOOL; // Use TOOL if available, else EQUIPMENT
    stoneKnife.resultAmount = 1;
    stoneKnife.ingredients.push_back({"Wood", 1});
    stoneKnife.ingredients.push_back({"Stone", 1});
    registerRecipe(stoneKnife);

    // Kamienna siekiera (Stone Axe)
    CraftingRecipe stoneAxe;
    stoneAxe.id = "stone_axe";
    stoneAxe.name = "Stone Axe";
    stoneAxe.description = "A rudimentary axe for chopping wood faster.";
    stoneAxe.craftingTime = 5.0f;
    stoneAxe.resultItemId = "Stone Axe";
    stoneAxe.resultType = ItemType::TOOL; 
    stoneAxe.resultAmount = 1;
    stoneAxe.ingredients.push_back({"Wood", 3});
    stoneAxe.ingredients.push_back({"Stone", 2});
    registerRecipe(stoneAxe);

    // Receptura "knife" (alias dla stone_knife, zgodnie ze specyfikacją)
    CraftingRecipe knife;
    knife.id = "knife";
    knife.name = "Knife";
    knife.description = "A sharp knife for hunting.";
    knife.craftingTime = 3.0f;
    knife.resultItemId = "Knife";
    knife.resultType = ItemType::TOOL;
    knife.resultAmount = 1;
    knife.ingredients.push_back({"Wood", 1});
    knife.ingredients.push_back({"Stone", 1});
    registerRecipe(knife);

    // Subskrybuj event zmiany ekwipunku
    EVENT_BUS.registerHandler<InventoryChangedEvent>(
        [this](const std::any& eventData) {
            const auto& event = std::any_cast<const InventoryChangedEvent&>(eventData);
            this->onInventoryChanged(event);
        },
        "CraftingSystem"
    );
    std::cout << "CraftingSystem initialized." << std::endl;
}

void CraftingSystem::update(float deltaTime) {
    // Update crafting progress
    for (auto& task : m_activeTasks) {
        task.progress += deltaTime / 5.0f; // dummy progress
        if (task.progress >= 1.0f) {
            // task completed automatically? maybe not
        }
    }

    // Periodic check for pending crafts (fallback when events are missing)
    m_resumeCheckTimer += deltaTime;
    if (m_resumeCheckTimer >= m_resumeCheckInterval) {
        m_resumeCheckTimer = 0.f;
        if (!m_pendingCrafts.empty()) {
            // Collect unique settler IDs to avoid duplicate checks
            std::unordered_set<std::string> settlerIds;
            for (const auto& kv : m_pendingCrafts) {
                settlerIds.insert(kv.second.settlerId);
            }
            for (const auto& settlerId : settlerIds) {
                checkAndResumeCraft(settlerId);
            }
        }
    }
}

void CraftingSystem::render() {
    // empty for now
}

void CraftingSystem::shutdown() {
    // cleanup if needed
}

void CraftingSystem::registerRecipe(const CraftingRecipe& recipe) {
    if (m_recipesById.find(recipe.id) == m_recipesById.end()) {
        m_recipesById[recipe.id] = recipe;
        m_recipeList.push_back(recipe);
    }
}

int CraftingSystem::queueTask(const std::string& recipeId, const std::string& targetSettlerId) {
    if (m_recipesById.find(recipeId) == m_recipesById.end()) {
        std::cerr << "CraftingSystem: Unknown recipe ID " << recipeId << std::endl;
        return -1;
    }
    int id = m_nextTaskId++;
    m_taskQueue.emplace_back(id, recipeId);
    m_taskQueue.back().targetSettlerId = targetSettlerId;
    std::cout << "CraftingSystem: Queued task " << id << " for recipe '" << recipeId << "'";
    if (!targetSettlerId.empty()) {
        std::cout << " (Target Settler: " << targetSettlerId << ")";
    }
    std::cout << std::endl;
    return id;
}

CraftingTask* CraftingSystem::getAvailableTask(const std::string& settlerId) {
    // Debug log if queue not empty (once per call)
    if (!m_taskQueue.empty()) {
        std::cout << "[CraftingSystem] getAvailableTask called for settler '" << settlerId
                  << "', queue size=" << m_taskQueue.size() << std::endl;
    }
    for (auto it = m_taskQueue.begin(); it != m_taskQueue.end(); ++it) {
        if (!it->isStarted) {
            // Check if task is restricted to a specific settler
            if (!it->targetSettlerId.empty() && it->targetSettlerId != settlerId) {
                // std::cout << "[CraftingSystem] Task " << it->taskId << " skipped (Target: " << it->targetSettlerId << " != " << settlerId << ")" << std::endl;
                continue; // Skip this task, it's for someone else
            }

            // Remove the canCraft check - assign task even if ingredients missing
            std::cout << "[CraftingSystem] Assigned craft taskId=" << it->taskId
                      << " recipe=" << it->recipeId << " to settler=" << settlerId << std::endl;
            CraftingTask task = *it;
            task.assignedSettlerId = settlerId;
            task.isStarted = true;
            m_activeTasks.push_back(task);
            m_taskQueue.erase(it);
            return &m_activeTasks.back();
        }
    }
    return nullptr;
}

std::unique_ptr<Item> CraftingSystem::completeTask(int taskId) {
    auto it = std::find_if(m_activeTasks.begin(), m_activeTasks.end(),
                           [taskId](const CraftingTask& t) { return t.taskId == taskId; });
    if (it != m_activeTasks.end()) {
        std::string recipeId = it->recipeId;
        std::unique_ptr<Item> result = nullptr;
        auto recIt = m_recipesById.find(recipeId);
        if (recIt != m_recipesById.end()) {
            result = createItemFromRecipe(recIt->second);
        }
        m_activeTasks.erase(it);
        return result;
    }
    return nullptr;
}

void CraftingSystem::cancelTask(int taskId) {
    // Remove from active tasks
    auto it = std::find_if(m_activeTasks.begin(), m_activeTasks.end(),
                           [taskId](const CraftingTask& t) { return t.taskId == taskId; });
    if (it != m_activeTasks.end()) {
        m_activeTasks.erase(it);
        return;
    }
    // Remove from queue
    auto itq = std::find_if(m_taskQueue.begin(), m_taskQueue.end(),
                            [taskId](const CraftingTask& t) { return t.taskId == taskId; });
    if (itq != m_taskQueue.end()) {
        m_taskQueue.erase(itq);
    }
}

bool CraftingSystem::canCraft(const std::string& recipeId, const std::string& settlerId, bool silent) {
    auto recIt = m_recipesById.find(recipeId);
    if (recIt == m_recipesById.end()) {
        if (!silent) std::cout << "CraftingSystem: Recipe not found." << std::endl;
        return false;
    }
    const auto& recipe = recIt->second;

    // Check inventory of settler
    if (!settlerId.empty()) {
        Settler* targetSettler = nullptr;
        for (auto* settler : GameSystem::getColony()->getSettlers()) {
            if (settler->getName() == settlerId) {
                targetSettler = settler;
                break;
            }
        }
        if (targetSettler) {
            for (const auto& ing : recipe.ingredients) {
                int inventoryAmount = targetSettler->getInventory().getResourceAmount(ing.resourceType);
                if (inventoryAmount < ing.amount) {
                    if (!silent) std::cout << "CraftingSystem: Missing " << ing.amount - inventoryAmount << " " << ing.resourceType << " in inventory" << std::endl;
                    return false;
                }
            }
            return true;
        }
    }

    // If no settler specified, check storage
    auto storageSys = GameEngine::getInstance().getSystem<StorageSystem>();
    if (!storageSys) {
        if (!silent) std::cout << "CraftingSystem: StorageSystem not available." << std::endl;
        return false;
    }
    if (!g_buildingSystem) {
        if (!silent) std::cout << "CraftingSystem: BuildingSystem not available." << std::endl;
        return false;
    }
    auto buildings = g_buildingSystem->getBuildingsInRange(Vector3{0,0,0}, 100000.0f);
    for (const auto& ing : recipe.ingredients) {
        Resources::ResourceType resourceType = StringToResourceType(ing.resourceType);
        int total = 0;
        for (const auto& bld : buildings) {
            if (!bld->isBuilt()) continue;
            std::string storageId = bld->getStorageId();
            if (storageId.empty()) continue;
            total += storageSys->getResourceAmount(storageId, resourceType);
        }
        if (total < ing.amount) {
            if (!silent) std::cout << "CraftingSystem: Missing " << ing.amount - total << " " << ing.resourceType << " in storage" << std::endl;
            return false;
        }
    }
    return true;
}

bool CraftingSystem::consumeIngredients(const std::string& recipeId, const std::string& settlerId) {
    std::cout << "CraftingSystem: Consuming ingredients for recipe '" << recipeId << "' settler '" << settlerId << "'" << std::endl;
    auto recIt = m_recipesById.find(recipeId);
    if (recIt == m_recipesById.end()) {
        std::cout << "CraftingSystem: Recipe not found." << std::endl;
        return false;
    }
    const auto& recipe = recIt->second;

    auto storageSys = GameEngine::getInstance().getSystem<StorageSystem>();
    if (!storageSys) {
        std::cout << "CraftingSystem: StorageSystem not available." << std::endl;
        return false;
    }
    if (!g_buildingSystem) {
        std::cout << "CraftingSystem: BuildingSystem not available." << std::endl;
        return false;
    }
    auto buildings = g_buildingSystem->getBuildingsInRange(Vector3{0,0,0}, 100000.0f);

    for (const auto& ing : recipe.ingredients) {
        std::string resourceTypeStr = ing.resourceType;
        int required = ing.amount;
        Resources::ResourceType resourceType = StringToResourceType(resourceTypeStr);
        int remaining = required;

        // First try to consume from inventory (if settler has any)
        if (!settlerId.empty()) {
            Settler* targetSettler = nullptr;
            for (auto* settler : GameSystem::getColony()->getSettlers()) {
                if (settler->getName() == settlerId) {
                    targetSettler = settler;
                    break;
                }
            }
            if (targetSettler) {
                int inventoryAmount = targetSettler->getInventory().getResourceAmount(resourceTypeStr);
                int takeFromInventory = std::min(inventoryAmount, remaining);
                if (takeFromInventory > 0) {
                    if (targetSettler->getInventory().removeResource(resourceTypeStr, takeFromInventory)) {
                        std::cout << "CraftingSystem: Consumed " << takeFromInventory << " " << resourceTypeStr << " from inventory" << std::endl;
                        remaining -= takeFromInventory;
                    }
                }
            }
        }

        // If still needed, consume from storage
        for (const auto& bld : buildings) {
            if (!bld->isBuilt()) continue;
            std::string storageId = bld->getStorageId();
            if (storageId.empty()) continue;
            int available = storageSys->getResourceAmount(storageId, resourceType);
            int take = std::min(available, remaining);
            if (take > 0) {
                int32_t removed = storageSys->removeResourceFromStorage(storageId, settlerId, resourceType, take);
                if (removed > 0) {
                    std::cout << "CraftingSystem: Consumed " << removed << " " << resourceTypeStr
                              << " from storage '" << storageId << "'" << std::endl;
                    remaining -= removed;
                }
            }
            if (remaining <= 0) break;
        }

        if (remaining > 0) {
            std::cout << "CraftingSystem: Failed to consume " << remaining << " " << resourceTypeStr << std::endl;
            return false;
        }
    }
    std::cout << "CraftingSystem: All ingredients consumed." << std::endl;
    return true;
}

std::unique_ptr<Item> CraftingSystem::createItemFromRecipe(const CraftingRecipe& recipe) {
    std::cout << "CraftingSystem: Creating item from recipe '" << recipe.id << "'" << std::endl;
    
    // Create equipment/tool based on resultType
    if (recipe.resultType == ItemType::TOOL || recipe.resultType == ItemType::EQUIPMENT) {
        auto item = std::make_unique<EquipmentItem>(
            recipe.resultItemId,                     // name
            EquipmentItem::EquipmentSlot::MAIN_HAND, // slot
            recipe.description                       // description  
        );
        std::cout << "CraftingSystem: Created equipment item: " << recipe.resultItemId << std::endl;
        return item;
    }
    // Add other types as needed
    else {
        std::cout << "CraftingSystem: Unknown result type for recipe " << recipe.id << std::endl;
        return nullptr;
    }
}

// Pending craft management
void CraftingSystem::addPendingCraft(const std::string& settlerId, const std::string& recipeId, const std::vector<std::pair<std::string, int>>& missing) {
    std::string key = settlerId + "|" + recipeId;
    PendingCraft pending;
    pending.settlerId = settlerId;
    pending.recipeId = recipeId;
    pending.missingResources = missing;
    pending.gatherTaskIssued = false;
    m_pendingCrafts[key] = pending;
    std::cout << "[CRAFT DEBUG] 1/5 Craft requested → pending craft added | settler: '" << settlerId
              << "' | recipe: '" << recipeId << "' | missing: ";
    for (const auto& m : missing) std::cout << m.first << ":" << m.second << " ";
    std::cout << std::endl;
    if (!missing.empty()) {
        std::cout << "[CRAFT DEBUG] 2/5 Gather task issued | settler: '" << settlerId
                  << "' | required: ";
        for (const auto& m : missing) std::cout << m.first << ":" << m.second << " ";
        std::cout << " | for recipe: '" << recipeId << "'" << std::endl;
        pending.gatherTaskIssued = true;
    }
}

void CraftingSystem::removePendingCraft(const std::string& settlerId, const std::string& recipeId) {
    std::string key = settlerId + "|" + recipeId;
    auto it = m_pendingCrafts.find(key);
    if (it != m_pendingCrafts.end()) {
        m_pendingCrafts.erase(it);
        std::cout << "CraftingSystem: Removed pending craft for settler '" << settlerId << "' recipe '" << recipeId << "'" << std::endl;
    }
}

PendingCraft* CraftingSystem::getPendingCraft(const std::string& settlerId, const std::string& recipeId) {
    std::string key = settlerId + "|" + recipeId;
    auto it = m_pendingCrafts.find(key);
    if (it != m_pendingCrafts.end()) {
        return &it->second;
    }
    return nullptr;
}

void CraftingSystem::onInventoryChanged(const InventoryChangedEvent& event) {
    // Check if this settler has any pending crafts
    std::string ownerId = event.owner ? event.owner->getId() : "";
    if (ownerId.empty()) return;
    for (auto it = m_pendingCrafts.begin(); it != m_pendingCrafts.end(); ) {
        if (it->second.settlerId == ownerId) {
            // Check if we can now craft
            if (canCraft(it->second.recipeId, ownerId, true)) {
                // Resume craft
                std::cout << "CraftingSystem: Resuming craft for settler '" << ownerId << "' recipe '" << it->second.recipeId << "'" << std::endl;
                // Queue the task again or trigger crafting directly
                queueTask(it->second.recipeId);
                it = m_pendingCrafts.erase(it); // Remove from pending
            } else {
                ++it;
            }
        } else {
            ++it;
        }
    }
}

void CraftingSystem::checkAndResumeCraft(const std::string& settlerId) {
    // Similar to onInventoryChanged but for manual check
    for (auto it = m_pendingCrafts.begin(); it != m_pendingCrafts.end(); ) {
        if (it->second.settlerId == settlerId) {
            if (canCraft(it->second.recipeId, settlerId, true)) {
                std::cout << "CraftingSystem: Resuming craft for settler '" << settlerId << "' recipe '" << it->second.recipeId << "'" << std::endl;
                queueTask(it->second.recipeId);
                it = m_pendingCrafts.erase(it);
            } else {
                ++it;
            }
        } else {
            ++it;
        }
    }
}
