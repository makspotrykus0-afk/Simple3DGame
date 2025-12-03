#include "ResourceComponent.h"
#include <algorithm>

ResourceComponent::ResourceComponent(const std::string& ownerId)
    : m_ownerId(ownerId) {
}

void ResourceComponent::initialize() {
    // Inicjalizacja komponentu zasobów
    // Tutaj można dodać dodatkową inicjalizację, jeśli jest potrzebna
}

void ResourceComponent::update(float deltaTime) {
    // Aktualizacja komponentu zasobów
    // Tutaj można dodać logikę aktualizacji, jeśli jest potrzebna
}

void ResourceComponent::render() {
    // Renderowanie komponentu zasobów
    // Tutaj można dodać logikę renderowania, jeśli jest potrzebna
}

void ResourceComponent::shutdown() {
    // Zamykanie komponentu zasobów
    m_resources.clear();
}

std::type_index ResourceComponent::getComponentType() const {
    return std::type_index(typeid(ResourceComponent));
}

int32_t ResourceComponent::addResource(const std::string& resourceType, int32_t amount) {
    if (amount <= 0) {
        return 0;
    }

    auto& resource = m_resources[resourceType];
    resource.resourceType = resourceType;
    resource.currentAmount += amount;
    resource.lastGained = std::chrono::high_resolution_clock::now();

    return amount;
}

int32_t ResourceComponent::removeResource(const std::string& resourceType, int32_t amount) {
    if (amount <= 0) {
        return 0;
    }

    auto it = m_resources.find(resourceType);
    if (it == m_resources.end() || it->second.currentAmount < amount) {
        return 0; // Nie wystarczająca ilość zasobu
    }

    it->second.currentAmount -= amount;
    it->second.lastSpent = std::chrono::high_resolution_clock::now();

    return amount;
}

int32_t ResourceComponent::getResourceAmount(const std::string& resourceType) const {
    auto it = m_resources.find(resourceType);
    if (it != m_resources.end()) {
        return it->second.currentAmount;
    }
    return 0;
}