#include "Tree.h"
#include "../core/GameEngine.h"
#include "../game/Item.h"
#include "raymath.h"
#include "rlgl.h"
#include <algorithm> // std::min
#include <iostream>

Tree::Tree(const PositionComponent &position, float health, float woodAmount)
    : GameEntity("Tree"), m_woodAmount(woodAmount), m_isStump(false),
      m_isReserved(false), m_rotation(0.0f) {

  m_fixedPosition = position.getPosition();

  auto posComp = std::make_shared<PositionComponent>(position);
  addComponent(posComp);
  posComp->setPosition(m_fixedPosition);

  addComponent(
      std::make_shared<StatsComponent>("Tree", health, 100.0f, 100.0f, 100.0f));
}

InteractionResult Tree::interact(GameEntity *player) {
  (void)player;
  if (m_isStump)
    return {false, "Already chopped."};

  // Interaction acts as a manual chop/harvest test
  // In real gameplay, Settler uses harvest()
  takeDamage(10.0f);
  return {true, "Chopping tree..."};
}

InteractionInfo Tree::getDisplayInfo() const {
  InteractionInfo info;
  info.objectName = "Tree";
  info.objectDescription = "A resource for wood.";
  info.availableActions.push_back("Wood: " + std::to_string((int)m_woodAmount));
  return info;
}

Vector3 Tree::getPosition() const { return m_fixedPosition; }

float Tree::getWoodAmount() const { return m_woodAmount; }

void Tree::takeDamage(float damage) {
  if (m_isStump)
    return;

  auto stats = getComponent<StatsComponent>();
  if (stats) {
    stats->modifyHealth(-damage);
    if (stats->getCurrentHealth() <= 0) {
      chopDown(true); // Destroyed by damage -> drops items
    }
  }
}
float Tree::harvest(float amount) {
  if (m_isStump || m_woodAmount <= 0)
    return 0.0f;

  float taken = std::min(amount, m_woodAmount);
  m_woodAmount -= taken;

  if (m_woodAmount <= 0) {
    std::cout << "[Tree] Wood depleted via harvest, dropping log." << std::endl;
    chopDown(true); // Depleted by harvesting -> drop log
  }

  return taken;
}
void Tree::chopDown(bool dropItems) {
  if (m_isStump || m_isFalling)
    return;

  m_isFalling = true;
  m_fallAngle = 0.0f;
  m_fallSpeed = 0.0f;
  
  // Random fall direction
  float randomAngle = (float)(rand() % 360) * DEG2RAD;
  m_fallAxis = {cosf(randomAngle), 0.0f, sinf(randomAngle)};
  
  // m_woodAmount = 0; // Wait until fallen to clear? Or clear now?
  // Let's keep woodAmount until it falls to prevent ghost harvesting, 
  // but isActive() checks woodAmount. So clear it now to prevent further chops.
  m_woodAmount = 0;

  // Clear reservation when chopped
  releaseReservation();

  Vector3 pos = getPosition();
  pos.y += 0.5f;
  pos.x += 1.0f;

  std::cout << "[Tree] Chopped down, spawning wood log at (" << pos.x << ", "
            << pos.y << ", " << pos.z << ")" << std::endl;

  if (dropItems && GameEngine::dropItemCallback) {
    // Only drop item if explicitly requested (e.g. destroyed by damage, not
    // harvested) Or maybe drop a small amount if destroyed? For now, sticking
    // to logic: if harvested fully, no drop. If destroyed, drop 1 log.
    Item *log = new ResourceItem("Wood", "Wood Log", "Freshly chopped wood.");
    GameEngine::dropItemCallback(pos, log);
  } else if (dropItems) {
    std::cout << "[Tree] WARNING: dropItemCallback is null, log not spawned."
              << std::endl;
  }
}

void Tree::update(float deltaTime) {
    if (m_isFalling) {
        m_fallSpeed += 300.0f * deltaTime; // Gravity acceleration
        m_fallAngle += m_fallSpeed * deltaTime;
        
        if (m_fallAngle >= 90.0f) {
            m_fallAngle = 90.0f;
            m_isFalling = false;
            m_isStump = true; // Final state
            
            // Screenshake or dust effect here?
        }
    }
}

void Tree::render() {
  Vector3 pos = getPosition();

  // Visual Debug: small dot at the base to confirm ground contact
  // DrawSphere(pos, 0.1f, RED);

  if (m_isStump) {
    DrawCylinderEx(Vector3{pos.x, pos.y - 0.2f, pos.z},
                   Vector3{pos.x, pos.y + 0.5f, pos.z}, 0.4f, 0.4f, 8, BROWN);
  } else {
    // Apply rotation
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(m_rotation, 0, 1, 0);
    
    // Impact Physics Fall
    if (m_isFalling) {
        rlRotatef(m_fallAngle, m_fallAxis.x, m_fallAxis.y, m_fallAxis.z);
    }
    
    rlTranslatef(-pos.x, -pos.y, -pos.z);

    // Trunk
    // Start slightly below ground (-0.2f) to avoid gaps
    Vector3 trunkStart = {pos.x, pos.y - 0.2f, pos.z};
    Vector3 trunkEnd = {pos.x, pos.y + 3.0f, pos.z};
    DrawCylinderEx(trunkStart, trunkEnd, 0.4f, 0.4f, 8, BROWN);

    // Foliage (Cone)
    Vector3 foliageStart = {pos.x, pos.y + 2.0f, pos.z};
    Vector3 foliageEnd = {pos.x, pos.y + 5.0f, pos.z};
    DrawCylinderEx(foliageStart, foliageEnd, 1.5f, 0.0f, 8, DARKGREEN);

    rlPopMatrix(); // Restore matrix

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