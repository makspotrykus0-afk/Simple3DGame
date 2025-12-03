#include "SkillsComponent.h"
#include "../core/Logger.h" // Assuming Logger is available

void SkillsComponent::addSkill(const std::string& skillName, int level, int xp) {
    if (m_skills.find(skillName) == m_skills.end()) {
        m_skills[skillName] = {level, xp};
        Logger::log(LogLevel::Info, "Skill '" + skillName + "' added with level " + std::to_string(level) + " and XP " + std::to_string(xp));
    } else {
        Logger::log(LogLevel::Warning, "Skill '" + skillName + "' already exists.");
    }
}

int SkillsComponent::getSkillLevel(const std::string& skillName) const {
    auto it = m_skills.find(skillName);
    if (it != m_skills.end()) {
        return it->second.level;
    }
    return -1; // Indicate skill not found
}

int SkillsComponent::getSkillXP(const std::string& skillName) const {
    auto it = m_skills.find(skillName);
    if (it != m_skills.end()) {
        return it->second.xp;
    }
    return -1; // Indicate skill not found
}

void SkillsComponent::addSkillXP(const std::string& skillName, int xp) {
    auto it = m_skills.find(skillName);
    if (it != m_skills.end()) {
        it->second.xp += xp;
        // Logic for leveling up could be added here, based on xp thresholds
        Logger::log(LogLevel::Debug, "Added " + std::to_string(xp) + " XP to skill '" + skillName + "'. Total XP: " + std::to_string(it->second.xp));
    } else {
        Logger::log(LogLevel::Warning, "Cannot add XP to non-existent skill '" + skillName + "'.");
    }
}

bool SkillsComponent::hasSkill(const std::string& skillName) const {
    return m_skills.count(skillName) > 0;
}

// Implementation of getSkillNames, now correctly placed in SkillsComponent
std::vector<std::string> SkillsComponent::getSkillNames() const {
    std::vector<std::string> names;
    for (const auto& pair : m_skills) {
        names.push_back(pair.first);
    }
    return names;
}

void SkillsComponent::update(float deltaTime) {
    // Placeholder for update logic if needed
}

void SkillsComponent::render() {
    // Placeholder for render logic if needed
}

void SkillsComponent::initialize() {
    // Placeholder for initialization logic if needed
}

void SkillsComponent::shutdown() {
    // Placeholder for shutdown logic if needed
}

std::type_index SkillsComponent::getComponentType() const {
    return std::type_index(typeid(SkillsComponent));
}