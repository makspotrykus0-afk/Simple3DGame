#include "Projectile.h"

Projectile::Projectile(Vector3 startPos, Vector3 targetPos, float speed, float damage)
    : position(startPos), speed(speed), damage(damage), active(true), lifeTime(3.0f) {
    
    // Calculate direction
    direction = Vector3Normalize(Vector3Subtract(targetPos, startPos));
}

void Projectile::update(float deltaTime) {
    if (!active) return;
    
    // Move
    position = Vector3Add(position, Vector3Scale(direction, speed * deltaTime));
    
    // Lifetime check
    lifeTime -= deltaTime;
    if (lifeTime <= 0.0f) {
        active = false;
    }
}

void Projectile::render() {
    if (!active) return;
    
    // Draw simple bullet
    DrawSphere(position, 0.1f, YELLOW);
    // Draw trail
    Vector3 tail = Vector3Subtract(position, Vector3Scale(direction, 0.5f));
    DrawLine3D(position, tail, ORANGE);
}