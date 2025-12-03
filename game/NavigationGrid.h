#pragma once

#include <vector>
#include <raylib.h>
#include <raymath.h>
#include <memory>

// Forward declarations
class BuildingInstance;
class Tree;
class ResourceNode;

struct GridNode {
    int x, y;
    bool isWalkable;
    
    // Dane dla algorytmu A*
    float gCost;
    float hCost;
    GridNode* parent;
    
    // Optymalizacja: identyfikator wyszukiwania do unikania resetu całej tablicy
    int searchId;
    
    float fCost() const { return gCost + hCost; }
};

class NavigationGrid {
public:
    NavigationGrid(int width, int height, float tileSize);
    ~NavigationGrid() = default;

    // Konwersja współrzędnych
    struct GridCoords { int x, y; };
    GridCoords WorldToGridCoords(Vector3 worldPos) const;
    Vector3 GridToWorldCoords(int x, int y) const;

    // Zarządzanie stanem
    void SetWalkable(int x, int y, bool walkable);
    bool IsWalkable(int x, int y) const;
    bool IsWithinBounds(int x, int y) const;

    // Główna funkcja aktualizacji siatki
    // Przyjmuje kontenery wskaźników (surowe lub smart pointery w zależności od definicji w systemie)
    void UpdateGrid(const std::vector<BuildingInstance*>& buildings, 
                   const std::vector<Tree*>& trees,
                   const std::vector<std::unique_ptr<ResourceNode>>& resources);

    // Algorytm A*
    // Zwraca listę punktów w świecie
    std::vector<Vector3> FindPath(Vector3 startWorld, Vector3 endWorld);

private:
    int m_width;
    int m_height;
    float m_tileSize;
    
    // Licznik dla leniwego resetowania węzłów
    int m_currentSearchId;
    
    // Siatka węzłów [y * width + x]
    std::vector<GridNode> m_nodes;
    
    float GetDistance(GridNode* nodeA, GridNode* nodeB) const;
    std::vector<GridNode*> GetNeighbors(GridNode* node);
};