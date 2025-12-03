#pragma once

#include <string>

// Undefine potential macro conflicts from Windows headers or Raylib
// We use PascalCase for enum members to avoid these conflicts, but strictly speaking, 
// if a macro is defined, it might still cause issues if we matched the case.
// Since we are switching to PascalCase (Wood, Stone, etc.), we shouldn't conflict with uppercase macros like GOLD.
// However, keeping undefs for safety if we were using uppercase. But for PascalCase it's less critical.
// We will remove them to avoid confusion, or keep them if we are paranoid. 
// Actually, let's keep the undefs just in case some windows header defines 'Wood' (unlikely).
// But Raylib defines 'GOLD'. Our enum will be 'Gold'. Case sensitive language, so no conflict.

namespace Resources {
    
    // Consolidated ResourceType definition
    // Merges definitions from ResourceTypes.h and ResourceSystem.h
    enum class ResourceType {
        None = 0,
        Wood,
        Stone,
        Metal, // Corresponds to IRON in some contexts
        Gold,
        Food,
        Water,
        Count
    };

    // Helper function to convert ResourceType to string
    inline std::string resourceTypeToString(ResourceType type) {
        switch (type) {
            case ResourceType::Wood: return "Wood";
            case ResourceType::Stone: return "Stone";
            case ResourceType::Food: return "Food";
            case ResourceType::Metal: return "Metal";
            case ResourceType::Gold: return "Gold";
            case ResourceType::Water: return "Water";
            case ResourceType::None: return "None";
            default: return "Unknown";
        }
    }
}