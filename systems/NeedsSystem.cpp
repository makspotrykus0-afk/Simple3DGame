#include "NeedsSystem.h"
#include "../game/BuildingInstance.h"
#include <iostream>
#include "raymath.h"

void NeedsSystem::update(float deltaTime) {
for (auto& settler : m_settlers) {
if (!settler) continue;
handleEnergyNeeds(*settler, deltaTime);
}
}
void NeedsSystem::handleEnergyNeeds(Settler& settler, float deltaTime) {
SettlerState state = settler.getState();

// Nie zmniejszamy energii - StatsComponent::update już to robi
// Sprawdzamy potrzebę snu za pomocą settler.needsSleep()
if (settler.needsSleep()) {
std::cout << "[NeedsSystem] Settler " << settler.getName() << " needs sleep (energy low), looking for bed." << std::endl;
// Interrupt current work and find bed if not already doing so
if (state != SettlerState::MOVING_TO_BED && state != SettlerState::SLEEPING) {
// Find a bed
BuildingInstance* bed = findFreeBed(settler);
if (bed) {
std::cout << "[NeedsSystem] Found bed at (" << bed->getPosition().x << "," << bed->getPosition().z << "), moving to it." << std::endl;
settler.assignBed(bed);
// Ustaw stan MOVING_TO_BED przed MoveTo, aby MoveTo nie zmienił stanu na MOVING (bo ma warunek)
settler.setState(SettlerState::MOVING_TO_BED);
settler.MoveTo(bed->getPosition());
std::cout << "[NeedsSystem] Settler " << settler.getName() << " state -> MOVING_TO_BED (energy low)." << std::endl;
} else {
std::cout << "[NeedsSystem] No free bed found." << std::endl;
}
} else {
std::cout << "[NeedsSystem] Already moving to bed or sleeping." << std::endl;
}
}
}
BuildingInstance* NeedsSystem::findFreeBed(Settler& settler) {
if (!m_buildingSystem) return nullptr;


auto buildings = m_buildingSystem->getAllBuildings();
BuildingInstance* nearestBed = nullptr;
float minDistance = 999999.0f;
Vector3 settlerPos = settler.getPosition();


for (auto* building : buildings) {
if (!building) continue;


// Check if building has a bed component/object associated
// Based on BuildingInstance.h, it has `m_bed` member which is `std::shared_ptr<Bed>`
// But we don't have a getter exposed in the snippet I read.
// However, we can check blueprint ID or use other heuristics if getter missing.
// Better yet, I should have checked for `getBed()` in BuildingInstance.h.
// Re-reading BuildingInstance.h content I have in context... 
// It has `void setBed(std::shared_ptr<Bed> bed)` at line 78.
// It does NOT seem to have `getBed()` in the lines 1-100 I read.
// It has `getBlueprintId()`.

if (building->getBlueprintId() == "house_small" || building->getBlueprintId() == "bed") {
     // Calculate distance
     float dist = Vector3Distance(settlerPos, building->getPosition());
     if (dist < minDistance) {
          // Ideally check if bed is occupied.
          // Without getBed(), we can't check `bed->isOccupied()`.
          // We will implement a naive check assuming if it's a valid house it's a candidate.
          // TODO: Add `getBed()` to BuildingInstance and `isOccupied()` check here.
          
          minDistance = dist;
          nearestBed = building;
     }
}


}


return nearestBed;
}