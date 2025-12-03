#pragma once

#include "../core/Entity.h"
#include "InteractableObject.h"
#include "../components/StatsComponent.h"
#include "../components/PositionComponent.h"
#include <memory>
#include <string>

enum class AnimalType {
    RABBIT,
    DEER
};

// Changed inheritance to include BaseInteractableObject
class Animal : public Entity, public BaseInteractableObject {
public:
    Animal(AnimalType type, Vector3 position);
    virtual ~Animal() = default;

    void update(float deltaTime);
    void render();

    // InteractableObject Interface Overrides (from BaseInteractableObject)
    // We can override these if needed or use BaseInteractableObject defaults
    InteractionResult interact(GameEntity* interactor) override;
    InteractionInfo getDisplayInfo() const override;
    
    // Helper to access position from component or base
    Vector3 getPosition() const override;
    bool isActive() const override;

    // Game Logic
    void takeDamage(float amount);
    bool isDead() const;
    AnimalType getType() const { return m_type; }

private:
    AnimalType m_type;
    std::unique_ptr<StatsComponent> m_stats;
    
    // AI State
    Vector3 m_targetPosition;
    float m_moveTimer;
    float m_idleTimer;
    bool m_isMoving;
    float m_moveSpeed;

    void updateAI(float deltaTime);
    void die();
};