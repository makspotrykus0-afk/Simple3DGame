#pragma once

#ifndef GAMESYSTEM_H
#define GAMESYSTEM_H

#include <string>
#include "IGameSystem.h"

// Forward declaration of NavigationGrid
class NavigationGrid;
class Colony; // Forward declaration
class Terrain; // Forward declaration

class GameSystem : public IGameSystem {
public:
    GameSystem(const std::string& name) : m_name(name) {}
    virtual ~GameSystem() = default;

    // Implement IGameSystem methods
    std::string getName() const override { return m_name; }
    int getPriority() const override { return 0; } // Default priority
    
    // Inicjalizacja systemu - domyślna pusta implementacja
    void initialize() override {}
    
    // Zamykanie systemu - domyślna pusta implementacja
    void shutdown() override {}

    // Aktualizacja systemu
    virtual void update(float deltaTime) override = 0;

    // Renderowanie systemu
    virtual void render() override = 0;

    // Static Accessor for Navigation Grid
    static NavigationGrid* getNavigationGrid() { return s_navigationGrid; }
    static void setNavigationGrid(NavigationGrid* grid) { s_navigationGrid = grid; }

    // Static Accessor for Colony
    static Colony* getColony() { return s_colony; }
    static void setColony(Colony* colony) { s_colony = colony; }

    // Static Accessor for Terrain
    static Terrain* getTerrain() { return s_terrain; }
    static void setTerrain(Terrain* terrain) { s_terrain = terrain; }

protected:
    std::string m_name;
    
private:
    static NavigationGrid* s_navigationGrid;
    static Colony* s_colony;
    static Terrain* s_terrain;
};

#endif // GAMESYSTEM_H