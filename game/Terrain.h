#pragma once

#include "raylib.h"
#include <vector>
#include <memory>

class Tree;
class ResourceNode;

class Terrain {
public:
    Terrain();
    ~Terrain();

    void generate(int width, int height, float tileSize);
    void render();
    void update();
    void cleanup();

    float getHeightAt(int x, int y) const;
    float getInterpolatedHeightAt(float x, float z) const;
    
    // New method for precise height detection via raycasting
    float getRaycastHeightAt(float x, float z) const;

    void setHeightAt(int x, int y, float height);
    bool isAccessible(Vector3 position) const;

    int getWidth() const { return width; }
    int getHeight() const { return height; }
    float getTileSize() const { return tileSize; }

    const std::vector<std::unique_ptr<Tree>>& getTrees() const;
    const std::vector<std::unique_ptr<ResourceNode>>& getResourceNodes() const;

    void addTree(std::unique_ptr<Tree> tree);
    void removeTree(Tree* tree);
    void addResourceNode(std::unique_ptr<ResourceNode> node);
    void removeResourceNode(ResourceNode* node);

private:
    int width;
    int height;
    float tileSize;
    std::vector<float> heightMap;
    
    Mesh mesh;
    Model model;
    
    std::vector<std::unique_ptr<Tree>> m_trees;
    std::vector<std::unique_ptr<ResourceNode>> m_resourceNodes;
};