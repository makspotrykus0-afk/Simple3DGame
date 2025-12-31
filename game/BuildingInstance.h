#pragma once

#include <string>
#include <vector>
#include "raylib.h"
#include "../entities/GameEntity.h"
#include "../game/BuildingBlueprint.h"
#include "Door.h"
#include "Bed.h"
#include "../systems/StorageSystem.h" // Include full definition for StorageSlot
#include "raymath.h" // Include for Vector3RotateByAxisAngle
#include "rlgl.h"

// Forward declaration
class Door;
class Bed;

/**
 * @brief Instance of a built building in the game world
 */
class BuildingInstance : public GameEntity {
public:
    BuildingInstance(const std::string& blueprintId, const Vector3& position, float rotation)
        : GameEntity("Building_" + blueprintId), m_blueprintId(blueprintId), m_rotation(rotation), m_constructionProgress(0.0f), m_isBuilt(false), m_health(100.0f), m_maxHealth(100.0f), m_cachedBlueprint(nullptr) {
        setPosition(position); // Use base class setter
    }

    virtual ~BuildingInstance() = default;

    // Implement GameEntity virtual methods
    Vector3 getPosition() const override { return m_position; }
    void setPosition(const Vector3& position) override { m_position = position; }

    // Getters
    std::string getBlueprintId() const { return m_blueprintId; }
    float getRotation() const { return m_rotation; }
    float getConstructionProgress() const { return m_constructionProgress; }
    bool isBuilt() const { return m_isBuilt; }
    float getHealth() const { return m_health; }
    std::string getOwner() const { return m_owner; }
    std::string getStorageId() const { return m_storageId; }
    BuildingBlueprint* getBlueprint() const { return m_cachedBlueprint; }
    struct VisualStorageSlot {
        Resources::ResourceType type;
        int amount;
        Vector3 localPosition;
        Vector3 scale;
    };

    // Ensure visual slots are only accessed if they exist, and return const ref
    const std::vector<VisualStorageSlot>& getVisualSlots() const { return m_visualSlots; }
    
    // Setters
    void setConstructionProgress(float progress) { 
        m_constructionProgress = progress;
        if (m_constructionProgress >= 100.0f) {
            m_isBuilt = true;
            m_constructionProgress = 100.0f;
        }
    }
    
    void setBuilt(bool built) {
        m_isBuilt = built;
        if (built) m_constructionProgress = 100.0f;
    }
    
    void takeDamage(float amount) {
        m_health -= amount;
        if (m_health < 0.0f) m_health = 0.0f;
    }

    void repair(float amount) {
        m_health += amount;
        if (m_health > m_maxHealth) m_health = m_maxHealth;
    }

    void setOwner(const std::string& owner) { m_owner = owner; }
    void setStorageId(const std::string& id) { m_storageId = id; }
    void setBlueprint(BuildingBlueprint* bp) { m_cachedBlueprint = bp; }
    
    void setDoor(std::shared_ptr<Door> door) { m_door = door; }
    void setBed(std::shared_ptr<Bed> bed) { m_bed = bed; }

    bool addItem(std::unique_ptr<Item> item);

    // --- Wizualizacja magazynu ---

    void updateVisualStorage(const std::vector<StorageSlot>& storageSlots) {
        // std::cout << "[DEBUG] BuildingInstance::updateVisualStorage - Input slots: " << storageSlots.size() << std::endl;
        m_visualSlots.clear();
        
        // Simple layout grid 3x3 (or more) inside the building
        // Assume building size around 3x3 units
        // Start from -spacing, -spacing
        
        int slotIndex = 0;
        int cols = 3;
        float spacing = 0.6f; // Slightly reduced spacing to fit more items tightly
        float startX = -spacing;
        float startZ = -spacing;
        
        // Check if blueprint ID is "stockpile" to use specific logic
        if (m_blueprintId == "stockpile") {
             // Stockpile layout logic (possibly wider grid)
             cols = 4;
             spacing = 0.6f;
             startX = -0.9f;
             startZ = -0.9f;
        }

        for (const auto& slot : storageSlots) {
            if (slot.isOccupied && slot.amount > 0) {
                int row = slotIndex / cols;
                int col = slotIndex % cols;
                
                if (row >= 4) break; // Visual limit
                
                // Y position is raised to ensure visibility on top of floor/ground
                Vector3 localPos = {
                    startX + col * spacing,
                    0.15f, // Raised slightly more
                    startZ + row * spacing
                };
                
                // Scale visual representation based on amount (capped)
                float visualAmount = (float)slot.amount / 20.0f;
                if (visualAmount < 0.2f) visualAmount = 0.2f;
                if (visualAmount > 1.2f) visualAmount = 1.2f;
                
                VisualStorageSlot vSlot;
                vSlot.type = slot.resourceType;
                vSlot.amount = slot.amount;
                vSlot.localPosition = localPos;
                
                // Make items look slightly different based on type if needed, but scale by amount primarily
                // Using slightly larger base scale
                vSlot.scale = { 0.5f, 0.5f * visualAmount, 0.5f };
                
                m_visualSlots.push_back(vSlot);
                slotIndex++;
            }
        }
        // std::cout << "[DEBUG] BuildingInstance::updateVisualStorage - Visual slots created: " << m_visualSlots.size() << std::endl;
    }

    // Methods required by Settler and other systems
    BoundingBox getBoundingBox() const {
        // Return a simple box around the position for now
        // In reality, this should be based on the Blueprint's size
        Vector3 pos = getPosition();
        Vector3 size = { 2.0f, 3.0f, 2.0f }; // Default size
        if (m_cachedBlueprint) {
            size = m_cachedBlueprint->getSize();
        } else if (m_blueprintId == "floor") {
            size = { 2.0f, 0.1f, 2.0f };
        } else if (m_blueprintId == "wall") {
            size = { 2.0f, 3.0f, 0.5f };
        } else if (m_blueprintId == "stockpile") {
            size = { 3.0f, 0.1f, 3.0f };
        } else if (m_blueprintId == "house_4") {
            size = { 4.0f, 3.0f, 4.0f };
        }
        
        Vector3 halfSize = { size.x/2.0f, size.y/2.0f, size.z/2.0f };
        
        return {
            {pos.x - halfSize.x, pos.y, pos.z - halfSize.z},
            {pos.x + halfSize.x, pos.y + size.y, pos.z + halfSize.z}
        };
    }

    bool CheckCollision(Vector3 pos, float radius) const {
        // Use local m_position to avoid base class ambiguity
        BoundingBox box = getBoundingBox();
        return CheckCollisionBoxSphere(box, pos, radius);
    }
    
    Door* getDoor() const {
        return m_door.get(); 
    }

    Bed* getBed() const {
        return m_bed.get();
    }

    // Render (simple placeholder if not handled by a system)
    void render() override {
        Vector3 pos = getPosition();
        if (m_isBuilt) {
            // Main model rendering is handled by BuildingSystem
            // DrawCube(pos, 2.0f, 2.0f, 2.0f, GRAY);
        } else {
            DrawCubeWires(pos, 2.0f, 2.0f, 2.0f, YELLOW);
        }
        // Rendering storage slots is now handled by BuildingSystem to avoid duplication and Z-fighting
    }
    
    void update(float deltaTime) override {
        // Basic update logic if needed
    }

private:
    std::string m_blueprintId;
    float m_rotation;
    float m_constructionProgress;
    bool m_isBuilt;
    float m_health;
    float m_maxHealth;
    std::string m_owner;
    std::string m_storageId;
    
    BuildingBlueprint* m_cachedBlueprint;
    
    std::shared_ptr<Door> m_door;
    std::shared_ptr<Bed> m_bed;
    
    std::vector<VisualStorageSlot> m_visualSlots;
    Vector3 m_position; // Added to store position
};