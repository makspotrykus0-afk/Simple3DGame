#ifndef GATHERING_TASK_H
#define GATHERING_TASK_H

#include <cstdint>
#include <raylib.h>
#include "../core/GameEntity.h"

class GatheringTask {
public:
    enum class GatheringState {
        IDLE,
        MOVING_TO_TARGET,
        GATHERING,
        COMPLETED,
        FAILED
    };

    GatheringTask(GameEntity* target) 
        : m_target(target), m_position({0,0,0}), m_amountGathered(0), m_state(GatheringState::IDLE) {
        if (m_target) {
            m_position = m_target->getPosition();
        }
    }
        
    void update(float deltaTime) {} // Logic handled in Settler for now

    GatheringState getState() const { return m_state; }
    void setState(GatheringState s) { m_state = s; }

    GameEntity* getTarget() const { return m_target; }
    
    bool isComplete() const { return m_state == GatheringState::COMPLETED; }
    void setComplete() { m_state = GatheringState::COMPLETED; }
    
    void removeWorker() {
        // If we implement worker tracking on task level
    }
    
    Vector3 getPosition() const { return m_position; }

private:
    GameEntity* m_target;
    Vector3 m_position;
    int32_t m_amountGathered;
    GatheringState m_state;
};

#endif // GATHERING_TASK_H