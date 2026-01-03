#include "Door.h"
#include "raymath.h"
#include "rlgl.h"
#include "Colony.h" // Include Colony header for full definition of Settler
#include <iostream>

Door::Door(const std::string& name, Vector3 position, float rotation, Colony* colony)
    : BaseInteractableObject(name, InteractionType::DOOR, position, 2.5f),
      m_colony(colony),
      m_isOpen(false),
      m_isLocked(false),
      m_currentAngle(0.0f),
      m_baseRotation(rotation),
      m_openAngle(90.0f),
      m_doorSpeed(180.0f), // 180 degrees per second
      m_width(2.0f),  // Width matched to wall size (2.0f)
      m_height(3.0f), // Height matched to wall size (3.0f)
      m_thickness(0.25f) // Thickness matched to wall (0.25f/0.5f?)
{
    m_description = "Drzwi do budynku. Otwierają się automatycznie.";
}

void Door::update(float deltaTime) {
    checkAutoOpen();

    // Animation logic
    float targetAngle = m_isOpen ? m_openAngle : 0.0f;
    
    if (m_currentAngle < targetAngle) {
        m_currentAngle += m_doorSpeed * deltaTime;
        if (m_currentAngle > targetAngle) m_currentAngle = targetAngle;
    } else if (m_currentAngle > targetAngle) {
        m_currentAngle -= m_doorSpeed * deltaTime;
        if (m_currentAngle < targetAngle) m_currentAngle = targetAngle;
    }
}

void Door::checkAutoOpen() {
    // Jeśli drzwi są zamknięte na klucz, nie otwieraj ich automatycznie
    if (m_isLocked) {
        if (m_isOpen) setOpen(false); // Upewnij się, że są fizycznie zamknięte
        return;
    }

    if (!m_colony) return;

    bool anyoneNear = false;
    float triggerDistance = 2.5f; 

    // Check ALL settlers (including player-controlled settlers)
    const auto& settlers = m_colony->getSettlers();
    for (const auto* settler : settlers) {
        float dist = Vector3Distance(settler->getPosition(), m_position);
        if (dist < triggerDistance) {
            anyoneNear = true;
            break;
        }
    }

    // Auto open/close logic  
    if (anyoneNear && !m_isOpen) {
        setOpen(true);
    } else if (!anyoneNear && m_isOpen) {
        setOpen(false);
    }
}

InteractionResult Door::interact(GameEntity* player) {
    if (m_isLocked) {
        return InteractionResult(false, "Drzwi są zamknięte.");
    }

    setOpen(!m_isOpen);
    std::string msg = m_isOpen ? "Drzwi otwarte." : "Drzwi zamkniete.";
    return InteractionResult(true, msg);
}

void Door::setOpen(bool open) {
    if (m_isLocked && open) return; // Cannot open locked door
    m_isOpen = open;
}

BoundingBox Door::getBoundingBox() const {
    // If open, return a very small box (effectively non-blocking)
    // Or move it out of the way
    if (m_isOpen || m_currentAngle > 10.0f) {
        Vector3 center = m_position;
        // Move box deep underground so CheckCollisionBoxSphere misses it
        center.y = -1000.0f; 
        return BoundingBox{ 
            Vector3Subtract(center, {0.1f, 0.1f, 0.1f}), 
            Vector3Add(center, {0.1f, 0.1f, 0.1f}) 
        };
    }
    
    // Default box (when closed)
    // AABB aligned with axes might be inaccurate if door is rotated but closed (usually closed = aligned)
    // Assuming closed door aligns with X or Z axis roughly for AABB checks.
    // For precise rotated collision we need OBB or custom check, but AABB is standard here.
    Vector3 halfSize = { m_width/2.0f, m_height/2.0f, m_thickness/2.0f };
    
    // Adjust for rotation (simple 90 degree check)
    // If baseRotation is 90 or 270, swap x/z thickness
    if (abs(fmod(m_baseRotation, 180.0f)) > 45.0f) {
        float temp = halfSize.x;
        halfSize.x = halfSize.z;
        halfSize.z = temp;
    }

    // Position is center of bottom edge? No, BaseInteractableObject/Door usually centers
    // In render() we translate to position (center of opening).
    // We need box centered at m_position + height/2
    Vector3 center = m_position;
    center.y += m_height / 2.0f;

    return BoundingBox{ 
        Vector3Subtract(center, halfSize), 
        Vector3Add(center, halfSize) 
    };
}

void Door::render() const {
    // Visual pivot calculation
    float totalAngle = m_baseRotation + m_currentAngle;
    
    // Use rlgl for transformations
    rlPushMatrix();
        rlTranslatef(m_position.x, m_position.y, m_position.z); // World Pos (Center of opening)
        
        // Shift to hinge position (Left edge of the 2.0 width door)
        rlTranslatef(-m_width / 2.0f, 0, 0);
        
        rlRotatef(totalAngle, 0, 1, 0); // Rotate around hinge
        
        // Move to center of the door volume for drawing (since DrawCube draws from center)
        rlTranslatef(m_width / 2.0f, m_height / 2.0f, 0);
        
        DrawCube(Vector3{0,0,0}, m_width, m_height, m_thickness, BROWN);
        DrawCubeWires(Vector3{0,0,0}, m_width, m_height, m_thickness, DARKBROWN);
        
        // Handle visual (Handle)
        // Position handle near the swinging edge (opposite to hinge)
        // Hinge is at local X=0 (relative to rotation). Door ends at X=width.
        // Cube center is at X=width/2.
        // We are currently at cube center.
        // Handle should be at X = width/2 - margin (relative to center? No, relative to hinge)
        // Relative to Center (0,0,0 here):
        // Right edge is at +width/2.
        // Handle should be near right edge: +width/2 - 0.2f
        rlTranslatef(m_width / 2.0f - 0.2f, 0, m_thickness/2.0f + 0.05f);
        DrawCube(Vector3{0,0,0}, 0.1f, 0.2f, 0.1f, GOLD);
        
    rlPopMatrix();
}