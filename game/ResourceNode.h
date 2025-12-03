#ifndef RESOURCE_NODE_H
#define RESOURCE_NODE_H

#include <cstdint>
#include <string>
#include "../systems/ResourceTypes.h"
#include "../core/Entity.h"
#include "InteractableObject.h"
#include "../components/PositionComponent.h"

// ResourceNode is now an interactable object in the world
class ResourceNode : public Entity, public InteractableObject {
public:
    ResourceNode(Resources::ResourceType type, const PositionComponent& position, float amount);
    
    void update(float deltaTime);
    void render();
    int32_t getCurrentAmount() const;
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

private:
    Resources::ResourceType m_type;
    int32_t m_currentAmount;
    int32_t m_maxAmount;
    float m_regenerationRate;
};

#endif // RESOURCE_NODE_H