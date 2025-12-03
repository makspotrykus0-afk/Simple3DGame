#ifndef SETTLER_H
#define SETTLER_H

#include "../core/GameEntity.h"
#include "../components/InventoryComponent.h"
#include "../components/StatsComponent.h"
#include "../systems/SkillsComponent.h"
#include "../game/BuildingTask.h"
#include "../game/GatheringTask.h"
#include "../game/InteractableObject.h"
#include <vector>
#include <string>
#include <memory>
#include "raylib.h"

enum class SettlerProfession {
    NONE,
    BUILDER,
    GATHERER,
    HUNTER,
    CRAFTER
};

enum class SettlerState {
    IDLE,
    MOVING,
    GATHERING,
    BUILDING,
    SLEEPING,
    HUNTING,
    HAULING,
    MOVING_TO_STORAGE,
    DEPOSITING
};

enum class TaskType {
    MOVE,
    GATHER,
    BUILD,
    ATTACK
};

// Forward declarations
class Tree;
class BuildingInstance;
class WorldItem;
class Bush;
class Animal;
class Item;

class Settler : public GameEntity, public InteractableObject {
public:
    Settler(const std::string& name, const Vector3& position, SettlerProfession profession = SettlerProfession::NONE);
    virtual ~Settler();

    void Update(float deltaTime, const std::vector<std::unique_ptr<Tree>>& trees, std::vector<WorldItem>& worldItems, const std::vector<Bush*>& bushes, const std::vector<BuildingInstance*>& buildings, const std::vector<std::unique_ptr<Animal>>& animals);
    void render() override;

    // InteractableObject Implementation
    InteractionResult interact(GameEntity* player) override;
    InteractionInfo getDisplayInfo() const override;
    InteractionType getInteractionType() const override { return InteractionType::CONVERSATION; }
    float getInteractionRange() const override { return 3.0f; }
    bool canInteract(GameEntity* player) const override { return true; }
    std::string getName() const override { return m_name; }
    Vector3 getPosition() const override { return position; } // Override from GameEntity/InteractableObject

    // Movement & Tasks
    void MoveTo(Vector3 destination);
    void Stop();
    void assignTask(TaskType type, GameEntity* target = nullptr, Vector3 pos = {0,0,0});
    
    // State accessors
    SettlerState getState() const { return m_state; }
    std::string GetStateString() const;
    
    // Components
    InventoryComponent& getInventory() { return m_inventory; }
    StatsComponent& getStats() { return m_stats; }
    SkillsComponent& getSkills() { return m_skills; }
    const SkillsComponent& getSkills() const { return m_skills; } // Const overload for UI
    
    // UI info
    void setSelected(bool selected) { m_isSelected = selected; }
    bool isSelected() const { return m_isSelected; }
    
    // Housing
    void assignBed(BuildingInstance* bed);
    Vector3 myHousePos() const;
    
    // Inventory helper
    bool PickupItem(Item* item);
    void DropItem(int slotIndex);

    // Task assignment (made public for InteractionSystem)
    void AssignBuildTask(BuildTask* task);
    void AssignToChop(GameEntity* tree);
    void ForceGatherTarget(GameEntity* target);

    std::string actionState; // Public for UI System

    // Public position for direct access and compatibility
    Vector3 position;

    // Compatibility fields for existing codebase
    float preferredHouseSize = 10.0f;
    bool hasHouse = false;
    Item* pendingDropItem = nullptr; 

    // Compatibility methods
    void SetSelected(bool s) { setSelected(s); }
    bool IsSelected() const { return isSelected(); }
    SettlerState GetState() const { return getState(); }

    // Override GameEntity position setter to sync with local field
    void setPosition(const Vector3& pos) override { position = pos; }

private:
    void UpdateMovement(float deltaTime, const std::vector<std::unique_ptr<Tree>>& trees, const std::vector<BuildingInstance*>& buildings);
    void UpdateGathering(float deltaTime, std::vector<WorldItem>& worldItems, const std::vector<BuildingInstance*>& buildings);
    void UpdateBuilding(float deltaTime);
    void UpdateHunting(float deltaTime, const std::vector<std::unique_ptr<Animal>>& animals, const std::vector<BuildingInstance*>& buildings);
    void UpdateHauling(float deltaTime, const std::vector<BuildingInstance*>& buildings, std::vector<WorldItem>& worldItems);
    void UpdateSleeping(float deltaTime);
    void UpdateMovingToStorage(float deltaTime, const std::vector<BuildingInstance*>& buildings);
    void UpdateDepositing(float deltaTime);
    
    // Collision handling
    bool CheckCollision(Vector3 targetPos, const std::vector<BuildingInstance*>& buildings);
    
    // Finding targets
    BuildingInstance* FindNearestStorage(const std::vector<BuildingInstance*>& buildings);

    std::string m_name;
    SettlerProfession m_profession;
    SettlerState m_state;
    
    InventoryComponent m_inventory;
    StatsComponent m_stats;
    SkillsComponent m_skills;
    
    // Navigation
    Vector3 m_targetPosition;
    std::vector<Vector3> m_currentPath;
    size_t m_currentPathIndex = 0;
    float m_moveSpeed;
    float m_rotation;
    
    // Tasks
    BuildTask* m_currentBuildTask;
    GatheringTask* m_currentGatherTask; // Using ptr for now, should be managed by system
    BuildingInstance* m_targetStorage = nullptr;
    
    float m_gatherTimer;
    float m_gatherInterval;
    
    bool m_isSelected;
    
    BuildingInstance* m_assignedBed = nullptr;
};

#endif // SETTLER_H