#pragma once

#include "raylib.h"
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
};
