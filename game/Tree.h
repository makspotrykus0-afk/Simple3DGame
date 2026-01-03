#pragma once

#include <string>
#include <vector>

// Dołączamy raylib.h, aby uzyskać dostęp do definicji Vector3 i innych typów
// raylib
#include "../components/PositionComponent.h" // Użycie ścieżki względnej
#include "../components/StatsComponent.h"    // Użycie ścieżki względnej
#include "../entities/GameEntity.h"          // Changed to GameEntity base class
#include "InteractableObject.h"              // Include for InteractableObject
#include "raylib.h"                          // Zawiera definicje Vector3


class Tree : public GameEntity,
             public InteractableObject { // Inherit from GameEntity AND
                                         // InteractableObject
public:
  Tree(const PositionComponent &position, float health, float woodAmount);

  // Implement InteractableObject interface
  InteractionType getInteractionType() const override {
    return InteractionType::RESOURCE_GATHERING;
  }
  float getInteractionRange() const override { return 3.0f; }
  bool canInteract(GameEntity * /*player*/) const override {
    return m_woodAmount > 0 && !m_isStump;
  }
  InteractionResult interact(GameEntity *player) override;
  InteractionInfo getDisplayInfo() const override;
  Vector3 getPosition() const override;
  void setPosition(const Vector3 &position) override {
    m_fixedPosition = position;
  }
  std::string getName() const override { return "Tree"; }
  bool isActive() const override { return m_woodAmount > 0; }

  float getWoodAmount() const;
  void takeDamage(float damage);

  // Harvest resources from the tree. Returns amount harvested.
  // If wood runs out, tree becomes a stump.
  float harvest(float amount);

  void chopDown(bool dropItems = true); // Triggers falling animation
  void update(float deltaTime); // Updates falling animation
  void render();

  bool isStump() const { return m_isStump; }
  bool isFalling() const { return m_isFalling; } // Getter for falling state
  bool shouldBeRemoved() const { return false; } // Pniaki zostają

  // Reservation System
  bool isReserved() const { return m_isReserved; }
  bool isReservedBy(const std::string &settlerName) const {
    return m_isReserved && m_reservedBy == settlerName;
  }
  void reserve(const std::string &settlerName) {
    m_isReserved = true;
    m_reservedBy = settlerName;
  }
  void releaseReservation() {
    m_isReserved = false;
    m_reservedBy = "";
  }
  std::string getReservedBy() const { return m_reservedBy; }

  float getMaxWoodAmount() const {
    return m_maxWoodAmount;
  }

  // Collision
  BoundingBox getBoundingBox() const {
    // Pień drzewa jako proste pudełko kolizyjne
    // Zakładamy szerokość pnia ok 1.0f i wysokość 4.0f (chyba że to pniak)
    float width = 1.0f;
    float height = m_isStump ? 0.5f : 4.0f;
    return BoundingBox{Vector3{m_fixedPosition.x - width / 2, m_fixedPosition.y,
                               m_fixedPosition.z - width / 2},
                       Vector3{m_fixedPosition.x + width / 2,
                               m_fixedPosition.y + height,
                               m_fixedPosition.z + width / 2}};
  }

  // Rotation Support
  float getRotation() const { return m_rotation; }
  void setRotation(float rotation) { m_rotation = rotation; }

private:
  float m_woodAmount; // Przechowuje ilość drewna dostępnego do zebrania
  float m_maxWoodAmount; // Stores initial max wood
  bool m_isStump;
  Vector3 m_fixedPosition; // Przechowuje pozycję niezależnie od komponentów
  float m_rotation = 0.0f; // Rotation in degrees

  // Falling Physics
  bool m_isFalling = false;
  float m_fallAngle = 0.0f;
  Vector3 m_fallAxis = {1.0f, 0.0f, 0.0f};
  float m_fallSpeed = 0.0f;

  // Reservation
  bool m_isReserved;
  std::string m_reservedBy;
};