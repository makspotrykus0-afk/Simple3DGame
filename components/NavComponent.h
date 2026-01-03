#ifndef NAV_COMPONENT_H
#define NAV_COMPONENT_H

#include "../core/IComponent.h"
#include "raylib.h"
#include <vector>
#include <deque>
#include <typeindex>
#include <typeinfo>

// Forward declarations
class Settler;

/**
 * @class NavComponent
 * @brief Komponent zarządzający ruchem, śledzeniem ścieżki i detekcją utknięcia.
 * Implementuje Protokół Płynności 9.5.
 */
class NavComponent : public IComponent {
public:
    NavComponent(Settler* owner);
    virtual ~NavComponent() = default;

    // IComponent interface
    void update(float deltaTime) override;
    void render() override;
    void initialize() override {}
    void shutdown() override {}
    std::type_index getComponentType() const override;

    // Navigation Core
    void moveTo(Vector3 target);
    void stop();
    bool isMoving() const { return !m_currentPath.empty(); }
    
    // Getters / Setters
    Vector3 getLastPathTarget() const { return m_lastPathTarget; }
    bool isPathValid() const { return m_lastPathValid; }

private:
    Settler* m_owner;

    // Path data
    std::deque<Vector3> m_currentPath;
    Vector3 m_lastPathTarget = {0, 0, 0};
    bool m_lastPathValid = false;

    // Stuck Detection
    float m_stuckTimer = 0.0f;
    Vector3 m_lastPosition = {0, 0, 0};
    const float STUCK_THRESHOLD = 0.5f; // Czas w sekundach
    const float STUCK_DISTANCE_EPSILON = 0.05f;

    // Movement params
    float m_movementSpeed = 3.5f;
    float m_rotationSmoothing = 10.0f; // 9.5: Płynny obrót

    // Internal Helpers
    void updatePathFollowing(float deltaTime);
    void updateStuckDetection(float deltaTime);
    void handleStuck();
};

#endif // NAV_COMPONENT_H
