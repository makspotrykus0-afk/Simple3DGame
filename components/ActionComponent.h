#ifndef ACTION_COMPONENT_H
#define ACTION_COMPONENT_H

#include "../game/SettlerTypes.h"
#include "IComponent.h"
#include <deque>
#include <memory>
#include <typeindex>
#include <vector>


class Settler;
class Tree;
class Bush;
class WorldItem;
class BuildingInstance;
class Animal;
class ResourceNode;

class ActionComponent : public IComponent {
public:
  ActionComponent(Settler *owner);

  std::type_index getComponentType() const override;

  void update(float deltaTime) override;
  void update(float deltaTime, float currentTime,
              const std::vector<std::unique_ptr<Tree>> &trees,
              std::vector<WorldItem> &worldItems,
              const std::vector<Bush *> &bushes,
              const std::vector<BuildingInstance *> &buildings,
              const std::vector<std::unique_ptr<Animal>> &animals,
              const std::vector<std::unique_ptr<ResourceNode>> &resourceNodes);

  void initialize() override {}
  void shutdown() override {}
  void render() override {}

  // Utility AI Scoring
  struct ScoredAction {
    SettlerState actionState;
    float score;
    std::string debugReason;

    bool operator<(const ScoredAction &other) const {
      return score < other.score;
    }
  };

  // FSM State management
  void setState(SettlerState newState);
  SettlerState getState() const { return m_currentState; }
  void interrupt();

  // Core Utility AI Evaluator
  void EvaluateAndChooseAction(
      float deltaTime, const std::vector<std::unique_ptr<Tree>> &trees,
      std::vector<WorldItem> &worldItems, const std::vector<Bush *> &bushes,
      const std::vector<BuildingInstance *> &buildings,
      const std::vector<std::unique_ptr<Animal>> &animals,
      const std::vector<std::unique_ptr<ResourceNode>> &resourceNodes);

  // Action Queue management
  void pushAction(Action action);
  void clearQueue();
  bool hasActions() const { return !m_actionQueue.empty(); }
  const std::deque<Action> &getQueue() const { return m_actionQueue; }

private:
  void
  updateFSM(float deltaTime, float currentTime,
            const std::vector<std::unique_ptr<Tree>> &trees,
            std::vector<WorldItem> &worldItems,
            const std::vector<Bush *> &bushes,
            const std::vector<BuildingInstance *> &buildings,
            const std::vector<std::unique_ptr<Animal>> &animals,
            const std::vector<std::unique_ptr<ResourceNode>> &resourceNodes);

  void UpdateIdleDecision(
      float deltaTime, const std::vector<std::unique_ptr<Tree>> &trees,
      std::vector<WorldItem> &worldItems, const std::vector<Bush *> &bushes,
      const std::vector<BuildingInstance *> &buildings,
      const std::vector<std::unique_ptr<Animal>> &animals,
      const std::vector<std::unique_ptr<ResourceNode>> &resourceNodes);

  void executeNextAction();

  // State Handlers (migrated from Settler)
  void UpdateMovement(
      float deltaTime, const std::vector<std::unique_ptr<Tree>> &trees,
      const std::vector<BuildingInstance *> &buildings,
      const std::vector<std::unique_ptr<ResourceNode>> &resourceNodes);
  void UpdateGathering(float deltaTime, std::vector<WorldItem> &worldItems,
                       const std::vector<BuildingInstance *> &buildings);
  void UpdateBuilding(float deltaTime);
  void UpdateSearchingForFood(float deltaTime,
                              const std::vector<Bush *> &bushes);
  void UpdateMovingToFood(float deltaTime);
  void UpdateEating(float deltaTime);
  void UpdateMovingToBed(float deltaTime);
  void UpdateSleeping(float deltaTime);
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
  void UpdatePickingUp(float deltaTime, std::vector<WorldItem> &worldItems,
                       const std::vector<BuildingInstance *> &buildings);
  void UpdateCrafting(float deltaTime);
  void UpdateChopping(float deltaTime);
  void UpdateMining(float deltaTime);
  void UpdateFetchingResource(float deltaTime,
                              const std::vector<BuildingInstance *> &buildings);
  void UpdateDepositing(float deltaTime,
                        const std::vector<BuildingInstance *> &buildings);
  void UpdateMovingToStorage(
      float deltaTime, const std::vector<BuildingInstance *> &buildings,
      const std::vector<std::unique_ptr<ResourceNode>> &resourceNodes);

  Settler *m_owner;
  SettlerState m_currentState;
  std::deque<Action> m_actionQueue;
};

#endif
