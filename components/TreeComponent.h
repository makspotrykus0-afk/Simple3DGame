#pragma once

#include "../core/GameComponent.h"
#include "../core/Entity.h"
#include "StatsComponent.h" // Assuming StatsComponent manages health
#include "../game/Tree.h" // For Tree properties like wood amount

class TreeComponent : public GameComponent {
public:
    TreeComponent(float maxHealth, float woodAmount);

    void takeDamage(float damage);
    float getWoodAmount() const { return m_woodAmount; }
    void decreaseWoodAmount(float amount) { m_woodAmount -= amount; }
    bool isDead() const { return m_woodAmount <= 0; }

private:
    float m_woodAmount;
    // Health will be managed by StatsComponent, so we don't need it here directly
};