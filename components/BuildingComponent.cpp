#include "BuildingComponent.h"

BuildingComponent::BuildingComponent(const std::string& ownerId)
    : m_ownerId(ownerId), m_maxConcurrentBuildings(0), m_baseBuildingSpeed(0.0f) {
    // Initialize any other members here
}

void BuildingComponent::setMaxConcurrentBuildings(size_t maxCount) {
    m_maxConcurrentBuildings = maxCount;
}

void BuildingComponent::setBaseBuildingSpeed(float speed) {
    m_baseBuildingSpeed = speed;
}

size_t BuildingComponent::getMaxConcurrentBuildings() const {
    return m_maxConcurrentBuildings;
}

float BuildingComponent::getBaseBuildingSpeed() const {
    return m_baseBuildingSpeed;
}

// Add other method implementations as needed

void BuildingComponent::update(float deltaTime) {
    // Implement building update logic if necessary
}

void BuildingComponent::render() {
    // Implement building rendering logic if necessary
}

void BuildingComponent::initialize() {
    // Initialize building component
}

void BuildingComponent::shutdown() {
    // Shutdown building component
}

std::type_index BuildingComponent::getComponentType() const {
    return std::type_index(typeid(BuildingComponent));
}