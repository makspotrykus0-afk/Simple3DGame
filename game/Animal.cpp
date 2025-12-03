#include "Animal.h"
#include "Colony.h"
#include "../systems/ResourceTypes.h"
#include "../game/Item.h"
#include <cmath>
#include <raymath.h>

Animal::Animal(AnimalType type, Vector3 position)
    : Entity("Animal")
    , BaseInteractableObject((type == AnimalType::RABBIT) ? "Krolik" : "Jelen", InteractionType::HUNTING, position, 2.0f)
    , m_type(type)
    , m_targetPosition(position)
    , m_moveTimer(0.0f)
    , m_idleTimer(0.0f)
    , m_isMoving(false)
    , m_moveSpeed(1.0f)
{
    float hp = (type == AnimalType::RABBIT) ? 20.0f : 100.0f;
    // ID, MaxHealth, MaxEnergy, MaxStamina, MaxHunger
    m_stats = std::make_unique<StatsComponent>("Animal", hp, 100.0f, 100.0f, 100.0f);
    
    m_stats->setHunger(0.0f);
    
    if (type == AnimalType::RABBIT) m_moveSpeed = 3.0f;
    if (type == AnimalType::DEER) m_moveSpeed = 5.0f;
    
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
    Vector3 size = (m_type == AnimalType::RABBIT) ? Vector3{0.5f, 0.5f, 0.5f} : Vector3{1.0f, 1.5f, 2.0f};
    
    // Offset Y so the animal stands ON the ground, not halfway in it.
    // DrawCube centers at the position given.
    Vector3 drawPos = pos;
    drawPos.y += size.y / 2.0f;
    
    // Simple cube representation for now
    DrawCube(drawPos, size.x, size.y, size.z, color);
    DrawCubeWires(drawPos, size.x, size.y, size.z, BLACK);
    
    // Health bar
    if (m_stats && m_stats->getCurrentHealth() < m_stats->getMaxHealth()) {
        Vector3 barPos = { drawPos.x, drawPos.y + size.y/2.0f + 0.5f, drawPos.z };
        float healthPct = m_stats->getCurrentHealth() / m_stats->getMaxHealth();
        if (healthPct < 0.0f) healthPct = 0.0f;
        
        DrawCube(barPos, 1.0f, 0.1f, 0.1f, RED);
        Vector3 greenBarPos = barPos;
        greenBarPos.x -= (1.0f - (1.0f * healthPct)) / 2.0f; // Center align fix attempt
        DrawCube(greenBarPos, 1.0f * healthPct, 0.1f, 0.11f, GREEN);
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
    
    m_stats->modifyHealth(-amount);
    if (m_stats->getCurrentHealth() <= 0) {
        die();
    } else {
        // Flee logic
        m_isMoving = true;
        Vector3 currentPos = getPosition();
        
        float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
        m_targetPosition.x = currentPos.x + cos(angle) * 10.0f;
        m_targetPosition.z = currentPos.z + sin(angle) * 10.0f;
        m_targetPosition.y = currentPos.y;
    }
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
            Vector3 newPos = Vector3Add(currentPos, Vector3Scale(dir, m_moveSpeed * deltaTime));
            
            newPos.y = 0.0f; // Should use Terrain::getHeight
            
            // Update position component AND BaseInteractableObject position
            auto* posComp = getComponent<PositionComponent>();
            if (posComp) {
                posComp->setPosition(newPos);
            }
            // Also update base class position storage to keep in sync if needed
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
    
    auto meat = std::make_unique<ConsumableItem>("Surowe Mies", "Swieze mieso ze zwierzecia.");
    colony.addDroppedItem(std::move(meat), getPosition());
}