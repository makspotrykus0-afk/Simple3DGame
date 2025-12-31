#include "GameSystem.h"
#include "../game/NavigationGrid.h" // Include full definition
#include "../game/Terrain.h" // Include full definition

// Initialize static members
NavigationGrid* GameSystem::s_navigationGrid = nullptr;
Colony* GameSystem::s_colony = nullptr;
Terrain* GameSystem::s_terrain = nullptr;