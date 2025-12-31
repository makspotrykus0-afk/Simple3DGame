#include "StatsComponent.h"
#include <algorithm> // For std::clamp

// Constructor definition
StatsComponent::StatsComponent(const std::string& ownerId, float maxHealth, float maxEnergy, float maxStamina, float maxHunger)
    : m_ownerId(ownerId), m_maxHealth(maxHealth), m_maxEnergy(maxEnergy), m_maxStamina(maxStamina), m_maxHunger(maxHunger),
      m_currentHealth(maxHealth), m_currentEnergy(maxEnergy), m_currentStamina(maxStamina), m_currentHunger(maxHunger) {
}

// Setters for max values
void StatsComponent::setMaxHealth(float maxHealth) { m_maxHealth = maxHealth; }
void StatsComponent::setMaxEnergy(float maxEnergy) { m_maxEnergy = maxEnergy; }
void StatsComponent::setMaxStamina(float maxStamina) { m_maxStamina = maxStamina; } 
void StatsComponent::setMaxHunger(float maxHunger) { m_maxHunger = maxHunger; }

// Setters for current values
void StatsComponent::setHealth(float currentHealth) { 
    m_currentHealth = std::clamp(currentHealth, 0.0f, m_maxHealth); 
}
void StatsComponent::setEnergy(float currentEnergy) { 
    m_currentEnergy = std::clamp(currentEnergy, 0.0f, m_maxEnergy); 
}
void StatsComponent::setStamina(float currentStamina) { 
    m_currentStamina = std::clamp(currentStamina, 0.0f, m_maxStamina); 
}
void StatsComponent::setHunger(float currentHunger) {
    m_currentHunger = std::clamp(currentHunger, 0.0f, m_maxHunger);
}

// Modify values
void StatsComponent::modifyHealth(float amount) {
    m_currentHealth += amount;
    m_currentHealth = std::clamp(m_currentHealth, 0.0f, m_maxHealth);
}

void StatsComponent::modifyEnergy(float amount) {
    m_currentEnergy += amount;
    m_currentEnergy = std::clamp(m_currentEnergy, 0.0f, m_maxEnergy);
}

void StatsComponent::modifyStamina(float amount) { 
    m_currentStamina += amount;
    m_currentStamina = std::clamp(m_currentStamina, 0.0f, m_maxStamina);
}

void StatsComponent::modifyHunger(float amount) {
    m_currentHunger += amount;
    m_currentHunger = std::clamp(m_currentHunger, 0.0f, m_maxHunger);
}

// Getters
float StatsComponent::getCurrentHealth() const { return m_currentHealth; }
float StatsComponent::getMaxHealth() const { return m_maxHealth; }
float StatsComponent::getCurrentEnergy() const { return m_currentEnergy; }
float StatsComponent::getMaxEnergy() const { return m_maxEnergy; }
float StatsComponent::getCurrentStamina() const { return m_currentStamina; }
float StatsComponent::getMaxStamina() const { return m_maxStamina; }
float StatsComponent::getCurrentHunger() const { return m_currentHunger; }
float StatsComponent::getMaxHunger() const { return m_maxHunger; }

// Decay methods
void StatsComponent::decayHunger(float amount) {
    modifyHunger(-amount);
}

void StatsComponent::decayEnergy(float amount) {
    modifyEnergy(-amount);
}

bool StatsComponent::isAlive() const { return m_currentHealth > 0.0f; }
bool StatsComponent::isStarving() const { return m_currentHunger <= 0.0f; }
bool StatsComponent::isExhausted() const { return m_currentEnergy <= 0.0f; }

// IComponent interface implementations
void StatsComponent::update(float deltaTime) {
    // Auto decay hunger (3x slower)
    if (m_currentHunger > 0.0f) {
        decayHunger((m_hungerDecayRate / 3.0f) * deltaTime);
    }

    // Auto decay energy (3x slower)
    if (m_currentEnergy > 0.0f) {
        decayEnergy((m_energyDecayRate / 3.0f) * deltaTime);
    }
}

void StatsComponent::render() {
    // Tutaj można dodać logikę renderowania, jeśli jest potrzebna
}

void StatsComponent::initialize() {
    // Inicjalizacja komponentu
}

void StatsComponent::shutdown() {
    // Zamykanie komponentu
}

std::type_index StatsComponent::getComponentType() const {
    return std::type_index(typeid(StatsComponent));
}
