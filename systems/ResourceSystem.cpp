#include "ResourceSystem.h"
#include "../core/EventSystem.h"
#include <iostream>

ResourceSystem::ResourceSystem() : m_name("ResourceSystem") {
}

ResourceSystem::~ResourceSystem() {
    shutdown();
}

void ResourceSystem::initialize() {
    // Initialize resources
    m_resources[Resources::ResourceType::Wood] = Resources::Resource(Resources::ResourceType::Wood, "Wood", 99);
    m_resources[Resources::ResourceType::Stone] = Resources::Resource(Resources::ResourceType::Stone, "Stone", 99);
    m_resources[Resources::ResourceType::Metal] = Resources::Resource(Resources::ResourceType::Metal, "Metal", 50);
    m_resources[Resources::ResourceType::Gold] = Resources::Resource(Resources::ResourceType::Gold, "Gold", 50);
    m_resources[Resources::ResourceType::Food] = Resources::Resource(Resources::ResourceType::Food, "Food", 200);
    m_resources[Resources::ResourceType::Water] = Resources::Resource(Resources::ResourceType::Water, "Water", 200);

    // Rejestracja handlerów zdarzeń
    EventBus::registerHandler<Resources::ResourceChangedEvent>(
        [this](const std::any& data) {
            try {
                const auto& event = std::any_cast<const Resources::ResourceChangedEvent&>(data);
                std::cout << "Resource Changed: " << event.playerId 
                          << " Type: " << static_cast<int>(event.type) 
                          << " Change: " << event.change 
                          << " New Amount: " << event.newAmount << std::endl;
            } catch (const std::bad_any_cast& e) {
                std::cerr << "Error handling ResourceChangedEvent: " << e.what() << std::endl;
            }
        },
        "ResourceSystem"
    );
}

void ResourceSystem::update(float deltaTime) {
    // Aktualizacja systemu
}

void ResourceSystem::render() {
    // Renderowanie systemu
}

void ResourceSystem::shutdown() {
    m_resources.clear();
}

const Resources::Resource* ResourceSystem::getResourceInfo(Resources::ResourceType type) const {
    auto it = m_resources.find(type);
    if (it != m_resources.end()) {
        return &it->second;
    }
    return nullptr;
}
