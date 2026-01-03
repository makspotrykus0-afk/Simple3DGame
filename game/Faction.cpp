#include "Faction.h"
#include "Settlement.h"
#include "raymath.h"
#include <iostream>

Faction::Faction(std::string name, Color color) : name(name), color(color) {

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
  for (auto &settlement : settlements) {
    settlement->Update(deltaTime);
  }

  // 2. Faction-level decision making (Strategy)
  decisionTimer += deltaTime;
  if (decisionTimer >= DECISION_INTERVAL) {
    decisionTimer = 0.0f;

    // Simple logic: If we have enough resources, tell settlements to expand
    // For prototype, we just log status
    // std::cout << "[Faction " << name << "] Gold: " << treasury.gold
    //           << " Pop: " << settlements.size() << " settlements" <<
    //           std::endl;
  }
}

// ... (Keep AddSettlement)

void Faction::AddSettlement(std::unique_ptr<Settlement> settlement) {
  settlements.push_back(std::move(settlement));
}

void Faction::SetRelation(const std::string &otherFactionName,
                          float opinionChange) {
  if (otherFactionName == name)
    return; // Cannot have relation with self

  auto &state = relations[otherFactionName];
  state.opinion += opinionChange;

  // Clamp opinion
  if (state.opinion > 100.0f)
    state.opinion = 100.0f;
  if (state.opinion < -100.0f)
    state.opinion = -100.0f;

  // Update status based on opinion
  if (state.opinion >= 50.0f)
    state.status = Relation::ALLY;
  else if (state.opinion >= 20.0f)
    state.status = Relation::FRIENDLY;
  else if (state.opinion >= -20.0f)
    state.status = Relation::NEUTRAL;
  else if (state.opinion >= -80.0f)
    state.status = Relation::HOSTILE;
  else
    state.status = Relation::WAR;

  std::cout << "[Faction " << name << "] Relation with " << otherFactionName
            << " changed to " << state.opinion << " (" << (int)state.status
            << ")" << std::endl;
}

float Faction::GetOpinion(const std::string &otherFactionName) const {
  auto it = relations.find(otherFactionName);
  if (it != relations.end()) {
    return it->second.opinion;
  }
  return 0.0f;
}

Faction::Relation
Faction::GetRelationStatus(const std::string &otherFactionName) const {
  auto it = relations.find(otherFactionName);
  if (it != relations.end()) {
    return it->second.status;
  }
  return Relation::NEUTRAL;
}

void Faction::UpdateEconomy(float deltaTime) {
  // Abstract economy simulation (Phase 2 stub)
  // Gather resources from settlements
  float dailyIncomeGold = 0.0f;

  for (const auto &settlement : settlements) {
    // Tax income based on population
    dailyIncomeGold += settlement->GetPopulation() * 0.1f * deltaTime;
  }

  treasury.gold += dailyIncomeGold;

  // Check goals
  if (treasury.gold > 2000.0f && !goals.expandedRecently) {
    std::cout << "[Faction " << name << "] Goal Met: Wealthy enough to expand!"
              << std::endl;
    AttemptExpansion();
    goals.expandedRecently = true; // Cooldown logic needed later
  }
}

void Faction::AttemptExpansion() {
  if (settlements.empty())
    return;

  // 1. Find a source settlement to expand from
  const auto &source = settlements.back(); // Simplest logic: expand from latest
  Vector3 sourcePos = source->GetLocation();

  // 2. Pick a random direction
  // For prototype: Just pick a neighboring region (N, S, E, or W)
  // Grid alignment assumed (100.0f region size)
  const float REGION_SIZE = 100.0f;
  Vector3 directions[] = {{REGION_SIZE, 0, 0},
                          {-REGION_SIZE, 0, 0},
                          {0, 0, REGION_SIZE},
                          {0, 0, -REGION_SIZE}};

  int dirIndex = GetRandomValue(0, 3);
  Vector3 targetPos = Vector3Add(sourcePos, directions[dirIndex]);

  // 3. Check if occupied (very basic check against own settlements only for
  // now)
  bool occupied = false;
  for (const auto &s : settlements) {
    if (Vector3Distance(s->GetLocation(), targetPos) < 10.0f) {
      occupied = true;
      break;
    }
  }

  // In a full implementation, we would check WorldManager for OTHER factions
  // too

  if (!occupied) {
    // 4. Found valid spot - Expand!
    treasury.gold -= 2000.0f; // Cost

    std::string newName =
        name + " Outpost " + std::to_string(settlements.size() + 1);
    AddSettlement(std::make_unique<Settlement>(newName, targetPos, this));

    std::cout << "[Faction " << name << "] EXPANDED! Founded " << newName
              << " at (" << targetPos.x << ", " << targetPos.z << ")"
              << std::endl;
  } else {
    std::cout << "[Faction " << name << "] Expansion failed: Target obstructed."
              << std::endl;
  }
}

void Faction::DebugDraw() {
  // Simple debug overlay list
  // This will be called by WorldManager::DrawDebugInfo
  // Implementation deferred to WorldManager to avoid gl calls in logic class
}
