#pragma once

#include "raylib.h"
#include "raymath.h"

class Projectile {
public:
    Projectile(Vector3 startPos, Vector3 targetPos, float speed = 15.0f, float damage = 35.0f);
    
    void update(float deltaTime);
    void render();
    
    bool isActive() const { return active; }
    Vector3 getPosition() const { return position; }
    float getDamage() const { return damage; }
    void deactivate() { active = false; }

private:
    Vector3 position;
    Vector3 velocity; // Changed from direction/speed to velocity for physics
    float speed;      // Kept for initial calculation or reference if needed
    float damage;
    bool active;
    float lifeTime;

    static constexpr float GRAVITY = 9.81f;
};