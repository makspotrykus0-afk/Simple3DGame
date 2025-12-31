#pragma once
#include "InteractableObject.h"
#include "../core/GameEntity.h"

class Bed : public BaseInteractableObject {
public:
    Bed(const std::string& name, Vector3 position, float rotation);
    virtual ~Bed() = default;

    bool isOccupied() const;
    void setOccupied(bool occupied);

    // New methods for sleeping logic
    bool startSleeping(GameEntity* sleeper);
    void stopSleeping();
    GameEntity* getSleeper() const;

    // Override methods from BaseInteractableObject
    InteractionResult interact(GameEntity* player) override;
    InteractionInfo getDisplayInfo() const override;

    void render(); // Custom render method
    void update(float deltaTime) override;

    InteractionType getInteractionType() const override { return InteractionType::INSPECTION; }

private:
    bool m_occupied;
    float m_rotation;
    BoundingBox m_boundingBox;
    GameEntity* m_currentSleeper; // Pointer to the entity currently sleeping
};