#include "ResourceNode.h"
#include "../systems/ResourceDefinitions.h"
#include <iostream>
#include <raylib.h>

ResourceNode::ResourceNode(Resources::ResourceType type, const PositionComponent& position, float amount)
    : m_type(type), m_currentAmount(static_cast<int32_t>(amount)), m_maxAmount(static_cast<int32_t>(amount)), m_regenerationRate(0.0f) {
    
    // Set entity position
    addComponent<PositionComponent>(position);
}

void ResourceNode::update(float deltaTime) {
    // Potential regeneration logic here
}

void ResourceNode::render() {
    if (isDepleted()) return;

    const PositionComponent* posComp = getComponent<PositionComponent>();
    if (!posComp) return;
    
    Vector3 pos = posComp->getPosition(); // Use accessor method
    
    // Offset Y to prevent sinking into ground (DrawCube draws from center)
    pos.y += 0.5f;

    Color color = GRAY; // Default for Stone

    switch (m_type) {
        case Resources::ResourceType::Wood: color = BROWN; break;
        case Resources::ResourceType::Stone: color = GRAY; break;
        case Resources::ResourceType::Food: color = ORANGE; break; // Bush?
        case Resources::ResourceType::Metal: color = DARKGRAY; break;
        case Resources::ResourceType::Gold: color = YELLOW; break;
        default: break;
    }

    // Simple geometric representation for now
    DrawCube(pos, 1.0f, 1.0f, 1.0f, color);
    DrawCubeWires(pos, 1.0f, 1.0f, 1.0f, DARKGRAY);
}

int32_t ResourceNode::getCurrentAmount() const {
    return m_currentAmount;
}

bool ResourceNode::isDepleted() const {
    return m_currentAmount <= 0;
}

InteractionResult ResourceNode::interact(GameEntity* player) {
    InteractionResult result;
    
    if (isDepleted()) {
        result.success = false;
        result.message = "Resource depleted.";
        return result;
    }

    // Logic for gathering resource
    // For now, simple immediate gathering of small amount
    int amountToGather = 10;
    if (m_currentAmount < amountToGather) {
        amountToGather = m_currentAmount;
    }
    
    m_currentAmount -= amountToGather;
    
    result.success = true;
    result.message = "Gathered " + std::to_string(amountToGather) + " " + resourceTypeToString(m_type);
    
    return result;
}

InteractionInfo ResourceNode::getDisplayInfo() const {
    InteractionInfo info;
    info.objectName = getName();
    info.interactionPrompt = "Press E to Gather";
    info.objectDescription = "Contains " + std::to_string(m_currentAmount) + " resources";
    info.canInteract = !isDepleted();
    info.position = getPosition();
    return info;
}

Vector3 ResourceNode::getPosition() const {
    const PositionComponent* pos = getComponent<PositionComponent>();
    return pos ? pos->getPosition() : Vector3{0,0,0}; // Use accessor method
}

std::string ResourceNode::getName() const {
    return resourceTypeToString(m_type);
}