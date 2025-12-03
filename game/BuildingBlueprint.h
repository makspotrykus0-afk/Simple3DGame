#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include "raylib.h"
#include "raymath.h"
#include "../systems/ResourceTypes.h"

enum class BuildingCategory {
    STRUCTURE,
    RESIDENTIAL,
    STORAGE,
    PRODUCTION,
    FURNITURE // Added for Bed
};

struct ResourceRequirement {
    std::string resourceType;
    int amount;
    bool isOptional;
    std::string description;

    ResourceRequirement(std::string type, int amt, bool optional = false, std::string desc = "")
        : resourceType(std::move(type)), amount(amt), isOptional(optional), description(std::move(desc)) {}
};

struct BlueprintComponent {
    std::string blueprintId;
    Vector3 localPosition;
    float localRotation;
};

class BuildingBlueprint {
public:
    BuildingBlueprint(std::string id, std::string name, BuildingCategory category)
        : m_id(id), m_name(name), m_category(category), m_walkable(false), m_model({ 0 }) {}
    
    // Keep old constructor for compatibility if needed
    BuildingBlueprint(std::string id, std::string name, std::string description)
        : m_id(id), m_name(name), m_description(description), m_category(BuildingCategory::STRUCTURE), m_walkable(false), m_model({ 0 }) {}

    ~BuildingBlueprint() {
        unloadModel();
    }

    const std::string& getId() const { return m_id; }
    const std::string& getName() const { return m_name; }
    const std::string& getDescription() const { return m_description; }
    BuildingCategory getCategory() const { return m_category; }
    
    void setModelPath(const std::string& path) { m_modelPath = path; }
    const std::string& getModelPath() const { return m_modelPath; }
    
    void loadModel() {
        if (!m_modelPath.empty() && m_model.meshCount == 0) {
            m_model = LoadModel(m_modelPath.c_str());
        }
    }

    void unloadModel() {
        if (m_model.meshCount > 0) {
            UnloadModel(m_model);
            m_model = { 0 };
        }
    }

    Model* getModel() {
        if (m_model.meshCount > 0) return &m_model;
        return nullptr;
    }

    const Model* getModel() const {
        if (m_model.meshCount > 0) return &m_model;
        return nullptr;
    }

    void addCost(Resources::ResourceType type, int amount) {
        m_requirements.push_back(ResourceRequirement(Resources::resourceTypeToString(type), amount));
    }
    
    void setSize(Vector3 size) { m_size = size; }
    Vector3 getSize() const { return m_size; }
    
    void setCollisionBox(BoundingBox box) { m_collisionBox = box; }
    BoundingBox getCollisionBox() const { return m_collisionBox; }
    
    void setWalkable(bool w) { m_walkable = w; }
    bool isWalkable() const { return m_walkable; }
    
    // Component system for composite buildings
    void addComponent(const BlueprintComponent& comp) { m_components.push_back(comp); }
    const std::vector<BlueprintComponent>& getComponents() const { return m_components; }

    // Compatibility methods
    const std::vector<ResourceRequirement>& getRequirements() const { return m_requirements; }
    
    float getBuildTime() const { return 5.0f; } 
    float getMaxHealth() const { return 100.0f; }

private:
    std::string m_id;
    std::string m_name;
    std::string m_description;
    BuildingCategory m_category;
    std::string m_modelPath;
    Model m_model;
    Vector3 m_size = { 1.0f, 1.0f, 1.0f };
    BoundingBox m_collisionBox = { {-0.5f, 0.0f, -0.5f}, {0.5f, 1.0f, 0.5f} };
    bool m_walkable;
    
    std::vector<ResourceRequirement> m_requirements;
    std::vector<BlueprintComponent> m_components;
};
