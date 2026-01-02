#pragma once

#include "../core/IComponent.h"
#include "../systems/SkillTypes.h"
#include <map>
#include <string>
#include <vector>


struct Skill {
  SkillType type;
  std::string name;
  int level;
  float currentXP;
  float xpToNextLevel;
  int priority;

  Skill()
      : type(SkillType::UNKNOWN), name("Unknown"), level(1), currentXP(0.0f),
        xpToNextLevel(100.0f), priority(0) {}
  Skill(SkillType t, int l, float xp)
      : type(t), name(skillTypeToString(t)), level(l), currentXP(xp),
        xpToNextLevel(100.0f * (l + 1)), priority(0) {}
};

class SkillsComponent : public IComponent {
public:
  bool autoEquipBestItems = true;

  SkillsComponent() = default;
  ~SkillsComponent() = default;

  // Add a new skill to the component
  void addSkill(SkillType skillType, int level = 1, float xp = 0.0f) {
    if (m_skills.find(skillType) == m_skills.end()) {
      m_skills[skillType] = Skill(skillType, level, xp);
    }
  }

  // Get the level of a specific skill
  int getSkillLevel(SkillType skillType) const {
    auto it = m_skills.find(skillType);
    if (it != m_skills.end()) {
      return it->second.level;
    }
    return -1;
  }

  // Get the XP of a specific skill
  float getSkillXP(SkillType skillType) const {
    auto it = m_skills.find(skillType);
    if (it != m_skills.end()) {
      return it->second.currentXP;
    }
    return 0.0f;
  }

  // Add XP to a specific skill
  bool addSkillXP(SkillType skillType, float xp) {
    auto it = m_skills.find(skillType);
    if (it != m_skills.end()) {
      it->second.currentXP += xp;
      bool leveledUp = false;
      while (it->second.currentXP >= it->second.xpToNextLevel) {
        it->second.currentXP -= it->second.xpToNextLevel;
        it->second.level++;
        it->second.xpToNextLevel = 100.0f * (it->second.level + 1);
        leveledUp = true;
      }
      return leveledUp;
    }
    return false;
  }

  // Check if a skill exists
  bool hasSkill(SkillType skillType) const {
    return m_skills.find(skillType) != m_skills.end();
  }

  // Set priority
  void setSkillPriority(SkillType skillType, int priority) {
    auto it = m_skills.find(skillType);
    if (it != m_skills.end()) {
      if (priority < 0)
        priority = 0;
      if (priority > 10)
        priority = 10;
      it->second.priority = priority;
    }
  }

  int getSkillPriority(SkillType skillType) const {
    auto it = m_skills.find(skillType);
    if (it != m_skills.end()) {
      return it->second.priority;
    }
    return 0;
  }

  // Helper to get raw skill data
  const std::map<SkillType, Skill> &getAllSkills() const { return m_skills; }

  // Implement IComponent interface methods (stubs for compatibility)
  void update(float /*deltaTime*/) override {}
  void render() override {}
  void initialize() override {}
  void shutdown() override {}
  std::type_index getComponentType() const override {
    return std::type_index(typeid(SkillsComponent));
  }

private:
  std::map<SkillType, Skill> m_skills;
};
