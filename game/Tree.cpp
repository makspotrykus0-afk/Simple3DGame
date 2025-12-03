#include "Tree.h"
#include <iostream>
#include "../game/Item.h"
#include "../core/GameEngine.h"
#include "raymath.h"
#include <algorithm> // std::min

Tree::Tree(const PositionComponent& position, float health, float woodAmount)
    : GameEntity("Tree"), m_woodAmount(woodAmount), m_isStump(false), m_isReserved(false) {
    
    m_fixedPosition = position.getPosition();
    
    auto posComp = std::make_shared<PositionComponent>(position);
    addComponent(posComp);
    posComp->setPosition(m_fixedPosition);
    
    addComponent(std::make_shared<StatsComponent>("Tree", health, 100.0f, 100.0f, 100.0f));
}

InteractionResult Tree::interact(GameEntity* player) {
    (void)player;
    if (m_isStump) return { false, "Already chopped." };
    
    // Interaction acts as a manual chop/harvest test
    // In real gameplay, Settler uses harvest()
    takeDamage(10.0f);
    return { true, "Chopping tree..." };
}

InteractionInfo Tree::getDisplayInfo() const {
    InteractionInfo info;
    info.objectName = "Tree";
    info.objectDescription = "A resource for wood.";
    info.availableActions.push_back("Wood: " + std::to_string((int)m_woodAmount));
    return info;
}

Vector3 Tree::getPosition() const {
    return m_fixedPosition;
}

float Tree::getWoodAmount() const {
    return m_woodAmount;
}

void Tree::takeDamage(float damage) {
    if (m_isStump) return;
    
    auto stats = getComponent<StatsComponent>();
    if (stats) {
        stats->modifyHealth(-damage);
        if (stats->getCurrentHealth() <= 0) {
            chopDown(true); // Destroyed by damage -> drops items
        }
    }
}

float Tree::harvest(float amount) {
    if (m_isStump || m_woodAmount <= 0) return 0.0f;

    float taken = std::min(amount, m_woodAmount);
    m_woodAmount -= taken;

    if (m_woodAmount <= 0) {
        chopDown(false); // Depleted by harvesting -> no extra drop
    }

    return taken;
}

void Tree::chopDown(bool dropItems) {
    if (m_isStump) return;
    
    m_isStump = true;
    m_woodAmount = 0;
    
    // Clear reservation when chopped
    releaseReservation();
    
    Vector3 pos = getPosition();
    pos.y += 0.5f; 
    pos.x += 1.0f; 
    
    if (dropItems && GameEngine::dropItemCallback) {
        // Only drop item if explicitly requested (e.g. destroyed by damage, not harvested)
        // Or maybe drop a small amount if destroyed?
        // For now, sticking to logic: if harvested fully, no drop. If destroyed, drop 1 log.
        Item* log = new ResourceItem("Wood", "Wood Log", "Freshly chopped wood.");
        GameEngine::dropItemCallback(pos, log);
    }
}

void Tree::render() {
    Vector3 pos = getPosition();
    
    // Visual Debug: small dot at the base to confirm ground contact
    // DrawSphere(pos, 0.1f, RED);

    if (m_isStump) {
        DrawCylinderEx(Vector3{pos.x, pos.y - 0.2f, pos.z}, Vector3{pos.x, pos.y + 0.5f, pos.z}, 0.4f, 0.4f, 8, BROWN);
    } else {
        // Trunk
        // Start slightly below ground (-0.2f) to avoid gaps
        Vector3 trunkStart = { pos.x, pos.y - 0.2f, pos.z };
        Vector3 trunkEnd = { pos.x, pos.y + 3.0f, pos.z };
        DrawCylinderEx(trunkStart, trunkEnd, 0.4f, 0.4f, 8, BROWN);
        
        // Foliage (Cone)
        Vector3 foliageStart = { pos.x, pos.y + 2.0f, pos.z };
        Vector3 foliageEnd = { pos.x, pos.y + 5.0f, pos.z };
        DrawCylinderEx(foliageStart, foliageEnd, 1.5f, 0.0f, 8, DARKGREEN);
        
        // HP Bar
        auto stats = getComponent<StatsComponent>();
        if (stats && stats->getCurrentHealth() < stats->getMaxHealth()) {
            Vector3 barPos = pos;
            barPos.y += 5.5f;
            
            float healthPct = stats->getCurrentHealth() / stats->getMaxHealth();
            
            DrawCube(barPos, 1.0f, 0.1f, 0.1f, RED);
            DrawCube(barPos, 1.0f * healthPct, 0.12f, 0.12f, GREEN);
        }
    }
}