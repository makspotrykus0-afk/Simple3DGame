#include "NavComponent.h"
#include "../game/Settler.h"
#include "../game/NavigationGrid.h"
#include "raymath.h"
#include <iostream>

#include "../core/GameSystem.h"

NavComponent::NavComponent(Settler* owner) : m_owner(owner) {
    if (m_owner) {
        m_lastPosition = m_owner->getPosition();
    }
}

std::type_index NavComponent::getComponentType() const {
    return std::type_index(typeid(NavComponent));
}

void NavComponent::update(float deltaTime) {
    if (!m_owner || m_owner->isPlayerControlled()) return;

    updatePathFollowing(deltaTime);
    updateStuckDetection(deltaTime);
}

void NavComponent::render() {
    // 9.5: Opcjonalne rysowanie ścieżki w trybie debug
    if (!m_currentPath.empty()) {
        for (size_t i = 0; i < m_currentPath.size(); ++i) {
            DrawSphere(m_currentPath[i], 0.1f, {0, 255, 255, 100});
            if (i > 0) {
                DrawLine3D(m_currentPath[i-1], m_currentPath[i], SKYBLUE);
            }
        }
    }
}

void NavComponent::moveTo(Vector3 target) {
    NavigationGrid* navGrid = GameSystem::getNavigationGrid();
    if (!m_owner || !navGrid) return;
    // Unikamy restartowania ścieżki jeśli cel jest niemal identyczny
    if (Vector3Distance(target, m_lastPathTarget) < 0.2f && m_lastPathValid) {
        return;
    }

    auto path = navGrid->FindPath(m_owner->getPosition(), target);
    m_currentPath = std::deque<Vector3>(path.begin(), path.end());
    m_lastPathTarget = target;
    m_lastPathValid = !m_currentPath.empty();
    m_stuckTimer = 0.0f;

    if (!m_lastPathValid) {
        // std::cout << "[NavComponent] No path found to target." << std::endl;
    }
}

void NavComponent::stop() {
    m_currentPath.clear();
    m_lastPathValid = false;
}

void NavComponent::updatePathFollowing(float deltaTime) {
    if (m_currentPath.empty()) return;

    Vector3 myPos = m_owner->getPosition();
    Vector3 nextPoint = m_currentPath.front();
    
    // Ignorujemy wysokość Y dla dystansu (zakładamy płaski teren dla AI)
    Vector3 myPos2D = {myPos.x, 0, myPos.z};
    Vector3 target2D = {nextPoint.x, 0, nextPoint.z};

    float dist = Vector3Distance(myPos2D, target2D);
    
    if (dist < 0.3f) {
        m_currentPath.pop_front();
        if (m_currentPath.empty()) {
            return;
        }
    }

    // Ruch w stronę punktu
    Vector3 direction = Vector3Normalize(Vector3Subtract(nextPoint, myPos));
    direction.y = 0; // Blokada lotu

    Vector3 velocity = Vector3Scale(direction, m_movementSpeed * deltaTime);
    m_owner->setPosition(Vector3Add(myPos, velocity));

    // Płynna rotacja (9.5 Masterpiece)
    float targetRotation = atan2f(direction.x, direction.z) * RAD2DEG;
    float currentRot = m_owner->getRotation();
    
    // Krótki dystans kątowy
    float diff = targetRotation - currentRot;
    while (diff > 180) diff -= 360;
    while (diff < -180) diff += 360;

    m_owner->setRotation(currentRot + diff * m_rotationSmoothing * deltaTime);
}

void NavComponent::updateStuckDetection(float deltaTime) {
    if (m_currentPath.empty()) {
        m_stuckTimer = 0.0f;
        return;
    }

    Vector3 currentPos = m_owner->getPosition();
    float distMoved = Vector3Distance(currentPos, m_lastPosition);

    if (distMoved < STUCK_DISTANCE_EPSILON) {
        m_stuckTimer += deltaTime;
        if (m_stuckTimer > STUCK_THRESHOLD) {
            handleStuck();
        }
    } else {
        m_stuckTimer = 0.0f;
    }

    m_lastPosition = currentPos;
}

void NavComponent::handleStuck() {
    // std::cout << "[NavComponent] Stuck detected! Re-routing..." << std::endl;
    m_stuckTimer = 0.0f;
    
    // Próba lekkiego "odbicia" lub po prostu re-path
    if (m_lastPathValid) {
        m_lastPathValid = false; // Wymusi nowy path w następnym moveTo
        moveTo(m_lastPathTarget);
    }
}
