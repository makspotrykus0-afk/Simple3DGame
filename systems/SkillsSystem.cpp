#include "SkillsSystem.h"
#include "SkillsComponent.h"
#include "../core/GameEntity.h" 
#include <iostream>

void SkillsSystem::initialize() {
    // Initialization logic
    std::cout << "SkillsSystem initialized." << std::endl;
}

void SkillsSystem::update(float deltaTime) {
    (void)deltaTime; // Suppress unused parameter warning
    // Update logic
}

SkillsComponent* SkillsSystem::getSkillsComponent(GameEntity* entity) const {
    if (!entity) {
        return nullptr;
    }
    // GameEntity uses std::shared_ptr to store components.
    // We check if the component exists by attempting to get it.
    // The getComponent method returns std::shared_ptr<T>
    auto component = entity->getComponent<SkillsComponent>();
    if (component) {
         return component.get();
    }
    return nullptr;
}

void SkillsSystem::addSkillToEntity(GameEntity* entity, SkillType skillType, int level, float xp) {
    SkillsComponent* skillsComp = getSkillsComponent(entity);
    if (skillsComp) {
        skillsComp->addSkill(skillType, level, xp);
    } else {
        // Log error or handle case where entity has no SkillsComponent
        // std::cerr << "Error: Entity does not have SkillsComponent to add skill." << std::endl;
    }
}

int SkillsSystem::getSkillLevelForEntity(GameEntity* entity, SkillType skillType) const {
    SkillsComponent* skillsComp = getSkillsComponent(entity);
    if (skillsComp) {
        return skillsComp->getSkillLevel(skillType);
    }
    return -1; // Indicate skill not found or component missing
}

void SkillsSystem::addXPToSkillForEntity(GameEntity* entity, SkillType skillType, float xp) {
    SkillsComponent* skillsComp = getSkillsComponent(entity);
    if (skillsComp) {
        if (skillsComp->addSkillXP(skillType, xp)) {
             // TODO: Emit LevelUpEvent
             std::cout << "Entity leveled up skill: " << skillTypeToString(skillType) << std::endl;
        }
    } else {
        // std::cerr << "Error: Entity does not have SkillsComponent to add XP." << std::endl;
    }
}

bool SkillsSystem::doesEntityHaveSkill(GameEntity* entity, SkillType skillType) const {
    SkillsComponent* skillsComp = getSkillsComponent(entity);
    if (skillsComp) {
        return skillsComp->hasSkill(skillType);
    }
    return false; // Component missing or skill not present
}
