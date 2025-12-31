
#pragma once
#include <string>

#include <memory> // For std::shared_ptr

#include <vector>
#include "SkillTypes.h"

#include "../core/GameSystem.h"
// Forward declarations

class GameEntity;

class SkillsComponent;
class SkillsSystem : public GameSystem {

public:

SkillsSystem(const std::string& name = "SkillsSystem") : GameSystem(name) {}

~SkillsSystem() override = default;
// Initialize the system, potentially loading skill data or configurations
void initialize() override;

// Update the system (e.g., process skill-related events)
void update(float deltaTime) override;

// Renderowanie systemu (puste, bo SkillsSystem nie renderuje)
void render() override;

// Zamykanie systemu (puste, ale wymagane przez interfejs)
void shutdown() override;

// Pobierz nazwę systemu (już zaimplementowane w GameSystem, ale możemy przesłonić jeśli potrzebne)
std::string getName() const override { return m_name; }

// Pobierz priorytet systemu (wg planu refaktoryzacji: 10)
int getPriority() const override { return 10; }

// Add a new skill to an entity's SkillsComponent
void addSkillToEntity(GameEntity* entity, SkillType skillType, int level = 1, float xp = 0.0f);

// Get the level of a skill for a specific entity
int getSkillLevelForEntity(GameEntity* entity, SkillType skillType) const;

// Add XP to a skill for a specific entity
void addXPToSkillForEntity(GameEntity* entity, SkillType skillType, float xp);

// Check if an entity has a specific skill
bool doesEntityHaveSkill(GameEntity* entity, SkillType skillType) const;

private:

// Helper to get the SkillsComponent from an entity

SkillsComponent* getSkillsComponent(GameEntity* entity) const;

};
