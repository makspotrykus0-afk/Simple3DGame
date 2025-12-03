#include "ColonyAI.h"
#include <map>
#include <iostream>
#include <algorithm>
#include "../systems/BuildingSystem.h" 
#include "Colony.h"
#include "Settler.h" // Needed for TaskType
#include "Tree.h" // Needed for Tree
#include "Terrain.h" // Needed for Terrain access

ColonyAI::ColonyAI(Colony* colony, BuildingSystem* buildingSystem)
    : m_colony(colony), m_buildingSystem(buildingSystem), m_timer(0.0f), m_updateInterval(5.0f), m_needsHousing(false)
{
}

ColonyAI::~ColonyAI()
{
}

void ColonyAI::update(float deltaTime)
{
    m_timer += deltaTime;
    if (m_timer >= m_updateInterval) {
        m_timer = 0.0f;
        checkNeeds();
        processConstructionRequests();
        assignJobs(); // New job assignment phase
    }
}

void ColonyAI::registerHouse(Vector3 pos, int size, bool occupied)
{
    float currentTime = (float)GetTime();
    m_knownHouses.push_back({pos, size, occupied, currentTime});
}

void ColonyAI::checkNeeds()
{
    if (!m_colony || !m_buildingSystem) return;

    float currentTime = (float)GetTime();

    // REFRESH KNOWN HOUSES FROM ACTUAL WORLD STATE
    m_knownHouses.clear();
    const auto& buildings = m_buildingSystem->getAllBuildings();
    
    for (const auto& b : buildings) {
        std::string id = b->getBlueprintId();
        // Check if residential
        bool isHouse = (id.find("house") != std::string::npos);
        if (isHouse) {
            int size = 4; // Default (house_4)
            if (id == "house_4") size = 4;
            else if (id == "house_6") size = 6;
            else if (id == "house_9") size = 9;
            // Try parsing dynamic size if still using old format or generic dynamic
            else if (id.find("house_") == 0) {
                try {
                    size = std::stoi(id.substr(6));
                } catch(...) {}
            }
            
            // Check occupancy via settler data later, for now add as available
            m_knownHouses.push_back({b->getPosition(), size, false, currentTime});
        }
    }

    // 3. Assign houses to settlers
    auto settlers = m_colony->getSettlers();
    m_needsHousing = false;

    // Phase 1: Stickiness - Keep previous houses if possible
    for (auto* settler : settlers) {
        (void)settler; // Mark as used to silence unused variable warning
    }

    // Phase 2: Find new homes for homeless
    for (auto* settler : settlers) {
        if (settler->hasHouse) continue; 
        
        // SKIP IF SLEEPING or TIRED - don't interrupt rest with assignment logic that might trigger moves
        if (settler->GetState() == SettlerState::SLEEPING || settler->actionState == "Going to Sleep") {
            continue;
        }

        int bestIdx = -1;
        int minDiff = 999;

        for (size_t i = 0; i < m_knownHouses.size(); ++i) {
            if (m_knownHouses[i].occupied) continue;
            
            // Cast to int to fix warning
            if (m_knownHouses[i].size >= (int)settler->preferredHouseSize) {
                int diff = m_knownHouses[i].size - (int)settler->preferredHouseSize;
                if (diff < minDiff) {
                    minDiff = diff;
                    bestIdx = (int)i;
                }
            }
        }

        if (bestIdx != -1) {
            m_knownHouses[bestIdx].occupied = true;
            // settler->hasHouse = true; // Cannot assign
            // settler->myHousePos = m_knownHouses[bestIdx].position; // Cannot assign
        } else {
            // Need to build!
            m_needsHousing = true;
            
            // Map preferences to valid blueprints (4, 6, 9)
            int size = (int)settler->preferredHouseSize;
            std::string bpId;
            if (size <= 4) { size = 4; bpId = "house_4"; }
            else if (size <= 6) { size = 6; bpId = "house_6"; }
            else { size = 9; bpId = "house_9"; }
            
            // Check pending builds BEFORE ordering new one
            if (m_buildingSystem->getPendingBuildCount(bpId) > 0) {
                continue; // Already building one of this type, wait
            }

            Vector3 center = settler->getPosition();
            Vector3 buildPos = findBuildPosition(center, 40.0f, bpId);

            if (buildPos.y > -500.0f) {
                BuildTask* task = m_buildingSystem->startBuilding(bpId, buildPos, nullptr, 0.0f);
                
                if (task) {
                    // Don't add to knownHouses yet, wait until built
                } else {
                    std::cerr << "ColonyAI: Failed to start building " << bpId << std::endl;
                }
            }
        }
    }

    int housedCount = 0;
    for (auto* settler : settlers) {
        if (settler->hasHouse) housedCount++; 
    }
}

void ColonyAI::processConstructionRequests()
{
    // Logic moved to checkNeeds
}

void ColonyAI::assignJobs() {
    auto settlers = m_colony->getSettlers();
    
    // Only assign jobs to idle settlers
    for (auto* settler : settlers) {
        if (settler->GetState() != SettlerState::IDLE) continue;
        
        // Check profession or default to gathering if nothing else
        // For now, treat everyone without a specific task as potential gatherer if needed
        // But ideally we respect professions.
        
        // Simplification: If idle and not sleeping, find a tree to chop
        if (settler->GetState() == SettlerState::IDLE) {
            // Find nearest unreserved tree
            Tree* bestTree = nullptr;
            float minDistSq = 99999.0f;
            
            // Need access to trees. They are in Terrain or managed by Colony/GameSystem
            // Colony has update method receiving trees, but AI doesn't store them.
            // We need to access global or passed in trees.
            // Since we don't have easy access here without changing architecture, 
            // let's use GameSystem singleton if available, or assume Colony can provide them.
            
            // HACK: Using GameSystem or Terrain directly would be better, but let's try to find a way.
            // Terrain is likely available via GameSystem
            Terrain* terrain = GameSystem::getTerrain();
            if (terrain) {
                const auto& trees = terrain->getTrees();
                Vector3 settlerPos = settler->getPosition();
                
                for (const auto& treePtr : trees) {
                    Tree* tree = treePtr.get();
                    if (!tree->isActive() || tree->isStump() || tree->isReserved()) continue;
                    
                    float distSq = Vector3DistanceSqr(settlerPos, tree->getPosition());
                    if (distSq < minDistSq) {
                        minDistSq = distSq;
                        bestTree = tree;
                    }
                }
                
                if (bestTree) {
                    // Assign task
                    settler->assignTask(TaskType::GATHER, bestTree);
                    std::cout << "ColonyAI: Assigned gathering task to " << settler->getName() << std::endl;
                }
            }
        }
    }
}

Vector3 ColonyAI::findBuildPosition(Vector3 center, float radius, const std::string& blueprintId)
{
    (void)radius;
    float step = 15.0f; 
    int maxSteps = 20;
    
    float x = 0;
    float z = 0;
    float dx = 0;
    float dz = -1;
    float t = step;
    
    for (int i = 0; i < maxSteps * maxSteps; ++i) {
        if (-maxSteps/2 < x && x <= maxSteps/2 && -maxSteps/2 < z && z <= maxSteps/2) {
             Vector3 testPos = { center.x + x * step, 0.0f, center.z + z * step };
             
             if (m_buildingSystem->canBuild(blueprintId, testPos)) {
                 return testPos;
             }
        }
        
        if (x == z || (x < 0 && x == -z) || (x > 0 && x == 1-z)) {
            t = dx;
            dx = -dz;
            dz = t;
        }
        x += dx;
        z += dz;
    }
    
    return { 0.0f, -1000.0f, 0.0f }; 
}