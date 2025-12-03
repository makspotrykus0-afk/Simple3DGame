#pragma once

#include "../core/IComponent.h"
#include <string>
#include <algorithm> // For std::clamp
#include <typeindex> // Include for type_index

class StatsComponent : public IComponent {
public:
    // Updated constructor to include maxStamina and initialize m_currentStamina
    StatsComponent(const std::string& ownerId, float maxHealth, float maxEnergy, float maxStamina, float maxHunger);

    // Setters for max values
    void setMaxHealth(float maxHealth);
    void setMaxEnergy(float maxEnergy);
    void setMaxStamina(float maxStamina); 
    void setMaxHunger(float maxHunger);

    // Setters for current values
    void setHealth(float currentHealth);
    void setEnergy(float currentEnergy);
    void setStamina(float currentStamina); 
    void setHunger(float currentHunger);

    // Modify values
    void modifyHealth(float amount);
    void modifyEnergy(float amount);
    void modifyStamina(float amount); 
    void modifyHunger(float amount);

    // Getters
    float getCurrentHealth() const;
    float getMaxHealth() const;
    float getCurrentEnergy() const;
    float getMaxEnergy() const;
    float getCurrentStamina() const; 
    float getMaxStamina() const; 
    float getCurrentHunger() const;
    float getMaxHunger() const;

    // Decay methods (called in update)
    void decayHunger(float amount);
    void decayEnergy(float amount);

    // Check if alive
    bool isAlive() const;
    bool isStarving() const;
    bool isExhausted() const;

    // IComponent interface implementations (declarations only in header)
    void update(float deltaTime) override;
    void render() override;
    void initialize() override;
    void shutdown() override;
    std::type_index getComponentType() const override;

private:
    std::string m_ownerId;
    float m_maxHealth;
    float m_maxEnergy;
    float m_maxStamina; 
    float m_maxHunger;

    float m_currentHealth;
    float m_currentEnergy;
    float m_currentStamina; 
    float m_currentHunger;
};