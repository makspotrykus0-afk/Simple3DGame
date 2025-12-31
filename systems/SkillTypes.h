#pragma once

#include <string>

// Enum defining different skill types in the game
enum class SkillType {
    // Gathering Skills
    WOODCUTTING,
    MINING,
    FARMING,
    FISHING,

    // Crafting & Building Skills
    SMITHING,
    CRAFTING,
    BUILDING,

    // Combat & Survival Skills
    COMBAT,
    SURVIVAL,

    // Add more skill types as needed
    UNKNOWN // Default or unknown skill type
};

// Helper function to convert SkillType to string for display or logging
inline std::string skillTypeToString(SkillType type) {
    switch (type) {
        case SkillType::WOODCUTTING: return "Woodcutting";
        case SkillType::MINING:      return "Mining";
        case SkillType::FARMING:     return "Farming";
        case SkillType::FISHING:     return "Fishing";
        case SkillType::SMITHING:    return "Smithing";
        case SkillType::CRAFTING:    return "Crafting";
        case SkillType::BUILDING:    return "Building";
        case SkillType::COMBAT:      return "Combat";
        case SkillType::SURVIVAL:    return "Survival";
        default:                     return "Unknown Skill";
    }
}

// Helper function to convert string to SkillType
inline SkillType stringToSkillType(const std::string& typeName) {
    if (typeName == "Woodcutting") return SkillType::WOODCUTTING;
    if (typeName == "Mining") return SkillType::MINING;
    if (typeName == "Farming") return SkillType::FARMING;
    if (typeName == "Fishing") return SkillType::FISHING;
    if (typeName == "Smithing") return SkillType::SMITHING;
    if (typeName == "Crafting") return SkillType::CRAFTING;
    if (typeName == "Building") return SkillType::BUILDING;
    if (typeName == "Combat") return SkillType::COMBAT;
    if (typeName == "Survival") return SkillType::SURVIVAL;
    return SkillType::UNKNOWN;
}