#include "Faction.h"
#include "Settlement.h"
#include <iostream>

Faction::Faction(std::string name, Color color)
    : name(name), color(color) {
    
    // Initial resources
    treasury.gold = 1000.0f;
    treasury.food = 500.0f;
    treasury.wood = 200.0f;
    treasury.stone = 100.0f;
    
    std::cout << "[Faction] Created " << name << std::endl;
}

Faction::~Faction() {
    std::cout << "[Faction] Destroyed " << name << std::endl;
}

void Faction::Update(float deltaTime) {
    // 1. Update all owned settlements
    for (auto& settlement : settlements) {
        settlement->Update(deltaTime);
    }
    
    // 2. Faction-level decision making (Strategy)
    decisionTimer += deltaTime;
    if (decisionTimer >= DECISION_INTERVAL) {
        decisionTimer = 0.0f;
        
        // Simple logic: If we have enough resources, tell settlements to expand
        // For prototype, we just log status
        // std::cout << "[Faction " << name << "] Gold: " << treasury.gold 
        //           << " Pop: " << settlements.size() << " settlements" << std::endl;
    }
}

void Faction::AddSettlement(std::unique_ptr<Settlement> settlement) {
    settlements.push_back(std::move(settlement));
}

void Faction::DebugDraw() {
    // Simple debug overlay list
    // This will be called by WorldManager::DrawDebugInfo
    // Implementation deferred to WorldManager to avoid gl calls in logic class
}
