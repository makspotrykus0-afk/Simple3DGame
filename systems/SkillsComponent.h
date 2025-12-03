#pragma once

#include <string>
#include <map>
#include <iostream>
#include "SkillTypes.h"

// Structure to represent a skill
struct Skill {
    SkillType type;
    std::string name;
    int level;
    float currentXP;
    float xpToNextLevel; 

    Skill() : type(SkillType::UNKNOWN), name("Unknown"), level(1), currentXP(0.0f), xpToNextLevel(100.0f) {}
    Skill(SkillType t, int l, float xp) 
        : type(t), name(skillTypeToString(t)), level(l), currentXP(xp), xpToNextLevel(100.0f * (l + 1)) {}
};

class SkillsComponent {
public:
    SkillsComponent() = default;
    ~SkillsComponent() = default;

    // Adds a skill or updates it if it exists
    void addSkill(SkillType skillType, int level = 1, float xp = 0.0f) {
        std::string skillName = skillTypeToString(skillType);
        if (m_skills.find(skillType) == m_skills.end()) {
            m_skills[skillType] = Skill(skillType, level, xp);
        } else {
            // Optionally update level/xp? For now, just log or ignore
            // std::cout << "Skill " << skillName << " already exists." << std::endl;
        }
    }

    // Checks if the entity has the skill
    bool hasSkill(SkillType skillType) const {
        return m_skills.find(skillType) != m_skills.end();
    }

    // Returns the level of the skill, or -1 if not found
    int getSkillLevel(SkillType skillType) const {
        auto it = m_skills.find(skillType);
        if (it != m_skills.end()) {
            return it->second.level;
        }
        return -1;
    }

    // Adds XP to a skill and handles potential level up
    // Returns true if leveled up
    bool addSkillXP(SkillType skillType, float xp) {
        auto it = m_skills.find(skillType);
        if (it != m_skills.end()) {
            it->second.currentXP += xp;
            bool leveledUp = false;
            // Simple level up logic
            while (it->second.currentXP >= it->second.xpToNextLevel) {
                it->second.currentXP -= it->second.xpToNextLevel;
                it->second.level++;
                it->second.xpToNextLevel = 100.0f * (it->second.level + 1); // Linear scaling for now
                leveledUp = true;
                std::cout << "Skill " << it->second.name << " leveled up to " << it->second.level << "!" << std::endl;
            }
            return leveledUp;
        }
        return false;
    }

    float getSkillXP(SkillType skillType) const {
        auto it = m_skills.find(skillType);
        if (it != m_skills.end()) {
            return it->second.currentXP;
        }
        return 0.0f;
    }
    
    float getXPToNextLevel(SkillType skillType) const {
        auto it = m_skills.find(skillType);
        if (it != m_skills.end()) {
            return it->second.xpToNextLevel;
        }
        return 100.0f;
    }

    // Helper to get raw skill data (optional)
    const std::map<SkillType, Skill>& getAllSkills() const {
        return m_skills;
    }

private:
    std::map<SkillType, Skill> m_skills;
};