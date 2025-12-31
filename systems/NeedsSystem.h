#pragma once
#include "../core/GameSystem.h"
#include "../game/Settler.h"
#include "../game/Bed.h"
#include "../systems/BuildingSystem.h"
#include <vector>
#include <memory>

class NeedsSystem : public GameSystem {
public:
NeedsSystem() : GameSystem("NeedsSystem"), m_buildingSystem(nullptr) {}
// Parameterized constructor for backward compatibility or specific initialization
NeedsSystem(const std::vector<Settler*>& settlers, BuildingSystem* buildingSystem)
: GameSystem("NeedsSystem"), m_settlers(settlers), m_buildingSystem(buildingSystem) {}
void update(float deltaTime) override;
void initialize() override {}
// Implementacja renderowania
void render() override {}
void shutdown() override {}
int getPriority() const override { return 5; } // Middle priority

void setBuildingSystem(BuildingSystem* system) { m_buildingSystem = system; }
void addSettler(Settler* settler) { m_settlers.push_back(settler); }

// Structure for UI display
struct NeedInfo {
float currentValue;
float maxValue;
};

// Helper to get needs for a settler (for UI)
std::unordered_map<std::string, NeedInfo> getNeeds(const Settler* settler) const {
std::unordered_map<std::string, NeedInfo> needs;
if (!settler) return needs;
const auto& stats = settler->getStats();
needs["Health"] = { stats.getCurrentHealth(), stats.getMaxHealth() };
needs["Hunger"] = { stats.getCurrentHunger(), stats.getMaxHunger() };
// Accessing EnergyComponent directly might be harder if we don't have non-const access or getters on Settler
// Assuming we can get it via standard component system or if Settler exposes it.
// Settler::getComponent is a template. We need to cast const away or use a const version if available.
// For this simple UI helper, we'll trust stats component for primary needs.
// If Settler had getComponent<const T>, we could use it.
// Let's try to use stats->getCurrentEnergy() if it exists, or leave it out.
// Based on previous code, stats has getCurrentEnergy.
needs["Energy"] = { stats.getCurrentEnergy(), stats.getMaxEnergy() };
return needs;
}
private:
std::vector<Settler*> m_settlers;
BuildingSystem* m_buildingSystem;
void handleEnergyNeeds(Settler& settler, float deltaTime);
BuildingInstance* findFreeBed(Settler& settler);
};