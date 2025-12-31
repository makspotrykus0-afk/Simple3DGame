#include "TreeComponent.h"
#include "../game/Tree.h" // For Tree properties like wood amount
#include "../components/StatsComponent.h" // Assuming StatsComponent manages health

TreeComponent::TreeComponent(float /*maxHealth*/, float woodAmount)
    : m_woodAmount(woodAmount)
{
    // We assume that the entity this component is attached to already has a StatsComponent
    // that manages health. If not, this component might need to manage it, or ensure it's added.
}

void TreeComponent::takeDamage(float damage) {
    // Assuming StatsComponent exists and manages health.
    // If StatsComponent is not present or doesn't handle health, this needs adjustment.
    // For now, we directly decrease wood amount if it's considered "dead" for resource purposes.
    if (m_woodAmount > 0) {
        m_woodAmount -= damage; // This is a simplified representation. Real health management is in StatsComponent.
        if (m_woodAmount < 0) m_woodAmount = 0;
    }
}