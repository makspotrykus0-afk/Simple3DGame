#include "Ant.h"
#include "raymath.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <memory>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "../components/PositionComponent.h"

Ant::Ant(const std::string& id, Vector3 startPos) 
    : GameEntity(id)
    , m_position(startPos)
    , target(startPos)
    , speed(2.0f)
    , state(AntState::IDLE)
    , hasFood(false)
    , health(100.0f)
    , color(RED) 
{
    addComponent(std::make_shared<PositionComponent>(startPos));
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
}

Ant::Ant(const std::string& id, Vector3 startPos, Color c, float spd) 
    : GameEntity(id)
    , m_position(startPos)
    , target(startPos)
    , speed(spd)
    , state(AntState::IDLE)
    , hasFood(false)
    , health(100.0f)
    , color(c) 
{
    addComponent(std::make_shared<PositionComponent>(startPos));
}

void Ant::update(float deltaTime) {
    // Sync GameEntity position
    auto posComp = getComponent<PositionComponent>();
    if (posComp) posComp->setPosition(m_position);

    // Simple AI behavior based on state
    switch (state) {
        case AntState::IDLE:
            // Random movement
            if (GetRandomValue(0, 100) < 5) { // 5% chance per frame to start searching
                state = AntState::SEARCHING;
            }
            break;
            
        case AntState::SEARCHING:
            // Look for food
            if (GetRandomValue(0, 100) < 2) { // 2% chance to find food
                PickUpFood();
            } else {
                // Random wandering
                float angle = static_cast<float>(GetRandomValue(0, 360)) * M_PI / 180.0f;
                Vector3 randomMove = { cosf(angle) * 2.0f, 0.0f, sinf(angle) * 2.0f };
                m_position = Vector3Add(m_position, Vector3Scale(randomMove, deltaTime));
            }
            break;
            
        case AntState::CARRYING_FOOD:
            // Head back to colony (simplified)
            state = AntState::RETURNING_TO_COLONY;
            break;
            
        case AntState::RETURNING_TO_COLONY: {
            // Move towards origin (colony center)
            Vector3 direction = Vector3Subtract({0.0f, 0.0f, 0.0f}, m_position);
            float distance = Vector3Length(direction);
            if (distance > 0.1f) {
                Vector3 normalizedDir = Vector3Normalize(direction);
                m_position = Vector3Add(m_position, Vector3Scale(normalizedDir, speed * deltaTime));
            } else {
                DropFood();
            }
            break;
        }
            
        case AntState::FIGHTING:
            // Simple fighting behavior (can be expanded)
            break;
    }
    
    // Update m_position after changes
    if (posComp) posComp->setPosition(m_position);
}

void Ant::render() {
    Color antColor = color; // Default to instance color
    
    switch (state) {
        case AntState::IDLE:
            // Use default color
            break;
        case AntState::SEARCHING:
            antColor = Color{0, 100, 0, 255}; // Dark green
            break;
        case AntState::CARRYING_FOOD:
            antColor = Color{255, 0, 255, 255}; // Magenta
            break;
        case AntState::RETURNING_TO_COLONY:
            antColor = Color{0, 0, 255, 255}; // Blue
            break;
        case AntState::FIGHTING:
            antColor = Color{255, 0, 0, 255}; // Red
            break;
    }
    
    // Draw ant as a small sphere
    DrawSphere(m_position, 0.3f, antColor);
}

void Ant::SetTarget(Vector3 newTarget) {
    target = newTarget;
    state = AntState::SEARCHING;
}