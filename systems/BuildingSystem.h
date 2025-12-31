#pragma once

#include "../core/GameSystem.h"
#include "../game/BuildingBlueprint.h"
#include "../game/BuildingInstance.h"
#include "../game/BuildingTask.h"
#include "InteractionSystem.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <typeindex> // Ensure type_index is included
#include "raylib.h"

class Colony;
class StorageSystem;

class BuildingSystem : public GameSystem {
public:
    BuildingSystem();
    virtual ~BuildingSystem() { shutdown(); }

    void initialize() override;
    void shutdown() override;
    void update(float dt) override;
    void render() override;
    // Removing getComponentType as it is not part of GameSystem interface and caused confusion
    // If BuildingSystem needs to identify itself, it can use typeid() directly or we can add it to IGameSystem
    // But standard GameSystem pattern usually uses RTTI or name.
    // std::type_index getComponentType() const;

    // Blueprint management
    void registerDefaultBlueprints();
    void registerBlueprint(std::unique_ptr<BuildingBlueprint> blueprint);
    void loadBlueprints(const std::string& filepath);
    BuildingBlueprint* getBlueprint(const std::string& id) const;
    void ensureBlueprintExists(const std::string& blueprintId);
    std::vector<BuildingBlueprint*> getAvailableBlueprints() const;

    // Building operations
    bool canBuild(const std::string& blueprintId, Vector3 position);
    BuildTask* startBuilding(const std::string& blueprintId, Vector3 position, GameEntity* builder = nullptr, float rotation = 0.0f, bool forceNoSnap = false, bool ignoreCollision = false, bool instant = false, bool* outSuccess = nullptr);
    void cancelBuilding(BuildTask* task);
    void completeBuilding(BuildTask* task);

    // Queries
    std::vector<BuildingInstance*> getBuildingsInRange(Vector3 center, float radius) const;
    std::vector<BuildingInstance*> getAllBuildings() const;
    BuildingInstance* getBuildingAt(Vector3 position) const;
    BuildTask* getBuildTaskAt(Vector3 position, float radius = 1.0f) const;
    int getPendingBuildCount(const std::string& blueprintId) const;
    std::vector<BuildTask*> getActiveBuildTasks() const;
    
    // New methods to fix compilation errors
    void enablePlanningMode(bool enable);
    void setSelectedBlueprint(const std::string& blueprintId);
    void updatePreviewPosition(Vector3 pos);
    void renderPreview(const std::string& blueprintId, Vector3 position, float rotation);
    private:
    // Helper function to ensure storage is created for a building instance
    void EnsureStorageForBuildingInstance(BuildingInstance* building);
    bool getBuildingAtRay(Ray ray, BuildingInstance** outBuilding) const;

    // Dependencies
    void setInteractionSystem(InteractionSystem* system) { m_interactionSystem = system; }
    void setColony(Colony* colony) { m_colony = colony; }
    void setStorageSystem(StorageSystem* system) { m_storageSystem = system; }

private:
    void renderStorageContents(BuildingInstance* building);

    std::unordered_map<std::string, std::unique_ptr<BuildingBlueprint>> m_blueprints;
    std::vector<std::unique_ptr<BuildingInstance>> m_buildings;
    std::vector<std::unique_ptr<BuildTask>> m_buildTasks;

    InteractionSystem* m_interactionSystem;
    Colony* m_colony;
    StorageSystem* m_storageSystem = nullptr;
    
    float m_gridSize;
    bool m_snapToGrid;
    
    // Planning state
    bool m_isPlanning;
    std::string m_selectedBlueprintId;
    Vector3 m_previewPosition;
};
