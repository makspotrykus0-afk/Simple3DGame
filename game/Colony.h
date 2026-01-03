#ifndef COLONY_H
#define COLONY_H

#include <vector>
#include <unordered_map>
#include "raylib.h"
#include "raymath.h"
#include <string>
#include <iostream>
#include <memory>
#include <cmath>
#include <algorithm>
#include "Item.h"
#include "Tree.h"
#include "Animal.h"
#include "Bed.h"
#include "Projectile.h"
#include "ResourceNode.h"
#include "../components/SkillsComponent.h"
#include "../core/GameEntity.h"
#include "../components/PositionComponent.h"
#include "../components/StatsComponent.h"

// Forward declarations
// Forward declarations
class BuildTask;
class Terrain;
class BuildingInstance;
class Door;
class Settler; // Forward declaration of Settler (defined in Settler.h)
#include "Settler.h" // Include the full definition of Settler
#include "ColonyAI.h" // Include full definition for unique_ptr


// Structure representing an item dropped in the world
class WorldItem {
public:
    Vector3 position;
    std::unique_ptr<Item> item;
    float dropTime;
    bool pendingRemoval = false;
    int amount = 1;

    WorldItem(Vector3 pos, std::unique_ptr<Item> it, float time, bool pending, int amt)
        : position(pos), item(std::move(it)), dropTime(time), pendingRemoval(pending), amount(amt) {}
};

// Structure representing a bush
class Bush {
public:
    Vector3 position;
    bool hasFruit;
    float regrowthTimer;
    float maxRegrowthTime;
    Bush(Vector3 pos) : position(pos), hasFruit(true), regrowthTimer(0.0f), maxRegrowthTime(30.0f) {}
};
class Colony {
private:
std::vector<Settler*> settlers;
std::vector<std::unique_ptr<ResourceNode>> m_resourceNodes;
std::vector<std::unique_ptr<Animal>> m_animals;
std::vector<std::unique_ptr<Projectile>> m_projectiles;
std::vector<Bush*> bushes;
std::vector<WorldItem> m_droppedItemsStorage;
std::unique_ptr<ColonyAI> m_ai;
std::vector<BuildingInstance*> m_storageBuildings;
std::unordered_map<std::string, int> m_activeGatheringTasks; // resource type -> count
    std::vector<Door*> m_doors; // Registered doors

// Tree Respawn Logic
float m_treeRespawnTimer = 0.0f;
const int MAX_TREES = 50;

// Helper to find valid spawn position
Vector3 FindValidTreeSpawnPos();


public:
public:
Colony();
~Colony();
void initialize();
void addSettler(Vector3 position, std::string name = "Settler", SettlerProfession profession = SettlerProfession::BUILDER);
void addAnimal(Vector3 position, AnimalType type);
void addProjectile(std::unique_ptr<Projectile> projectile);
void addBush(Vector3 position);
void addResource(const std::string& resourceName, int amount);
void addResourceNode(std::unique_ptr<ResourceNode> node) { m_resourceNodes.push_back(std::move(node)); }
const std::vector<std::unique_ptr<ResourceNode>>& getResourceNodes() const { return m_resourceNodes; }

const std::vector<Settler*>& getSettlers() const { return settlers; }
const std::vector<std::unique_ptr<Animal>>& getAnimals() const { return m_animals; }
const std::vector<Bush*>& getBushes() const { return bushes; }

void render(bool fpsMode = false, Settler* fpsSettler = nullptr);
void cleanup();

// FIX: Changed buildings parameter to match std::vector<BuildingInstance*> used in main/BuildingSystem
// BuildingSystem stores raw pointers now (vector<BuildingInstance*>) based on main.cpp and other system files
void update(float deltaTime, const std::vector<std::unique_ptr<Tree>>& trees, const std::vector<BuildingInstance*>& buildings);

const std::vector<WorldItem>& getDroppedItems() const { return m_droppedItemsStorage; }
void addDroppedItem(std::unique_ptr<Item> item, Vector3 position);
std::unique_ptr<Item> takeDroppedItem(size_t index);
void registerHouse(Vector3 pos, int size, bool occupied);

void registerStorageBuilding(BuildingInstance* b);
const std::vector<BuildingInstance*>& getStorageBuildings() const;

void registerDoor(Door* door);
const std::vector<Door*>& getDoors() const;

Settler* getSettlerAt(Vector3 position, float radius = 2.0f);

std::vector<Settler*> getSelectedSettlers() const;
void clearSelection();
bool checkHit(Ray ray, float range, std::string& hitInfo);

// Logistyka Kolonii - Bonusy
float getEfficiencyModifier(Vector3 pos, SettlerState state) const;

bool isGatheringTaskActive(const std::string& resourceType) const;
void registerGatheringTask(const std::string& resourceType);
    void unregisterGatheringTask(const std::string& resourceType);

    // Zasoby - Gettery dla UI (sumują zasoby z magazynów)
    int getWood() const;
    int getStone() const;
    int getFood() const;

};
#endif