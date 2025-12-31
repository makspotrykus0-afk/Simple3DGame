#pragma once

#include "../core/IGameSystem.h"
#include "ResourceTypes.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <any>

namespace Resources {
    // ResourceType is now defined in ResourceTypes.h

    /**
     * @brief Definicja zasobu
     */
    struct Resource {
        Resources::ResourceType type;
        std::string name;
        std::string description;
        int32_t maxStack;
        float weight;
        float volume;
        
        Resource(Resources::ResourceType t = Resources::ResourceType::None, const std::string& n = "", int32_t stack = 99)
            : type(t), name(n), maxStack(stack), weight(1.0f), volume(1.0f) {}
    };

    // Simple event struct
    struct ResourceChangedEvent {
        std::string playerId;
        Resources::ResourceType type;
        int32_t oldAmount;
        int32_t newAmount;
        int32_t change;
        std::string source;
        double timestamp;
    };
}

class ResourceSystem : public IGameSystem {
public:
    ResourceSystem();
    ~ResourceSystem() override;

    void initialize() override;
    void update(float deltaTime) override;
    void render() override;
    void shutdown() override;
    std::string getName() const override { return m_name; }
    int getPriority() const override { return 5; } // Default priority
    
    // Helper to get resource info
    const Resources::Resource* getResourceInfo(Resources::ResourceType type) const;

private:
    std::string m_name;
    std::unordered_map<Resources::ResourceType, Resources::Resource> m_resources;
};