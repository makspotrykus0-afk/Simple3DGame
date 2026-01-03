#ifndef SIMPLE3DGAME_GAME_SETTLER_H

#define SIMPLE3DGAME_GAME_SETTLER_H
#include <string>

#include <vector>

#include <deque>

#include <memory>

#include <algorithm>

#include "raylib.h"

#include "raymath.h"

#include "../core/GameEntity.h"

#include "InteractableObject.h"

#include "../components/InventoryComponent.h"

#include "../components/StatsComponent.h"

#include "../components/SkillsComponent.h"

// Forward declarations

class Tree;
class BuildingInstance;
#include "WorldItem.h"
class Bush;
class Animal;
class Item;
class ResourceNode;
class BuildTask;
class GatheringTask;
class Projectile;
struct CraftingTask; // Forward decl
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

  CHOPPING,

  MINING,

  BUILDING,

  SLEEPING,

  HUNTING,

  HAULING,

  MOVING_TO_STORAGE,

  DEPOSITING,

  SEARCHING_FOR_FOOD,

  MOVING_TO_FOOD,

  EATING,

  MOVING_TO_BED,

  PICKING_UP,

  WANDER,

  WAITING,
  CRAFTING,
  SKINNING,
  MOVING_TO_SKIN,
  FETCHING_RESOURCE,
  SOCIAL,
  MOVING_TO_SOCIAL,
  MORNING_STRETCH,
  SOCIAL_LOOKOUT
};
enum class TaskType {
  MOVE,

  GATHER,

  CHOP_TREE,

  MINE_ROCK,

  BUILD,

  ATTACK,

  WAIT,

  DEPOSIT,

  PICKUP

};
struct Action {
  TaskType type;
  GameEntity *targetEntity = nullptr;
  Bush *targetBush = nullptr;
  Animal *targetAnimal = nullptr;
  Tree *targetTree = nullptr;
  BuildingInstance *targetBuilding = nullptr;
  WorldItem *targetWorldItem = nullptr;
  ResourceNode *targetResourceNode = nullptr;
  Vector3 targetPosition = {0, 0, 0};
  float duration = 0.0f;
  bool completed = false;
  static Action Move(Vector3 pos) {
    Action a;
    a.type = TaskType::MOVE;
    a.targetPosition = pos;
    return a;
  }
  static Action Pickup(Vector3 pos) {
    Action a;
    a.type = TaskType::PICKUP;
    a.targetPosition = pos;
    return a;
  }
};
struct SavedTask {
  TaskType type;
  Vector3 targetPosition;
  int targetEntityId;
  float duration;
  bool hasTarget;
};
class Settler : public GameEntity, public InteractableObject {
public:
  Settler(const std::string &name, const Vector3 &pos,
          SettlerProfession profession);
  virtual ~Settler();
  void Update(float deltaTime, float currentTime, const std::vector<std::unique_ptr<Tree>> &trees,
              std::vector<WorldItem> &worldItems,
              const std::vector<Bush *> &bushes,
              const std::vector<BuildingInstance *> &buildings,
              const std::vector<std::unique_ptr<Animal>> &animals,
              const std::vector<std::unique_ptr<ResourceNode>> &resourceNodes);
  void render() override;
  void render(bool isFps); // Overload for FPS mode
  // renderFPS removed
  Vector3 getPosition() const override { return position; }
  void setPosition(const Vector3 &pos) override { position = pos; }
  InteractionResult interact(GameEntity * /*player*/) override;
  InteractionInfo getDisplayInfo() const override;
  std::string getName() const override { return m_name; }
  InteractionType getInteractionType() const override {
    return InteractionType::INSPECTION;
  }
  float getInteractionRange() const override { return 2.0f; }
  bool canInteract(GameEntity * /*player*/) const override { return true; }
  SettlerState getState() const { return m_state; }
  void setState(SettlerState newState) { m_state = newState; }
  std::string GetStateString() const;
  SettlerProfession getProfession() const { return m_profession; }
  void setProfession(SettlerProfession newProfession) {
    m_profession = newProfession;
  }
  std::string GetProfessionString() const;
  void assignTask(TaskType type, GameEntity *target, Vector3 pos);
  void clearTasks();
  bool hasTasks() const { return !m_actionQueue.empty(); }
  const std::deque<Action> &getTaskQueue() const { return m_actionQueue; }
  void MoveTo(Vector3 destination);
  void setMoveSpeed(float speed) { m_moveSpeed = speed; }
  float getMoveSpeed() const { return m_moveSpeed; }
  bool isMoving() const { return m_state == SettlerState::MOVING; }
  Vector3 getTargetPosition() const { return m_targetPosition; }
  void Stop();
  InventoryComponent &getInventory() { return m_inventory; }
  const InventoryComponent &getInventory() const { return m_inventory; }
  StatsComponent &getStats() { return m_stats; }
  const StatsComponent &getStats() const { return m_stats; }
  SkillsComponent &getSkills() { return m_skills; }
  const SkillsComponent &getSkills() const { return m_skills; }
  void setName(const std::string &newName) { m_name = newName; }
  bool isSelected() const { return m_isSelected; }
  void setSelected(bool sel) { m_isSelected = sel; }
  // Dummy Setters/Getters for interface compatibility if needed
  void setTargetTree(Tree * /*tree*/) {}
  void setTargetBuilding(BuildingInstance * /*building*/) {}
  void setTargetBush(Bush * /*bush*/) {}
  void setTargetAnimal(Animal * /*animal*/) {}
  void setTargetWorldItem(WorldItem * /*item*/) {}
  void setTargetResourceNode(ResourceNode * /*node*/) {}
  Tree *getTargetTree() const { return nullptr; }
  BuildingInstance *getTargetBuilding() const { return nullptr; }
  Bush *getTargetBush() const { return nullptr; }
  Animal *getTargetAnimal() const { return nullptr; }
  WorldItem *getTargetWorldItem() const { return nullptr; }
  ResourceNode *getTargetResourceNode() const { return nullptr; }
  float getWorkProgress() const { return 0.0f; }
  void setWorkProgress(float /*progress*/) {}
  void setPath(const std::vector<Vector3> &newPath);
  bool hasPath() const { return !m_currentPath.empty(); }
  void clearPath() {
    m_currentPath.clear();
    m_currentPathIndex = 0;
  }
  SavedTask serializeCurrentTask() const;
  void deserializeTask(const SavedTask &savedTask);
  float getDistanceTo(Vector3 point) const;
  bool isAtPosition(Vector3 pos, float tolerance = 0.5f) const;
  void updateNeeds(float deltaTime);
  bool needsFood() const;
  bool needsSleep() const;
  void takeDamage(float damage);
  bool isDead() const { return m_stats.getCurrentHealth() <= 0; }
  void assignBed(BuildingInstance *bed);
  void assignBuildTask(BuildTask *task);
  void clearBuildTask();
  void assignToChop(GameEntity *tree);
  void assignToMine(GameEntity *rock);
  void forceGatherTarget(GameEntity *target);
  void ExecuteNextAction();
  void UpdateMovement(
      float deltaTime, const std::vector<std::unique_ptr<Tree>> &trees,
      const std::vector<BuildingInstance *> &buildings,
      const std::vector<std::unique_ptr<ResourceNode>> &resourceNodes = {});

  void UpdateGathering(float deltaTime, std::vector<WorldItem> &worldItems,
                       const std::vector<BuildingInstance *> &buildings);
  void UpdateBuilding(float deltaTime);
  void UpdateSleeping(float deltaTime);
  void UpdateMovingToStorage(
      float deltaTime, const std::vector<BuildingInstance *> &buildings,
      const std::vector<std::unique_ptr<ResourceNode>> &resourceNodes);
  void UpdateDepositing(float deltaTime,
                        const std::vector<BuildingInstance *> &buildings);
  void UpdatePickingUp(float deltaTime, std::vector<WorldItem> &worldItems,
                       const std::vector<BuildingInstance *> &buildings);
  void UpdateSearchingForFood(float deltaTime,
                              const std::vector<Bush *> &bushes);
  void UpdateMovingToFood(float deltaTime);
  void UpdateEating(float deltaTime);
  void UpdateMovingToBed(float deltaTime);
  void UpdateChopping(float deltaTime);
  void UpdateMining(float deltaTime);
  void UpdateCrafting(float deltaTime);
  float GetCraftingProgress01() const;
  void UpdateHauling(float deltaTime,
                     const std::vector<BuildingInstance *> &buildings,
                     std::vector<WorldItem> &worldItems);
  void UpdateHunting(
      float deltaTime, const std::vector<std::unique_ptr<Animal>> &animals,
      const std::vector<BuildingInstance *> &buildings,
      const std::vector<std::unique_ptr<ResourceNode>> &resourceNodes);
  void UpdateSkinning(float deltaTime);
  void UpdateMovingToSkin(
      float deltaTime, const std::vector<BuildingInstance *> &buildings,
      const std::vector<std::unique_ptr<ResourceNode>> &resourceNodes);
  void UpdateFetchingResource(float deltaTime, const std::vector<BuildingInstance *> &buildings);
  void CraftTool(const std::string &toolName);
  bool PickupItem(Item *item);
  void DropItem(int slotIndex);
  BuildingInstance *
  FindNearestStorage(const std::vector<BuildingInstance *> &buildings);
  BuildingInstance *
  FindNearestStorageWithResource(const std::vector<BuildingInstance *> &buildings, const std::string& resourceType);
  BuildingInstance *
  FindNearestWorkshop(const std::vector<BuildingInstance *> &buildings);
  void ignoreStorage(const std::string &storageId);
  bool isStorageIgnored(const std::string &storageId) const;
  void ClearIgnoredStorages();
  Vector3 myHousePos() const;
  void SetSelected(bool sel) { setSelected(sel); }
  bool IsSelected() const { return isSelected(); }
  SettlerState GetState() const { return getState(); }

  // Independent builder logic
  void setIndependent(bool independent) {
    m_isIndependentBuilder = independent;
  }
  bool isIndependent() const { return m_isIndependentBuilder; }
  BuildTask *getPrivateBuildTask() const { return m_myPrivateBuildTask; }
  void setPrivateBuildTask(BuildTask *task) { m_myPrivateBuildTask = task; }
  BuildTask *getCurrentBuildTask() const { return m_currentBuildTask; }
  
  // Task commitment - prevents premature task switching
  bool isCommittedToTask() const;

  void ForceGatherTarget(GameEntity *target) { forceGatherTarget(target); }
  void AssignBuildTask(BuildTask *task) { assignBuildTask(task); }
  void ClearBuildTask() { clearBuildTask(); }
  void OnJobConfigurationChanged();
  // Held item management
  bool isHandFree() const { return m_heldItem == nullptr; }
  void setHeldItem(std::unique_ptr<Item> item) { m_heldItem = std::move(item); }
  const Item *getHeldItem() const { return m_heldItem.get(); }
  // Player control methods
  void setPlayerControlled(bool controlled);
  bool isPlayerControlled() const { return m_isPlayerControlled; }
  void setRotationFromMouse(float yaw);
  void setRotation(float rotation) { m_rotation = rotation; }

  /**
   * @brief Używa trzymanego przedmiotu (strzał, cios, rąbanie)
   * @param targetPos Pozycja celu (w świecie)
   */
  void useHeldItem(Vector3 targetPos);

  float getRotation() const { return m_rotation; }
  Vector3 getForwardVector() const;
  void setScoping(bool scoping) { m_isScoping = scoping; }
  bool isScoping() const { return m_isScoping; }

  // Combat
  void shoot(Vector3 targetPos);
  Vector3 getMuzzlePosition() const;

  // Building Logic
  void ProcessActiveBuildTask(float deltaTime,
                              const std::vector<BuildingInstance *> &buildings, std::vector<WorldItem> &worldItems);

private:
  float m_shootCooldownTimer = 0.0f;
  float m_weaponSpread = 0.005f; // Accuracy spread (reduced from 0.05f to
                                 // 0.005f for better accuracy)

  float m_weaponDamage = 35.0f;
  float m_weaponSpeed =
      120.0f; // Projectile speed (increased for better ballistics)

public:
  // Preferences
  int preferredHouseSize = 4;
  std::string actionState;
  bool hasHouse = false;
  // Job Flags
  bool gatherWood = false;
  bool gatherStone = false;
  bool gatherFood = false;
  bool performBuilding = false;
  bool huntAnimals = false;
  bool craftItems = false;
  bool haulToStorage = false;
  bool tendCrops = false;
  Item *pendingDropItem = nullptr;

private:
  std::string m_name;
  SettlerState m_state;
  SettlerProfession m_profession;
  bool m_isSelected;
  Vector3 position;
  InventoryComponent m_inventory;
  StatsComponent m_stats;
  SkillsComponent m_skills;
  float m_moveSpeed;
  Vector3 m_targetPosition;
  float m_rotation;
  bool m_isPlayerControlled = false;
  bool m_isScoping = false;
  float m_adsLerp = 0.0f; // 0.0 = Hip, 1.0 = ADS
  std::vector<Vector3> m_currentPath;
  int m_currentPathIndex;
  std::deque<Action> m_actionQueue;
  BuildTask *m_currentBuildTask;
  GatheringTask *m_currentGatherTask;
  BuildingInstance *m_targetStorage;
  BuildingInstance *m_targetWorkshop; // Warsztat do craftingu

  // Independent builder state
  bool m_isIndependentBuilder = false;
  BuildTask *m_myPrivateBuildTask = nullptr;

  Bush *m_currentGatherBush;
  Tree *m_currentTree = nullptr;
  ResourceNode *m_currentResourceNode = nullptr;
  BuildingInstance *m_assignedBed;
  Bush *m_targetFoodBush;
  float m_gatherInterval;
  std::string m_currentCraftTarget;
  int m_currentCraftTaskId = -1; // ID aktualnego zadania craftingu
  
  // Resource Fetching
  std::string m_resourceToFetch = "";
  int m_resourceFetchAmount = 0;

public:
  // FPS Editor Variables (Static) - Made Public for EditorSystem
  static float s_fpsUserFwd;   // X
  static float s_fpsUserRight; // Z
  static float s_fpsUserUp;    // Y
  static float s_fpsYaw;
  static float s_fpsPitch;

  int getCraftingTaskId() const { return m_currentCraftTaskId; }
  std::vector<std::string>
      m_ignoredStorages; // Job State History (to detect toggle)
  bool m_prevGatherWood = false;
  bool m_prevGatherStone = false;
  bool m_prevGatherFood = false;
  bool m_prevPerformBuilding = false;
  bool m_prevHuntAnimals = false;
  bool m_prevCraftItems = false;
  bool m_prevHaulToStorage = false;
  bool m_prevTendCrops = false;
  bool m_isMovingToCriticalTarget = false;
  bool m_pendingReevaluation = false;
  Vector3 m_lastPathTarget = {0, 0, 0};
  bool m_lastPathValid = false;
  // Wykrywanie utknięcia
  float m_stuckTimer = 0.0f; // czas, przez który osadnik jest "utknięty"
  Vector3 m_lastPosition = {0, 0,
                            0}; // ostatnia pozycja do wykrywania braku ruchu
  float m_sleepCooldownTimer =
      0.0f; // Blokada ponownego snu przez X sekund po obudzeniu
  float m_eatingCooldownTimer = 0.0f;
  float m_sleepEnterThreshold = 30.0f;
  float m_sleepExitThreshold = 80.0f;
  float m_hungerEnterThreshold = 40.0f;
  float m_hungerExitThreshold = 80.0f;
  // Timery dla akcji
  float m_gatherTimer = 0.0f;
  float m_eatingTimer = 0.0f;
  float m_craftingTimer = 0.0f;
  // Hunting
  Animal *m_currentTargetAnimal = nullptr;
  float m_huntingTimer = 0.0f;
  int m_attackCount = 0;
  float m_attackAnimTimer = 0.0f; // Timer dla animacji uderzenia (0.0-1.0)
  bool m_waitingToDealDamage =
      false; // Flaga opóźnienia obrażeń (czekamy na 'hit frame')
  float m_skinningTimer = 0.0f;
  float m_aiSearchTimer = 0.0f; // AI throttling timer
  // Przedmiot trzymany w ręce (nóż)
  std::unique_ptr<Item> m_heldItem;
  bool IsStateInterruptible() const;
  bool CheckForJobFlagActivation();
  void InterruptCurrentAction();
  Bush *FindNearestFood(const std::vector<Bush *> &bushes);

private:
    // Circadian Rhythm
    void UpdateCircadianRhythm(float deltaTime, float currentTime, const std::vector<BuildingInstance *> &buildings);
    BuildingInstance* FindNearestBuildingByBlueprint(const std::string& blueprintId, const std::vector<BuildingInstance *> &buildings);

    // Time Constants
    const float TIME_WAKE_UP = 6.0f;
    const float TIME_WORK_START = 8.0f;
    const float TIME_WORK_END = 18.0f;
    const float TIME_SLEEP = 22.0f;
    
    // Internal state for social/waiting
    float m_socialTimer = 0.0f;
    float m_stretchTimer = 0.0f;  // [NEW] Timer dla Morning Stretch
    bool m_hasGreetedMorning = false;
};
#endif // SIMPLE3DGAME_GAME_SETTLER_H
