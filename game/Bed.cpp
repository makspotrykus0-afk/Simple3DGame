#include "Bed.h"
#include "raylib.h"
#include "raymath.h" // For Vector3Subtract, Vector3Add, Vector3Scale
#include "rlgl.h"    // For rlPushMatrix, etc.
#include "../core/GameEntity.h"
#include <iostream>
#include <cmath>

Bed::Bed(const std::string& name, Vector3 position, float rotation)
    : BaseInteractableObject(name, InteractionType::INSPECTION, position, 1.5f), // Reduced interaction radius from 2.0 to 1.5
      m_occupied(false), 
      m_rotation(rotation),
      m_currentSleeper(nullptr) {
    
    // REDUCED COLLISION BOX (FIX for "collision in entire house")
    // We use a smaller box than the visual model to ensure easy movement around it.
    // Visual size is approx: 1.5 (X) x 0.4 (Y) x 2.0 (Z)
    // Physical collision box: 0.8 (X) x 0.4 (Y) x 1.2 (Z) - Only the core
    
    Vector3 boxSize = { 0.8f, 0.4f, 1.2f }; 

    // Handle rotation for AABB
    // Normalize rotation to 0-360 range
    float normRot = rotation;
    while (normRot < 0) normRot += 360.0f;
    while (normRot >= 360.0f) normRot -= 360.0f;

    // If rotated approx 90 or 270 degrees, swap dimensions X and Z
    if ((normRot > 45.0f && normRot < 135.0f) || (normRot > 225.0f && normRot < 315.0f)) {
        float temp = boxSize.x;
        boxSize.x = boxSize.z;
        boxSize.z = temp;
    }

    Vector3 halfSize = Vector3Scale(boxSize, 0.5f);
    
    m_boundingBox = BoundingBox{
        Vector3Subtract(position, halfSize),
        Vector3Add(position, halfSize)
    };
}

bool Bed::isOccupied() const {
    return m_occupied;
}

void Bed::setOccupied(bool occupied) {
    m_occupied = occupied;
    if (!occupied) {
        m_currentSleeper = nullptr;
    }
}

bool Bed::startSleeping(GameEntity* sleeper) {
    if (m_occupied) {
        return false;
    }
    m_occupied = true;
    m_currentSleeper = sleeper;
    return true;
}

void Bed::stopSleeping() {
    m_occupied = false;
    m_currentSleeper = nullptr;
}

GameEntity* Bed::getSleeper() const {
    return m_currentSleeper;
}

InteractionResult Bed::interact(GameEntity* player) {
    if (m_occupied) {
        // If the player interacting is the one sleeping, maybe wake them up?
        // For now, just say it's occupied.
        if (player == m_currentSleeper) {
             return InteractionResult(true, "You are resting.");
        }
        return InteractionResult(false, "Bed is occupied.");
    }
    
    // Basic interaction just inspects, actual sleeping is triggered by AI or specific command
    // But for player, we could potentially allow sleeping here if we wanted player mechanics
    return InteractionResult(true, "It's a comfy bed. You can sleep here to restore energy.");
}

InteractionInfo Bed::getDisplayInfo() const {
    InteractionInfo info = BaseInteractableObject::getDisplayInfo();
    info.objectDescription = m_occupied ? "Occupied Bed" : "Empty Bed";
    info.interactionPrompt = "Press E to Inspect"; 
    return info;
}

void Bed::render() {
    Color color = m_occupied ? DARKBLUE : BLUE;
    Vector3 size = { 1.5f, 0.4f, 2.0f };
    Vector3 drawPos = { m_position.x, m_position.y + 0.2f, m_position.z }; 
    
    // Apply rotation logic for rendering
    rlPushMatrix();
    rlTranslatef(drawPos.x, drawPos.y, drawPos.z);
    rlRotatef(m_rotation, 0.0f, 1.0f, 0.0f);
    
    // Draw Bed in local space
    DrawCube({0,0,0}, size.x, size.y, size.z, color);
    DrawCubeWires({0,0,0}, size.x, size.y, size.z, BLACK);
    
    // Pillow (local)
    DrawCube({0, 0.1f, -0.7f}, 1.2f, 0.2f, 0.4f, WHITE);
    
    rlPopMatrix();
    
    // Debug: Draw collision box
    // DrawBoundingBox(m_boundingBox, RED);
}

void Bed::update(float deltaTime) {
    BaseInteractableObject::update(deltaTime);
    
    // If we had logic to auto-eject sleeper or something, it could go here
}