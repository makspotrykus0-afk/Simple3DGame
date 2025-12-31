#ifndef ENERGY_COMPONENT_H
#define ENERGY_COMPONENT_H


#include "raylib.h"
#include "../core/IComponent.h"
#include <typeindex>
#include <typeinfo>


struct EnergyComponent : public IComponent {
float currentEnergy;
float maxEnergy;
float decayRate;      // Energy loss per second when active
float rechargeRate;   // Energy gain per second when sleeping
float fatigueThreshold; // Energy level below which settler seeks bed


EnergyComponent(float max = 100.0f, float decay = 1.0f, float recharge = 10.0f, float threshold = 30.0f)
    : currentEnergy(max)
    , maxEnergy(max)
    , decayRate(decay)
    , rechargeRate(recharge)
    , fatigueThreshold(threshold) {}

// Implementacja metod IComponent
void update(float deltaTime) override {} // Dane są aktualizowane przez NeedsSystem
void render() override {} // Komponent nie ma reprezentacji wizualnej
void initialize() override {}
void shutdown() override {}

std::type_index getComponentType() const override {
    return std::type_index(typeid(EnergyComponent));
}


};


#endif // ENERGY_COMPONENT_H