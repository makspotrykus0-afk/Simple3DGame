#pragma once

#include "raylib.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

// Forward declarations
class Settlement;

/**
 * @brief Represents resources owned by a Faction (Global Economy)
 */
struct FactionTreasury {
  float gold = 0.0f;
  float food = 0.0f;
  float wood = 0.0f;
  float stone = 0.0f;
  float iron = 0.0f;
};

/**
 * @brief Base class for an AI Faction
 *
 * Factions are high-level entities that manage multiple settlements,
 * conduct diplomacy, and make strategic decisions.
 */
class Faction {
private:
  std::string name;
  Color color;
  FactionTreasury treasury;
  std::vector<std::unique_ptr<Settlement>> settlements;

  // Logic timing
  float decisionTimer = 0.0f;
  static constexpr float DECISION_INTERVAL =
      5.0f; // Faction thinks every 5 seconds

public:
  enum class Relation {
    ALLY,     // +50 to +100
    FRIENDLY, // +20 to +50
    NEUTRAL,  // -20 to +20
    HOSTILE,  // -80 to -20
    WAR       // -100 to -80
  };

  struct DiplomacyState {
    float opinion = 0.0f; // -100 to +100
    Relation status = Relation::NEUTRAL;
  };

  struct FactionGoals {
    // Abstract resource targets for the economy
    float targetFood = 1000.0f;
    float targetWood = 500.0f;
    float targetStone = 200.0f;
    bool expandedRecently = false;
  };

private:
  // Faction ID -> Diplomacy State
  std::map<std::string, DiplomacyState> relations;
  FactionGoals goals;

public:
  // Diplomacy Interface
  void SetRelation(const std::string &otherFactionName, float opinionChange);
  float GetOpinion(const std::string &otherFactionName) const;
  Relation GetRelationStatus(const std::string &otherFactionName) const;

  // Economy Interface
  const FactionGoals &GetGoals() const { return goals; }
  void UpdateEconomy(float deltaTime);

public:
  Faction(std::string name, Color color);
  ~Faction();

  void Update(float deltaTime);

  // Getters
  const std::string &GetName() const { return name; }
  Color GetColor() const { return color; }
  FactionTreasury &GetTreasury() { return treasury; }
  const std::vector<std::unique_ptr<Settlement>> &GetSettlements() const {
    return settlements;
  }

  // Logic
  void AddSettlement(std::unique_ptr<Settlement> settlement);
  void DebugDraw(); // Draw debug info overlay

private:
  void AttemptExpansion();
};
