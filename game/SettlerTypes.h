#ifndef SIMPLE3DGAME_GAME_SETTLER_TYPES_H
#define SIMPLE3DGAME_GAME_SETTLER_TYPES_H

#include "raylib.h"
#include <deque>
#include <string>
#include <vector>


class GameEntity;
class Tree;
class BuildingInstance;
class Bush;
class Animal;
class WorldItem;
class ResourceNode;

enum class SettlerProfession { NONE, BUILDER, GATHERER, HUNTER, CRAFTER };

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
  SOCIAL_LOOKOUT,
  // Squad States
  FOLLOWING,
  GUARDING,
  ATTACKING_TARGET
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

#endif // SIMPLE3DGAME_GAME_SETTLER_TYPES_H
