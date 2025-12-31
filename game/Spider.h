#pragma once

#include "../core/Entity.h"

#include "InteractableObject.h"

#include "../components/StatsComponent.h"

#include "../components/PositionComponent.h"

#include <memory>

#include <string>

enum class SpiderType {

COMMON_SPIDER

};

class Spider : public Entity, public BaseInteractableObject {

public:

Spider(SpiderType type, Vector3 position);

virtual ~Spider() = default;



text


void update(float deltaTime);
void render();

// InteractableObject Interface Overrides
InteractionResult interact(GameEntity* interactor) override;
InteractionInfo getDisplayInfo() const override;

// Helper to access position from component or base
Vector3 getPosition() const override;
bool isActive() const override;

// Game Logic
void takeDamage(float amount);
bool isDead() const;
SpiderType getType() const { return m_type; }


private:

SpiderType m_type;

std::unique_ptr<StatsComponent> m_stats;



text


// AI State
Vector3 m_targetPosition;
float m_moveTimer;
float m_idleTimer;
float m_attackCooldown;
bool m_isMoving;
bool m_isAttacking;

// Stats
float m_moveSpeed;
float m_attackRange;
float m_damage;

void updateAI(float deltaTime);
void die();
void attack(GameEntity* target);


};