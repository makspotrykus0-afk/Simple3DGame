#pragma once

#include "raylib.h" // Assuming raylib.h provides necessary types like Mesh, Model, Vector3, Color
#include <vector>   // Dołączamy nagłówek dla std::vector

// Forward declarations to avoid incomplete type issues if Mesh/Model are complex
// If raylib headers define these correctly, these might be simplified or removed.
struct Mesh;
struct Model;

class Terrain {
public:
    Terrain();
    ~Terrain();

    void generate(int newWidth, int newHeight, float newTileSize);
    float getHeightAt(int x, int y) const;
    void setHeightAt(int x, int y, float height);
    bool isAccessible(Vector3 position) const;
    void render();
    void cleanup();

private:
    int width;
    int height;
    float tileSize;
    std::vector<float> heightMap;

    // Używamy wskaźników, aby uniknąć problemów z niekompletnymi typami, jeśli
    // definicje Mesh i Model są złożone lub zależą od kontekstu kompilacji.
    // Zmienimy to na bezpośrednie typy, gdy tylko ustalimy poprawne zależności.
    Mesh* mesh; 
    Model* model;
};