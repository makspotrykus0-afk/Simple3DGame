#ifndef COLONY_H
#define COLONY_H

#include <vector>
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
#include "../systems/SkillsComponent.h"
#include "../core/GameEntity.h"
#include "../components/PositionComponent.h"
#include "../components/StatsComponent.h"

// Forward declarations
class BuildTask;
class ColonyAI; 
class Terrain;
class BuildingInstance;
class Settler; // Forward declaration of Settler (defined in Settler.h)

#include "Settler.h" // Include the full definition of Settler

// Structure representing an item dropped in the world
struct WorldItem {
    Vector3 position;
    std::unique_ptr<Item> item;
    float dropTime;
    bool pendingRemoval = false;
    int amount = 1;

    WorldItem(Vector3 pos, std::unique_ptr<Item> it, float time, bool pending, int amt)
        : position(pos), item(std::move(it)), dropTime(time), pendingRemoval(pending), amount(amt) {}
};

// Structure representing a bush
struct Bush {
    Vector3 position;
    bool hasFruit;
    float regrowthTimer;
    float maxRegrowthTime;

    Bush(Vector3 pos) : position(pos), hasFruit(true), regrowthTimer(0.0f), maxRegrowthTime(30.0f) {}
};

class Colony {
private:
    std::vector<Settler*> settlers;
    std::vector<std::unique_ptr<Animal>> m_animals;
    std::vector<std::unique_ptr<Projectile>> m_projectiles;
    std::vector<Bush*> bushes;
    std::vector<WorldItem> m_droppedItemsStorage;
    std::vector<WorldItem>* currentDroppedItems; 
    std::unique_ptr<ColonyAI> m_ai; 

public:
    Colony();
    ~Colony();

    void initialize();
    void addSettler(Vector3 position, std::string name = "Settler", SettlerProfession profession = SettlerProfession::BUILDER);
    void addAnimal(Vector3 position, AnimalType type);
    void addProjectile(std::unique_ptr<Projectile> projectile);
    void addBush(Vector3 position);
    void addResource(const std::string& resourceName, int amount);
    const std::vector<Settler*>& getSettlers() const { return settlers; }
    const std::vector<std::unique_ptr<Animal>>& getAnimals() const { return m_animals; }
    const std::vector<Bush*>& getBushes() const { return bushes; }
    
    void render(bool fpsMode = false, Settler* fpsSettler = nullptr);
    void cleanup();
    
    // FIX: Changed buildings parameter to match std::vector<BuildingInstance*> used in main/BuildingSystem
    // BuildingSystem stores raw pointers now (vector<BuildingInstance*>) based on main.cpp and other system files
    void update(float deltaTime, const std::vector<std::unique_ptr<Tree>>& trees, std::vector<WorldItem>& droppedItems, const std::vector<BuildingInstance*>& buildings); 

    const std::vector<WorldItem>* getDroppedItems() const { return currentDroppedItems; }
    
    void addDroppedItem(std::unique_ptr<Item> item, Vector3 position);

    void registerHouse(Vector3 pos, int size, bool occupied);

    Settler* getSettlerAt(Vector3 position, float radius = 2.0f);
    void clearSelection();
    std::vector<Settler*> getSelectedSettlers() const;
    bool checkHit(Ray ray, float range, std::string& hitInfo);
};

#endif