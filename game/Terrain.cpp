#include "Terrain.h"
#include "Tree.h"
#include "ResourceNode.h"
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include "../core/GameEngine.h"
#include "../systems/InteractionSystem.h"
#include "raymath.h"

Terrain::Terrain() : width(0), height(0), tileSize(0.0f) {
    mesh = {};
    model = {};
}

Terrain::~Terrain() {
    cleanup();
}

void Terrain::generate(int newWidth, int newHeight, float newTileSize) {
    width = newWidth;
    height = newHeight;
    tileSize = newTileSize;

    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    heightMap.resize(width * height);

    m_trees.clear();
    m_resourceNodes.clear();

    // User feedback: "The terrain is flat, everything should be at the same height".
    // Disabling Perlin noise to ensure flat terrain (Y=0).
    // This resolves the issue where visual mesh and logical heightmap were desynchronized
    // (likely due to normals not being updated or visual mesh remaining flat while logic assumed hills).
    
    /*
    Image perlinImage = GenImagePerlinNoise(width, height, 0, 0, 5.0f);
    Color* pixels = LoadImageColors(perlinImage);
    */

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Flatten the terrain
            float heightVal = 0.0f; 
            // if (pixels) heightVal = (pixels[y * width + x].r / 255.0f) * 10.0f; // Disabled
            setHeightAt(x, y, heightVal);
        }
    }

    /*
    UnloadImageColors(pixels);
    UnloadImage(perlinImage);
    */

    float mapWidth = (width - 1) * tileSize;
    float mapHeight = (height - 1) * tileSize;

    std::cout << "Generating Flat Terrain: " << width << "x" << height << " TileSize: " << tileSize << std::endl;

    // GenMeshPlane creates a flat mesh at Y=0. Since we want flat terrain, we don't need to modify vertices.
    mesh = GenMeshPlane(mapWidth, mapHeight, width - 1, height - 1);
    
    // We skip vertex modification to ensure visual mesh is perfectly flat 0.0f
    /*
    for (int i = 0; i < mesh.vertexCount; i++) {
        // ... modification loop ...
    }
    */
    
    model = LoadModelFromMesh(mesh);

    // Tree Generation
    std::cout << "--- TREE PLACEMENT (FLAT) ---" << std::endl;
    for (int i = 0; i < 50; i++) {
        float r1 = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
        float r2 = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
        
        float x = (r1 * mapWidth) - (mapWidth / 2.0f);
        float z = (r2 * mapHeight) - (mapHeight / 2.0f);
        
        // Height is 0.0f for flat terrain
        float y = 0.0f;
        
        if (i < 5) {
             std::cout << "Tree " << i << " Pos: (" << x << ", " << y << ", " << z << ")" << std::endl;
        }

        auto newTree = std::make_unique<Tree>(PositionComponent({x, y, z}), 100.0f, 50.0f);
        addTree(std::move(newTree));
    }
    std::cout << "----------------------------" << std::endl;

    // Stone Generation
    for (int i = 0; i < 20; i++) {
        float r1 = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
        float r2 = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
        
        float x = (r1 * mapWidth) - (mapWidth / 2.0f);
        float z = (r2 * mapHeight) - (mapHeight / 2.0f);
        
        float y = 0.0f;
        
        auto newStone = std::make_unique<ResourceNode>(Resources::ResourceType::Stone, PositionComponent(Vector3{x, y, z}), 50.0f);
        addResourceNode(std::move(newStone));
    }
}

void Terrain::addTree(std::unique_ptr<Tree> tree) {
    if (auto interactionSystem = GameEngine::getInstance().getSystem<InteractionSystem>()) {
        interactionSystem->registerInteractableObject(tree.get());
    }
    m_trees.push_back(std::move(tree));
}

void Terrain::removeTree(Tree* tree) {
    if (auto interactionSystem = GameEngine::getInstance().getSystem<InteractionSystem>()) {
        interactionSystem->unregisterInteractableObject(tree);
    }

    for (auto it = m_trees.begin(); it != m_trees.end(); ++it) {
        if (it->get() == tree) {
            m_trees.erase(it);
            break;
        }
    }
}

void Terrain::addResourceNode(std::unique_ptr<ResourceNode> node) {
    if (auto interactionSystem = GameEngine::getInstance().getSystem<InteractionSystem>()) {
        interactionSystem->registerInteractableObject(node.get());
    }
    m_resourceNodes.push_back(std::move(node));
}

void Terrain::removeResourceNode(ResourceNode* node) {
    if (auto interactionSystem = GameEngine::getInstance().getSystem<InteractionSystem>()) {
        interactionSystem->unregisterInteractableObject(node);
    }

    for (auto it = m_resourceNodes.begin(); it != m_resourceNodes.end(); ++it) {
        if (it->get() == node) {
            m_resourceNodes.erase(it);
            break;
        }
    }
}

float Terrain::getHeightAt(int x, int y) const {
    // Always 0 for flat terrain
    return 0.0f;
}

float Terrain::getInterpolatedHeightAt(float x, float z) const {
    // Always 0 for flat terrain
    return 0.0f;
}

float Terrain::getRaycastHeightAt(float x, float z) const {
    // Simplified for flat terrain
    return 0.0f;
}

void Terrain::setHeightAt(int x, int y, float newHeight) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        heightMap[y * width + x] = newHeight;
    }
}

bool Terrain::isAccessible(Vector3 position) const {
    return true; 
}

void Terrain::render() {
    DrawModel(model, Vector3{0, 0, 0}, 1.0f, Color{34, 139, 34, 255});
    
    for (const auto& tree : m_trees) {
        tree->render();
    }
    
    for (const auto& node : m_resourceNodes) {
        node->render();
    }
}

void Terrain::update() {
    auto it = m_trees.begin();
    while (it != m_trees.end()) {
        Tree* tree = it->get();
        if (tree->shouldBeRemoved()) {
             if (auto interactionSystem = GameEngine::getInstance().getSystem<InteractionSystem>()) {
                interactionSystem->unregisterInteractableObject(tree);
            }
            it = m_trees.erase(it);
        } 
        else if (!tree->isActive()) {
            if (auto interactionSystem = GameEngine::getInstance().getSystem<InteractionSystem>()) {
                interactionSystem->unregisterInteractableObject(tree);
            }
            ++it;
        } else {
            ++it;
        }
    }
    
    auto resIt = m_resourceNodes.begin();
    while (resIt != m_resourceNodes.end()) {
        ResourceNode* node = resIt->get();
        node->update(0.016f);
        
        if (node->isDepleted()) {
             if (auto interactionSystem = GameEngine::getInstance().getSystem<InteractionSystem>()) {
                interactionSystem->unregisterInteractableObject(node);
            }
            resIt = m_resourceNodes.erase(resIt);
        } else {
            ++resIt;
        }
    }
}

void Terrain::cleanup() {
    if (model.meshCount > 0) {
        UnloadModel(model);
        model = {};
    }
    
    if (auto interactionSystem = GameEngine::getInstance().getSystem<InteractionSystem>()) {
        for (const auto& tree : m_trees) {
            interactionSystem->unregisterInteractableObject(tree.get());
        }
        for (const auto& node : m_resourceNodes) {
            interactionSystem->unregisterInteractableObject(node.get());
        }
    }

    m_trees.clear();
    m_resourceNodes.clear();
}

const std::vector<std::unique_ptr<Tree>>& Terrain::getTrees() const {
    return m_trees;
}

const std::vector<std::unique_ptr<ResourceNode>>& Terrain::getResourceNodes() const {
    return m_resourceNodes;
}