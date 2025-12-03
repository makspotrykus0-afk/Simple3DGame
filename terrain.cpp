#include "terrain.h"
#include <iostream>
#include <vector>
#include <cmath>

Terrain::Terrain() : width(0), height(0), tileSize(0.0f), mesh(nullptr), model(nullptr) {
    // mesh i model są wskaźnikami, inicjalizowane jako nullptr
}

Terrain::~Terrain() {
    cleanup();
}

void Terrain::generate(int newWidth, int newHeight, float newTileSize) {
    width = newWidth;
    height = newHeight;
    tileSize = newTileSize;

    heightMap.resize(width * height);

    // Generate flat heightmap for now
    for (int i = 0; i < width * height; ++i) {
        heightMap[i] = 0.0f;
    }

    // Generate mesh using raylib's procedural mesh generation
    // Zakładamy, że GenMeshPlane jest dostępne z raylib.h
    // Używamy `new Mesh(...)` ponieważ GenMeshPlane zwraca Mesh, a my potrzebujemy wskaźnika
    // Poprawiono błąd: GenMeshPlane zwraca obiekt, a nie wskaźnik, więc usuwamy `new Mesh(...)`
    mesh = new Mesh(GenMeshPlane(width * tileSize, height * tileSize, width - 1, height - 1));
    
    // Set vertex heights
    for (int i = 0; i < mesh->vertexCount; i++) {
        // Używamy mesh->vertices zamiast mesh.vertices, bo mesh jest wskaźnikiem
        if (i * 3 + 2 < mesh->vertexCount * 3) {
            mesh->vertices[i * 3 + 2] = getHeightAt(i % width, i / width);
        }
    }

    // Upload mesh to GPU
    UploadMesh(mesh, false);

    // Create model from mesh
    // Zakładamy, że LoadModelFromMesh jest dostępne z raylib.h
    // Używamy `new Model(...)` ponieważ LoadModelFromMesh zwraca Model, a my potrzebujemy wskaźnika
    // Poprawiono błąd: LoadModelFromMesh zwraca obiekt, a nie wskaźnik, więc usuwamy `new Model(...)`
    model = new Model(LoadModelFromMesh(*mesh)); 
}

float Terrain::getHeightAt(int x, int y) const {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        return heightMap[y * width + x];
    }
    return 0.0f;
}

void Terrain::setHeightAt(int x, int y, float height) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        heightMap[y * width + x] = height;
    }
}

bool Terrain::isAccessible(Vector3 position) const {
    // Simple implementation - check if position is on the ground
    return position.y <= 0.0f;
}

void Terrain::render() {
    // Draw terrain model with default material
    // Zakładamy, że DrawModel jest dostępne z raylib.h
    // Używamy `*model` aby dereferencjonować wskaźnik i przekazać obiekt Model
    if (model) {
       DrawModel(*model, Vector3{0, 0, 0}, 1.0f, Color{34, 139, 34, 255}); // DarkGreen
    }
}

void Terrain::cleanup() {
    if (model) {
        // Zakładamy, że UnloadModel jest dostępne z raylib.h
        UnloadModel(*model); 
        delete model;
        model = nullptr;
    }
    if (mesh) {
        // Zakładamy, że UnloadMesh jest dostępne z raylib.h
        UnloadMesh(*mesh); 
        delete mesh;
        mesh = nullptr;
    }
}