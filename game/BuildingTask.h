#pragma once

#include "../game/BuildingBlueprint.h"
#include "raylib.h"
#include <string>
#include <unordered_map>
#include <vector>

// Forward declarations
class GameEntity;
class Settler;

enum class BuildState { PLANNING, CONSTRUCTION, COMPLETED, CANCELLED };

/**
 * @brief Task representing a construction job
 */
class BuildTask {
public:
  BuildTask(BuildingBlueprint *blueprint, Vector3 position, GameEntity *builder,
            float rotation);

  // Methods
  void update(float deltaTime);
  void cancel();
  void addResource(const std::string &resource, int amount);
  bool hasAllResources() const;
  void advanceConstruction(float amount);

  // Getters
  BuildingBlueprint *getBlueprint() const { return m_blueprint; }
  Vector3 getPosition() const { return m_position; }
  float getRotation() const { return m_rotation; }
  GameEntity *getBuilder() const { return m_builder; }
  float getProgress() const { return m_progress; }
  float
  getMaterialProgress01() const; // Added for delivery progress visualization
  BuildState getState() const { return m_state; }
  std::vector<ResourceRequirement> getMissingResources() const;

  // State checks
  bool isCompleted() const { return m_state == BuildState::COMPLETED; }
  bool isCancelled() const { return m_state == BuildState::CANCELLED; }
  bool isActive() const {
    return m_state != BuildState::COMPLETED && m_state != BuildState::CANCELLED;
  }

  // Collision
  BoundingBox getBoundingBox() const;

  // Worker management
  void addWorker(Settler *worker);
  void removeWorker(Settler *worker);
  int getWorkerCount() const { return static_cast<int>(m_workers.size()); }
  const std::vector<Settler *> &getWorkers() const { return m_workers; }

private:
  BuildingBlueprint *m_blueprint;
  Vector3 m_position;
  GameEntity *m_builder;
  float m_rotation;
  float m_progress;
  BuildState m_state;
  float m_startTime;

  std::unordered_map<std::string, int> m_collectedResources;
  std::vector<Settler *> m_workers;
};