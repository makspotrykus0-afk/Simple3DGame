#pragma once

#include <map>
#include <string>
#include <vector>
#include "../core/IComponent.h" // Poprawiona ścieżka do IComponent.h

// Forward declarations
class Skill;

class SkillsComponent : public IComponent {
public:
    SkillsComponent() = default;
    ~SkillsComponent() = default;

    // Add a new skill to the component
    void addSkill(const std::string& skillName, int level = 0, int xp = 0);

    // Get the level of a specific skill
    int getSkillLevel(const std::string& skillName) const;

    // Get the XP of a specific skill
    int getSkillXP(const std::string& skillName) const;

    // Add XP to a specific skill
    void addSkillXP(const std::string& skillName, int xp);

    // Check if a skill exists
    bool hasSkill(const std::string& skillName) const;

    // Get all skill names
    std::vector<std::string> getSkillNames() const;

    // Implement IComponent interface methods
    void update(float deltaTime) override;
    void render() override;
    void initialize() override;
    void shutdown() override;
    std::type_index getComponentType() const override;

private:
    struct SkillData {
        int level = 0;
        int xp = 0;
    };

    std::map<std::string, SkillData> m_skills;
};
