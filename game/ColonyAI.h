#ifndef COLONY_AI_H
#define COLONY_AI_H

#include <vector>
#include <memory>
#include <string>
#include "raylib.h"

// Forward declaration
class Colony;
class BuildingSystem;

class ColonyAI {
public:
    ColonyAI(Colony* colony, BuildingSystem* buildingSystem);
    ~ColonyAI();

    void update(float deltaTime);
    
    // Register an existing house (e.g. pre-built)
    void registerHouse(Vector3 pos, int size, bool occupied);

private:
    Colony* m_colony;
    BuildingSystem* m_buildingSystem;
    float m_timer;
    float m_updateInterval;
    struct HouseInfo {
        Vector3 position;
        int size;
        bool occupied;
        float timestamp; // Creation time
    };
    std::vector<HouseInfo> m_knownHouses;

    // Potrzeby
    bool m_needsHousing;

    void checkNeeds();
    void processConstructionRequests();
    void assignJobs(); // Added method declaration
    
public:
    // Public for initializing houses
    Vector3 findBuildPosition(Vector3 center, float radius, const std::string& blueprintId);
};

#endif