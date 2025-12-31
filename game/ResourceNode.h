#ifndef RESOURCE_NODE_H
#define RESOURCE_NODE_H

#include <cstdint>
#include <string>
#include "../systems/ResourceTypes.h"
#include "../entities/GameEntity.h"
#include "InteractableObject.h"
#include "../components/PositionComponent.h"

// ResourceNode is now an interactable object in the world
class ResourceNode : public GameEntity, public InteractableObject {
public:
    ResourceNode(Resources::ResourceType type, const PositionComponent& position, float amount);
    
    void update(float deltaTime);
    void render();
    int32_t getCurrentAmount() const;
    int32_t getMaxAmount() const { return m_maxAmount; }
    bool isDepleted() const;
    
    // Implement InteractableObject interface
    InteractionType getInteractionType() const override { return InteractionType::RESOURCE_GATHERING; }
    float getInteractionRange() const override { return 3.0f; }
    bool canInteract(GameEntity* /*player*/) const override { return m_currentAmount > 0; }
    InteractionResult interact(GameEntity* player) override;
    InteractionInfo getDisplayInfo() const override;
    Vector3 getPosition() const override;
    std::string getName() const override;
    bool isActive() const override { return m_currentAmount > 0; }

    // Added methods for Settler interaction
    void reserve(const std::string& reservedBy);
    void releaseReservation();
    bool isReserved() const;
    std::string getReservedBy() const;
    
    Resources::ResourceType getResourceType() const;
    float harvest(float amount);

    // GameEntity implementation
    void setPosition(const Vector3& position) override;

private:
    Resources::ResourceType m_type;
    int32_t m_currentAmount;
    int32_t m_maxAmount;
    float m_regenerationRate;
    std::string m_reservedBy;
};

#endif // RESOURCE_NODE_H