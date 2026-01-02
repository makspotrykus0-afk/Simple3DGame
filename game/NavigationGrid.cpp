#include "NavigationGrid.h"
#include "../systems/BuildingSystem.h"
#include "Tree.h"
#include "ResourceNode.h"
#include "Door.h"
#include "BuildingBlueprint.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <vector>
#include <iostream> // Added for debug prints

NavigationGrid::NavigationGrid(int width, int height, float tileSize)
    : m_width(width), m_height(height), m_tileSize(tileSize), m_currentSearchId(0) {
    
    m_nodes.resize(width * height);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            GridNode& node = m_nodes[y * width + x];
            node.x = x;
            node.y = y;
            node.isWalkable = true;
            node.gCost = 0;
            node.hCost = 0;
            node.parent = nullptr;
            node.searchId = 0;
        }
    }
}

NavigationGrid::GridCoords NavigationGrid::WorldToGridCoords(Vector3 worldPos) const {
    // Offset współrzędnych o połowę rozmiaru mapy, aby (0,0) było na środku mapy
    // Zakładamy, że mapa jest wycentrowana w (0,0) świata
    float mapWidth = m_width * m_tileSize;
    float mapHeight = m_height * m_tileSize;
    
    float adjustedX = worldPos.x + (mapWidth / 2.0f);
    float adjustedZ = worldPos.z + (mapHeight / 2.0f);

    int x = static_cast<int>(adjustedX / m_tileSize);
    int y = static_cast<int>(adjustedZ / m_tileSize);
    
    // Clamp to bounds to avoid out of range access
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= m_width) x = m_width - 1;
    if (y >= m_height) y = m_height - 1;

    return {x, y};
}

Vector3 NavigationGrid::GridToWorldCoords(int x, int y) const {
    float mapWidth = m_width * m_tileSize;
    float mapHeight = m_height * m_tileSize;

    return Vector3{
        (x * m_tileSize + m_tileSize * 0.5f) - (mapWidth / 2.0f),
        0.0f,
        (y * m_tileSize + m_tileSize * 0.5f) - (mapHeight / 2.0f)
    };
}

void NavigationGrid::SetWalkable(int x, int y, bool walkable) {
    if (IsWithinBounds(x, y)) {
        m_nodes[y * m_width + x].isWalkable = walkable;
    }
}

bool NavigationGrid::IsWalkable(int x, int y) const {
    if (!IsWithinBounds(x, y)) return false;
    return m_nodes[y * m_width + x].isWalkable;
}

bool NavigationGrid::IsWithinBounds(int x, int y) const {
    return x >= 0 && x < m_width && y >= 0 && y < m_height;
}

void NavigationGrid::UpdateGrid(const std::vector<BuildingInstance*>& buildings, 
               const std::vector<Tree*>& trees,
               const std::vector<std::unique_ptr<ResourceNode>>& resources) {
    
    // Reset walkability
    for (auto& node : m_nodes) {
        node.isWalkable = true;
    }

    // Buildings
    for (const auto* building : buildings) {
        if (!building) continue;
        
        // Ignore floor and simple_storage - they are walkable
        if (building->getBlueprintId() == "floor" || building->getBlueprintId() == "simple_storage") continue;

        const BuildingBlueprint* bp = building->getBlueprint();

        // Handle composite buildings (like houses) specifically
        if (bp && !bp->getComponents().empty()) {
            Vector3 bPos = building->getPosition();
            float bRot = building->getRotation();

            for (const auto& comp : bp->getComponents()) {
                // Only block walls
                if (comp.blueprintId == "wall") {
                    // Calculate world position of the wall component
                    Vector3 rotatedOffset = Vector3RotateByAxisAngle(comp.localPosition, { 0.0f, 1.0f, 0.0f }, bRot * DEG2RAD);
                    Vector3 compPos = Vector3Add(bPos, rotatedOffset);
                    float compRot = bRot + comp.localRotation;

                    // Ulepszona logika blokowania ścian oparta na OBB (Oriented Bounding Box)
                    // Wall dimensions: 2.0 (width/X) x 0.5 (thickness/Z)
                    float hw = 1.0f; // Half Width
                    float ht = 0.25f; // Half Thickness

                    // Oblicz AABB dla wstępnej selekcji kratek
                    Vector3 corners[4] = {
                        { -hw, 0, -ht },
                        { hw, 0, -ht },
                        { hw, 0, ht },
                        { -hw, 0, ht }
                    };
                    
                    float minX = std::numeric_limits<float>::max();
                    float minZ = std::numeric_limits<float>::max();
                    float maxX = std::numeric_limits<float>::lowest();
                    float maxZ = std::numeric_limits<float>::lowest();
                    
                    Matrix rotMat = MatrixRotateY(compRot * DEG2RAD);
                    
                    for(auto& p : corners) {
                        Vector3 rotP = Vector3Transform(p, rotMat);
                        Vector3 worldP = Vector3Add(compPos, rotP);
                        if (worldP.x < minX) minX = worldP.x;
                        if (worldP.z < minZ) minZ = worldP.z;
                        if (worldP.x > maxX) maxX = worldP.x;
                        if (worldP.z > maxZ) maxZ = worldP.z;
                    }
                    
                    // Margines bezpieczeństwa dla AABB
                    float margin = 0.1f;
                    minX -= margin; minZ -= margin;
                    maxX += margin; maxZ += margin;
                    
                    GridCoords minGC = WorldToGridCoords({minX, 0, minZ});
                    GridCoords maxGC = WorldToGridCoords({maxX, 0, maxZ});
                    
                    // Iteruj tylko po kratkach wewnątrz AABB
                    for (int y = minGC.y; y <= maxGC.y; ++y) {
                        for (int x = minGC.x; x <= maxGC.x; ++x) {
                             // Sprawdź dokładnie czy środek kratki jest wewnątrz OBB ściany
                             Vector3 cellWorldPos = GridToWorldCoords(x, y);
                             
                             // Transformacja punktu do lokalnego układu ściany
                             // 1. Przesunięcie względem środka ściany
                             Vector3 localP = Vector3Subtract(cellWorldPos, compPos);
                             
                             // 2. Obrót odwrotny do obrotu ściany (czyli -compRot)
                             // MatrixRotateY bierze kąt w radianach.
                             // Obrót punktu o kąt -alfa to to samo co obrót układu o alfa.
                             Vector3 rotatedLocalP = Vector3Transform(localP, MatrixRotateY(-compRot * DEG2RAD));
                             
                             // Sprawdź czy punkt w lokalnym układzie mieści się w wymiarach ściany (+ margines kratki)
                             // ZMNIEJSZONY MARGINES KOLIZJI - aby nie blokować kratek "na styk" przy drzwiach
                             float collisionMarginX = m_tileSize * 0.25f; 
                             float collisionMarginZ = m_tileSize * 0.25f;
                             
                             if (std::abs(rotatedLocalP.x) <= (hw + collisionMarginX) && 
                                 std::abs(rotatedLocalP.z) <= (ht + collisionMarginZ)) {
                                 SetWalkable(x, y, false);
                             }
                        }
                    }
                }
                // Ignore 'bed', 'floor', 'door' (doors are handled separately or walkable)
            }
            
            // Ensure doors are walkable (explicitly unlock them if they were accidentally blocked by wall logic overlap)
            if (building->getDoor()) {
                Vector3 doorPos = building->getDoor()->getPosition();
                GridCoords doorCoords = WorldToGridCoords(doorPos);
                SetWalkable(doorCoords.x, doorCoords.y, true);
            }

        } else {
            // Legacy/Simple handling for single-block structures (or if blueprint missing)
            
            // Pobierz BoundingBox budynku
            BoundingBox bbox = building->getBoundingBox();
            
            // Lekko zmniejszamy bboxa, żeby nie łapać sąsiednich kratek "na styk"
            bbox.min.x += 0.1f; bbox.min.z += 0.1f;
            bbox.max.x -= 0.1f; bbox.max.z -= 0.1f;

            GridCoords min = WorldToGridCoords(bbox.min);
            GridCoords max = WorldToGridCoords(bbox.max);
            
            // Zablokuj obszar zajmowany przez budynek
            for (int y = min.y; y <= max.y; ++y) {
                for (int x = min.x; x <= max.x; ++x) {
                    SetWalkable(x, y, false);
                }
            }

            // Obsługa drzwi - jeśli są, odblokuj ich pole
            // Check building type if it is a door or has door component
            if (building->getBlueprintId() == "door" || building->getDoor()) {
                // Drzwi są zawsze przechodnie dla Pathfindingu (osadnik je otworzy)
                if (building->getBlueprintId() == "door") {
                     Vector3 doorPos = building->getPosition();
                     GridCoords doorCoords = WorldToGridCoords(doorPos);
                     SetWalkable(doorCoords.x, doorCoords.y, true);
                } else if (building->getDoor()) {
                     Vector3 doorPos = building->getDoor()->getPosition();
                     GridCoords doorCoords = WorldToGridCoords(doorPos);
                     SetWalkable(doorCoords.x, doorCoords.y, true);
                }
            }
        }
    }

    // Trees
    float treeRadius = 0.5f;
    float combinedRadius = treeRadius * 1.6f; // Zwiekszony promien (0.8m) zeby pathfinding omijal szeroko

    for (const auto* tree : trees) {
        if (!tree || !tree->isActive() || tree->isStump()) continue;
        
        Vector3 pos = tree->getPosition();
        
        // Block area around tree based on radius
        GridCoords min = WorldToGridCoords(Vector3{pos.x - combinedRadius, 0, pos.z - combinedRadius});
        GridCoords max = WorldToGridCoords(Vector3{pos.x + combinedRadius, 0, pos.z + combinedRadius});

        for (int y = min.y; y <= max.y; ++y) {
            for (int x = min.x; x <= max.x; ++x) {
                SetWalkable(x, y, false);
            }
        }
    }

    // Resources
    // Assume small radius for resources or similar to trees
    float resourceRadius = 0.5f; 
    float combinedResRadius = resourceRadius * 0.8f; // Zmniejszony promień

    for (const auto& resource : resources) {
        if (!resource || !resource->isActive() || resource->isDepleted()) continue;
        
        Vector3 pos = resource->getPosition();
        
        GridCoords min = WorldToGridCoords(Vector3{pos.x - combinedResRadius, 0, pos.z - combinedResRadius});
        GridCoords max = WorldToGridCoords(Vector3{pos.x + combinedResRadius, 0, pos.z + combinedResRadius});

        for (int y = min.y; y <= max.y; ++y) {
            for (int x = min.x; x <= max.x; ++x) {
                SetWalkable(x, y, false);
            }
        }
    }
}

float NavigationGrid::GetDistance(GridNode* nodeA, GridNode* nodeB) const {
    int dstX = std::abs(nodeA->x - nodeB->x);
    int dstY = std::abs(nodeA->y - nodeB->y);
    
    // Octile distance
    if (dstX > dstY)
        return 14.0f * dstY + 10.0f * (dstX - dstY);
    return 14.0f * dstX + 10.0f * (dstY - dstX);
}

std::vector<GridNode*> NavigationGrid::GetNeighbors(GridNode* node) {
    std::vector<GridNode*> neighbors;
    neighbors.reserve(8);
    
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            if (x == 0 && y == 0) continue;
            
            int checkX = node->x + x;
            int checkY = node->y + y;
            
            if (IsWithinBounds(checkX, checkY)) {
                GridNode* neighbor = &m_nodes[checkY * m_width + checkX];
                
                // Prevent corner cutting for diagonal movement
                if (x != 0 && y != 0) {
                    // Check orthogonal neighbors
                    bool w1 = IsWalkable(node->x + x, node->y);
                    bool w2 = IsWalkable(node->x, node->y + y);
                    
                    // If either orthogonal is blocked, diagonal is blocked
                    if (!w1 || !w2) continue;
                }
                
                neighbors.push_back(neighbor);
            }
        }
    }
    
    return neighbors;
}

std::vector<Vector3> NavigationGrid::FindPath(Vector3 startWorld, Vector3 endWorld) {
    GridCoords startCoords = WorldToGridCoords(startWorld);
    GridCoords endCoords = WorldToGridCoords(endWorld);
    
    if (!IsWithinBounds(startCoords.x, startCoords.y) || !IsWithinBounds(endCoords.x, endCoords.y)) {
        // std::cout << "DEBUG: Start or End out of bounds!" << std::endl;
        return {}; // Poza mapą
    }

    GridNode* startNode = &m_nodes[startCoords.y * m_width + startCoords.x];
    GridNode* endNode = &m_nodes[endCoords.y * m_width + endCoords.x];
    
    // Jeśli cel jest niedostępny, spróbuj znaleźć najbliższy dostępny węzeł wokół celu
    if (!endNode->isWalkable) {
        // Proste przeszukanie sąsiadów celu
        bool foundAlternative = false;
        std::vector<GridNode*> neighbors = GetNeighbors(endNode);
        // Sortuj po dystansie do startu, żeby nie iść na około
        std::sort(neighbors.begin(), neighbors.end(), [this, startNode](GridNode* a, GridNode* b){
            return GetDistance(a, startNode) < GetDistance(b, startNode);
        });

        for(auto* neighbor : neighbors) {
            if(neighbor->isWalkable) {
                endNode = neighbor;
                foundAlternative = true;
                break;
            }
        }
        if (!foundAlternative) return {}; // Nie znaleziono wejścia
    }

    // Zwiększ ID wyszukiwania aby zresetować stan węzłów leniwie
    m_currentSearchId++;

    // Lambda do porównywania wskaźników w priority_queue (najmniejszy fCost na górze)
    auto compareNodes = [](GridNode* a, GridNode* b) {
        if (a->fCost() == b->fCost())
            return a->hCost > b->hCost;
        return a->fCost() > b->fCost();
    };

    // Używamy priority_queue dla OpenSet dla wydajności O(log n)
    std::priority_queue<GridNode*, std::vector<GridNode*>, decltype(compareNodes)> openSet(compareNodes);
    
    // Inicjalizacja startNode
    startNode->gCost = 0;
    startNode->hCost = GetDistance(startNode, endNode);
    startNode->parent = nullptr;
    startNode->searchId = m_currentSearchId;
    
    openSet.push(startNode);
    
    // Użyj std::vector<bool> dla szybkiego sprawdzania obecności w OpenSet i ClosedSet
    // Rozmiar musi być równy liczbie węzłów
    size_t gridSize = m_nodes.size();
    std::vector<bool> inClosedSet(gridSize, false);
    std::vector<bool> inOpenSet(gridSize, false);

    inOpenSet[startNode->y * m_width + startNode->x] = true;

    while (!openSet.empty()) {
        GridNode* currentNode = openSet.top();
        openSet.pop();
        
        int currentIndex = currentNode->y * m_width + currentNode->x;
        inOpenSet[currentIndex] = false; // Wyjęty z Open
        inClosedSet[currentIndex] = true; // Włożony do Closed

        if (currentNode == endNode) {
            std::vector<Vector3> path;
            GridNode* curr = endNode;
            while (curr != startNode) {
                path.push_back(GridToWorldCoords(curr->x, curr->y));
                curr = curr->parent;
            }
            // Nie dodajemy startNode do ścieżki (lub dodajemy - zależy od konwencji, tu bez startu)
            std::reverse(path.begin(), path.end());
            return path;
        }

        for (GridNode* neighbor : GetNeighbors(currentNode)) {
            int neighborIndex = neighbor->y * m_width + neighbor->x;
            
            if (!neighbor->isWalkable || inClosedSet[neighborIndex]) {
                continue;
            }

            float newMovementCostToNeighbor = currentNode->gCost + GetDistance(currentNode, neighbor);
            
            // Jeśli węzeł jest "stary" (z poprzedniego szukania), traktuj go jak nieodwiedzony
            bool isFresh = (neighbor->searchId != m_currentSearchId);
            
            if (isFresh || newMovementCostToNeighbor < neighbor->gCost) {
                neighbor->gCost = newMovementCostToNeighbor;
                neighbor->hCost = GetDistance(neighbor, endNode);
                neighbor->parent = currentNode;
                neighbor->searchId = m_currentSearchId; // Oznacz jako aktualny
                
                if (!inOpenSet[neighborIndex]) {
                    openSet.push(neighbor);
                    inOpenSet[neighborIndex] = true;
                }
            }
        }
    }
    
    return {}; // Brak ścieżki
}