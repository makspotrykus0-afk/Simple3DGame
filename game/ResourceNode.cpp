#include "ResourceNode.h"
#include "../systems/ResourceDefinitions.h"
#include <iostream>
#include <raylib.h>
#include "../game/Item.h"
#include "../core/GameEngine.h"

ResourceNode::ResourceNode(Resources::ResourceType type, const PositionComponent& position, float amount)
    : GameEntity("resource_node_" + std::to_string(static_cast<int>(type))), m_type(type), m_currentAmount(static_cast<int32_t>(amount)), m_maxAmount(static_cast<int32_t>(amount)), m_regenerationRate(0.0f) {
    
    // Set entity position
    addComponent(std::make_shared<PositionComponent>(position));
}

void ResourceNode::update(float deltaTime) {
    // Potential regeneration logic here
}

void ResourceNode::render() {
    if (isDepleted()) return;

    std::shared_ptr<PositionComponent> posComp = getComponent<PositionComponent>();
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
    std::shared_ptr<const PositionComponent> pos = getComponent<PositionComponent>();
    return pos ? pos->getPosition() : Vector3{0,0,0}; // Use accessor method
}

void ResourceNode::setPosition(const Vector3& position) {
    std::shared_ptr<PositionComponent> pos = getComponent<PositionComponent>();
    if (pos) {
        pos->setPosition(position);
    }
}

std::string ResourceNode::getName() const {
    return resourceTypeToString(m_type);
}

void ResourceNode::reserve(const std::string& reservedBy) {
    m_reservedBy = reservedBy;
}

void ResourceNode::releaseReservation() {
    m_reservedBy.clear();
}

bool ResourceNode::isReserved() const {
    return !m_reservedBy.empty();
}

std::string ResourceNode::getReservedBy() const {
    return m_reservedBy;
}

Resources::ResourceType ResourceNode::getResourceType() const {
    return m_type;
}

float ResourceNode::harvest(float amount) {
    if (m_currentAmount <= 0) return 0.0f;
    
    float actualHarvested = amount;
    if (actualHarvested > m_currentAmount) {
        actualHarvested = static_cast<float>(m_currentAmount);
    }
    
    m_currentAmount -= static_cast<int>(actualHarvested);
    
    if (m_currentAmount <= 0) {
        // Depleted - drop item
        Vector3 pos = getPosition();
        pos.y += 0.5f;

        std::string resType;
        std::string resName;
        
        switch(m_type) {
            case Resources::ResourceType::Wood: resType = "Wood"; resName = "Wood Log"; break;
            case Resources::ResourceType::Stone: resType = "Stone"; resName = "Stone Chunk"; break;
            case Resources::ResourceType::Food: resType = "Food"; resName = "Berries"; break;
            case Resources::ResourceType::Metal: resType = "Metal"; resName = "Metal Ore"; break;
            case Resources::ResourceType::Gold: resType = "Gold"; resName = "Gold Nugget"; break;
        }
        
        if (!resType.empty() && GameEngine::dropItemCallback) {
             Item* droppedItem = new ResourceItem(resType, resName, "Harvested resource.");
             GameEngine::dropItemCallback(pos, droppedItem, 1);
             std::cout << "[ResourceNode] Dropped " << resName << " at (" << pos.x << "," << pos.y << "," << pos.z << ")" << std::endl;
        }
    }

    return actualHarvested;
}

BoundingBox ResourceNode::getBoundingBox() const {
    // Kamień jako sześcian 1.5x1.5x1.5m
    float size = 1.5f;
    Vector3 pos = getPosition();
    return BoundingBox{
        Vector3{ pos.x - size/2, pos.y, pos.z - size/2 },
        Vector3{ pos.x + size/2, pos.y + size, pos.z + size/2 }
    };
}