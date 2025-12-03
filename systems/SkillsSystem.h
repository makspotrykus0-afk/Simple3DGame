#pragma once

#include <string>
#include <memory> // For std::shared_ptr
#include <vector>
#include "SkillTypes.h"

// Forward declarations
class GameEntity; 
class SkillsComponent;

class SkillsSystem {
public:
    SkillsSystem() = default;
    ~SkillsSystem() = default;

    // Initialize the system, potentially loading skill data or configurations
    void initialize();

    // Update the system (e.g., process skill-related events)
    void update(float deltaTime);

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