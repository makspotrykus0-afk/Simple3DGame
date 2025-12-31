#include "Animal.h"
#include "Colony.h"
#include "../systems/ResourceTypes.h"
#include "../game/Item.h"
#include <cmath>
#include <raymath.h>
#include <rlgl.h>  // Dla rlPushMatrix, rlRotatef, etc.

Animal::Animal(AnimalType type, Vector3 position)
    : Entity("Animal")
    , BaseInteractableObject((type == AnimalType::RABBIT) ? "Krolik" : "Jelen", InteractionType::HUNTING, position, 2.0f)
    , m_type(type)
    , m_targetPosition(position)
    , m_moveTimer(0.0f)
    , m_idleTimer(0.0f)
    , m_isMoving(false)
    , m_moveSpeed(1.0f)
    , m_rotation(0.0f)  // Inicjalizacja rotacji
{
    float hp = (type == AnimalType::RABBIT) ? 20.0f : 100.0f;
    // ID, MaxHealth, MaxEnergy, MaxStamina, MaxHunger
    m_stats = std::make_unique<StatsComponent>("Animal", hp, 100.0f, 100.0f, 100.0f);
    
    m_stats->setHunger(0.0f);
    
    // Zmniejszone prędkości dla lepszej grywalności
    if (type == AnimalType::RABBIT) m_moveSpeed = 2.0f;  // Było 3.0f
    if (type == AnimalType::DEER) m_moveSpeed = 3.5f;    // Było 5.0f
    
    addComponent(PositionComponent(position));
}

void Animal::update(float deltaTime) {
    updateAI(deltaTime);
    
    // Note: We do NOT call m_stats->update(deltaTime) here.
    // StatsComponent automatically increases hunger and decreases energy over time.
    // For animals, we want to disable this survival mechanic to prevent them from dying of hunger.
    // Only health is managed via takeDamage.
}

void Animal::render() {
    Vector3 pos = getPosition();
    Color color = (m_type == AnimalType::RABBIT) ? BROWN : DARKBROWN;
    
    // Zastosuj rotację - zapisz stan macierzy
    rlPushMatrix();
    rlTranslatef(pos.x, 0.0f, pos.z);  // Przesuń do pozycji
    rlRotatef(m_rotation * RAD2DEG, 0.0f, 1.0f, 0.0f);  // Obróć wokół osi Y
    rlTranslatef(-pos.x, 0.0f, -pos.z);  // Przesuń z powrotem
    
    if (m_type == AnimalType::RABBIT) {
        // Królik - bardziej rozpoznawalny model (Z ROTACJĄ)
        Vector3 bodyPos = pos;
        bodyPos.y += 0.25f;
        DrawCube(bodyPos, 0.6f, 0.5f, 0.8f, BROWN);
        DrawCubeWires(bodyPos, 0.6f, 0.5f, 0.8f, BLACK);
        
        Vector3 headPos = bodyPos;
        headPos.z -= 0.5f;  // Przód (w kierunku patrzenia)
        headPos.y += 0.15f;
        DrawCube(headPos, 0.4f, 0.35f, 0.4f, BEIGE);
        DrawCubeWires(headPos, 0.4f, 0.35f, 0.4f, BLACK);
        
        Vector3 leftEar = headPos;
        leftEar.x -= 0.15f;
        leftEar.y += 0.3f;
        DrawCube(leftEar, 0.1f, 0.4f, 0.1f, BROWN);
        
        Vector3 rightEar = headPos;
        rightEar.x += 0.15f;
        rightEar.y += 0.3f;
        DrawCube(rightEar, 0.1f, 0.4f, 0.1f, BROWN);
        
        Vector3 tailPos = bodyPos;
        tailPos.z += 0.45f; // Z tyłu
        DrawSphere(tailPos, 0.15f, WHITE);
    } else {
        // Jeleń
        Vector3 size = Vector3{1.0f, 1.5f, 2.0f};
        Vector3 drawPos = pos;
        drawPos.y += size.y / 2.0f;
        DrawCube(drawPos, size.x, size.y, size.z, color);
        DrawCubeWires(drawPos, size.x, size.y, size.z, BLACK);
    }
    
    rlPopMatrix();  // Przywróć stan macierzy
    
    // Health bar - TYLKO GDY HP < 100%
    if (m_stats && m_stats->getCurrentHealth() < m_stats->getMaxHealth()) {
        Vector3 barPos = { pos.x, pos.y + 1.2f, pos.z };
        float healthPct = m_stats->getCurrentHealth() / m_stats->getMaxHealth();
        if (healthPct < 0.0f) healthPct = 0.0f;
        if (healthPct > 1.0f) healthPct = 1.0f;
        
        DrawCube(barPos, 1.0f, 0.15f, 0.05f, RED);
        Vector3 greenBarPos = barPos;
        greenBarPos.x -= (1.0f - healthPct) * 0.5f;
        DrawCube(greenBarPos, healthPct * 1.0f, 0.15f, 0.06f, GREEN);
        DrawCubeWires(barPos, 1.0f, 0.15f, 0.05f, BLACK);
    }
}

InteractionResult Animal::interact(GameEntity* interactor) {
    // For now, interaction equals attack
    (void)interactor;
    takeDamage(10.0f); // Static damage for now
    return InteractionResult(true, "Attacked animal", 0.5f);
}

InteractionInfo Animal::getDisplayInfo() const {
    InteractionInfo info;
    info.objectName = getName();
    info.objectDescription = (m_type == AnimalType::RABBIT) ? "Maly, szybki krolik." : "Plochliwy jelen.";
    info.type = InteractionType::HUNTING;
    info.position = getPosition();
    info.distance = 0.0f; // Calculated by caller
    info.canInteract = true;
    info.interactionPrompt = "Poluj";
    return info;
}

Vector3 Animal::getPosition() const {
    // Prefer component position
    const auto* posComp = const_cast<Animal*>(this)->getComponent<PositionComponent>();
    if (posComp) {
        return posComp->getPosition();
    }
    // Fallback to BaseInteractableObject position storage
    return m_position;
}

bool Animal::isActive() const {
    return m_stats && m_stats->isAlive();
}

void Animal::takeDamage(float amount) {
    if (!m_stats) return;
    
    std::cout << "[Animal] Taking damage: " << amount << ", HP before: " << m_stats->getCurrentHealth() << std::endl;
    
    m_stats->modifyHealth(-amount);
    
    std::cout << "[Animal] HP after: " << m_stats->getCurrentHealth() << ", isDead: " << (m_stats->getCurrentHealth() <= 0) << std::endl;
    
    if (m_stats->getCurrentHealth() <= 0) {
        std::cout << "[Animal] DYING - calling die()" << std::endl;
        die();
    } else {
        // Flee logic - zwiększona odległość ucieczki
        m_isMoving = true;
        Vector3 currentPos = getPosition();
        
        float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
        float fleeDistance = 15.0f;  // Zwiększone z 10.0f do 15.0f
        m_targetPosition.x = currentPos.x + cos(angle) * fleeDistance;
        m_targetPosition.z = currentPos.z + sin(angle) * fleeDistance;
        m_targetPosition.y = currentPos.y;
        
        std::cout << "[Animal] FLEEING from " << currentPos.x << "," << currentPos.z 
                  << " to " << m_targetPosition.x << "," << m_targetPosition.z << std::endl;
    }
}

void Animal::scareAway(Vector3 threatPosition) {
    // Uciekaj w przeciwnym kierunku od zagrożenia
    m_isMoving = true;
    Vector3 currentPos = getPosition();
    Vector3 fleeDir = Vector3Subtract(currentPos, threatPosition);
    if (Vector3Length(fleeDir) < 0.01f) {
        // Jeśli za blisko, wybierz losowy kierunek
        float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
        fleeDir.x = cos(angle);
        fleeDir.z = sin(angle);
    } else {
        fleeDir = Vector3Normalize(fleeDir);
    }
    
    float fleeDistance = 8.0f;  // Królik ucieka na 8 jednostek
    m_targetPosition = Vector3Add(currentPos, Vector3Scale(fleeDir, fleeDistance));
    m_targetPosition.y = currentPos.y;
    
    std::cout << "[Animal] SCARED! Fleeing from threat at " << threatPosition.x << "," << threatPosition.z << std::endl;
}

bool Animal::isDead() const {
    return m_stats && !m_stats->isAlive();
}

void Animal::updateAI(float deltaTime) {
    if (!m_stats || !m_stats->isAlive()) return;

    Vector3 currentPos = getPosition();

    if (m_isMoving) {
        Vector3 dir = Vector3Subtract(m_targetPosition, currentPos);
        float dist = Vector3Length(dir);
        
        if (dist < 0.1f) {
            m_isMoving = false;
            m_idleTimer = (float)GetRandomValue(2, 5);
        } else {
            dir = Vector3Normalize(dir);
            
            // OBLICZ ROTACJĘ z kierunku ruchu
            // UWAGA: Model królika ma głowę na -Z, więc odwracamy rotację
            m_rotation = atan2f(-dir.x, -dir.z);  // Odwrócona rotacja dla poprawnego kierunku
            
            Vector3 newPos = Vector3Add(currentPos, Vector3Scale(dir, m_moveSpeed * deltaTime));
            newPos.y = 0.0f;
            
            auto* posComp = getComponent<PositionComponent>();
            if (posComp) {
                posComp->setPosition(newPos);
            }
            const_cast<Animal*>(this)->setPosition(newPos);
        }
    } else {
        m_idleTimer -= deltaTime;
        if (m_idleTimer <= 0) {
            m_isMoving = true;
            float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
            float dist = (float)GetRandomValue(3, 8);
            m_targetPosition.x = currentPos.x + cos(angle) * dist;
            m_targetPosition.z = currentPos.z + sin(angle) * dist;
            m_targetPosition.y = currentPos.y;
        }
    }
}

void Animal::die() {
    extern Colony colony;
    
    Vector3 deathPos = getPosition();
    std::cout << "[Animal] DIE() called at position: " << deathPos.x << "," << deathPos.y << "," << deathPos.z << std::endl;
    
    auto meat = std::make_unique<ConsumableItem>("Surowe Mies", "Swieze mieso ze zwierzecia.");
    std::cout << "[Animal] Created meat item, calling colony.addDroppedItem..." << std::endl;
    
    colony.addDroppedItem(std::move(meat), deathPos);
    
    std::cout << "[Animal] Meat dropped successfully!" << std::endl;
}