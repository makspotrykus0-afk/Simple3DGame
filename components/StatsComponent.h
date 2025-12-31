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

    // Hunger configuration
    float m_hungerDecayRate = 1.0f; // Points per second
    float m_energyDecayRate = 2.0f; // Points per second
    float m_hungerThreshold = 20.0f; // Below this, settler is hungry (critical)
    float m_foodSearchThreshold = 40.0f; // Below this, start looking for food

public:
    void setHungerDecayRate(float rate) { m_hungerDecayRate = rate; }
    float getHungerDecayRate() const { return m_hungerDecayRate; }

    void setEnergyDecayRate(float rate) { m_energyDecayRate = rate; }
    float getEnergyDecayRate() const { return m_energyDecayRate; }
    
    void setHungerThreshold(float threshold) { m_hungerThreshold = threshold; }
    float getHungerThreshold() const { return m_hungerThreshold; }
    
    void setFoodSearchThreshold(float threshold) { m_foodSearchThreshold = threshold; }
    float getFoodSearchThreshold() const { return m_foodSearchThreshold; }
};