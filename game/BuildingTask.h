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
  void setPosition(Vector3 pos) { m_position = pos; } // Added for Editor
  float getRotation() const { return m_rotation; }
  void setRotation(float rot) { m_rotation = rot; } // Added for Editor
  GameEntity *getBuilder() const { return m_builder; }
  float getProgress() const { return m_progress; }
  float
  getMaterialProgress01() const; // Added for delivery progress visualization
  const std::unordered_map<std::string, int> &getCollectedResources() const {
    return m_collectedResources;
  }
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
  
  // Task commitment helpers
  bool hasWorker(Settler *settler) const;
  bool isFullyStaffed() const { return getWorkerCount() >= m_maxWorkers; }
  int getMaxWorkers() const { return m_maxWorkers; }
  void setMaxWorkers(int max) { m_maxWorkers = max; }

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
  int m_maxWorkers = 3; // Maximum concurrent workers per task
  float m_lastLogProgress = 0.0f;
};