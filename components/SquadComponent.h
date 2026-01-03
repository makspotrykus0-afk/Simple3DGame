#ifndef SIMPLE3DGAME_COMPONENTS_SQUADCOMPONENT_H
#define SIMPLE3DGAME_COMPONENTS_SQUADCOMPONENT_H

#include "../core/Component.h"
#include "raylib.h"
#include <vector>

enum class SquadOrder {
  FOLLOW,  // Follow leader closely
  HOLD,    // Stay at current position
  MOVE_TO, // Go to specific position
  ATTACK   // Attack specific target (future)
};

class SquadComponent : public Component {
public:
  SquadComponent()
      : m_leaderID(-1), m_currentOrder(SquadOrder::FOLLOW),
        m_targetPosition({0, 0, 0}) {}

  // Leader ID (Entity ID of the player or commander)
  // -1 means no leader (free agent)
  int m_leaderID;

  // Current active order
  SquadOrder m_currentOrder;

  // Target position for MOVE_TO order
  Vector3 m_targetPosition;

  // Formation offset (where to stand relative to leader/target)
  Vector3 m_formationOffset = {0, 0, 0};

  bool hasLeader() const { return m_leaderID != -1; }

  void setLeader(int id) { m_leaderID = id; }
  void clearLeader() { m_leaderID = -1; }

  void setOrder(SquadOrder order, Vector3 target = {0, 0, 0}) {
    m_currentOrder = order;
    m_targetPosition = target;
  }
};

#endif // SIMPLE3DGAME_COMPONENTS_SQUADCOMPONENT_H
