#include "SkillsSystem.h"
#include "../components/SkillsComponent.h" // Poprawiona ścieżka dołączenia
#include "Entity.h"          // Assuming Entity class is defined in Entity.h
#include <iostream>         // For basic output, replace with game logger if available

// Placeholder for Entity class, replace with your actual Entity implementation
// class Entity {
// public:
//     template<typename T>
//     T* getComponent() {
//         // Implementation to retrieve a component from the entity
//         return nullptr; // Placeholder
//     }
// };

void SkillsSystem::initialize() {
    // Initialization logic, e.g., loading skill data, setting up event handlers
    std::cout << "SkillsSystem initialized." << std::endl;
}

void SkillsSystem::update(float deltaTime) {
    // Update logic, e.g., processing passive skill effects, checking for level-ups
    // This might involve iterating through entities with SkillsComponents
}

SkillsComponent* SkillsSystem::getSkillsComponent(Entity* entity) const {
    if (!entity) {
        return nullptr;
    }
    // Assuming Entity has a method to get components by type
    // Replace with your actual component retrieval mechanism
    return entity->getComponent<SkillsComponent>();
}

void SkillsSystem::addSkillToEntity(Entity* entity, const std::string& skillName, int level, int xp) {
    SkillsComponent* skillsComp = getSkillsComponent(entity);
    if (skillsComp) {
        skillsComp->addSkill(skillName, level, xp);
    } else {
        // Log error or handle case where entity has no SkillsComponent
        std::cerr << "Error: Entity does not have SkillsComponent to add skill." << std::endl;
    }
}

int SkillsSystem::getSkillLevelForEntity(Entity* entity, const std::string& skillName) const {
    SkillsComponent* skillsComp = getSkillsComponent(entity);
    if (skillsComp) {
        return skillsComp->getSkillLevel(skillName);
    }
    return -1; // Indicate skill not found or component missing
}

void SkillsSystem::addXPToSkillForEntity(Entity* entity, const std::string& skillName, int xp) {
    SkillsComponent* skillsComp = getSkillsComponent(entity);
    if (skillsComp) {
        skillsComp->addSkillXP(skillName, xp);
    } else {
        std::cerr << "Error: Entity does not have SkillsComponent to add XP." << std::endl;
    }
}

bool SkillsSystem::doesEntityHaveSkill(Entity* entity, const std::string& skillName) const {
    SkillsComponent* skillsComp = getSkillsComponent(entity);
    if (skillsComp) {
        return skillsComp->hasSkill(skillName);
    }
    return false; // Component missing or skill not present
}