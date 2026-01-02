#include "Projectile.h"

Projectile::Projectile(Vector3 startPos, Vector3 targetPos, float speed, float damage)
    : position(startPos), speed(speed), damage(damage), active(true), lifeTime(5.0f) {
    
    // Calculate initial velocity based on direction to target
    Vector3 direction = Vector3Normalize(Vector3Subtract(targetPos, startPos));
    velocity = Vector3Scale(direction, speed);
}

void Projectile::update(float deltaTime) {
    if (!active) return;
    
    // Apply Gravity to Velocity
    velocity.y -= GRAVITY * deltaTime;

    // Move based on velocity
    position = Vector3Add(position, Vector3Scale(velocity, deltaTime));
    
    // Floor collision (simple check)
    if (position.y <= 0.0f) {
        active = false; // Destroy on ground impact
    }

    // Lifetime check
    lifeTime -= deltaTime;
    if (lifeTime <= 0.0f) {
        active = false;
    }
}

void Projectile::render() {
    if (!active) return;
    
    // Draw tracer line (long, thin, bright)
    if (Vector3LengthSqr(velocity) > 0.1f) {
        // Oblicz ogon na podstawie prędkości - im szybciej tym dłuższy
        // Dla 120.0f -> tracer ma np. 6.0f długości
        float trailLen = Vector3Length(velocity) * 0.08f; 
        if (trailLen < 1.0f) trailLen = 1.0f; // Minimalna długość

        Vector3 dir = Vector3Normalize(velocity);
        Vector3 tail = Vector3Subtract(position, Vector3Scale(dir, trailLen));
        
        // Główny rdzeń pocisku (jasny żółty)
        DrawLine3D(position, tail, YELLOW);
        
        // Opcjonalnie: nieco szerszy wizualnie efekt (jeśli wspierane przez engine, np. druga linia obok)
        // Ale DrawLine3D jest cienkie. 
        // Możemy narysować drugą linię w minimalnym offsecie dla "grubości" lub zostawić 1px.
        // W raylib DrawLine3D to proste linie GL.
        // Spróbujmy dodać lekki "glow" rysując krótszy, ale inny kolor w środku?
        // Na razie prosta linia wystarczy dla "wąskiego obiektu".
    }
}