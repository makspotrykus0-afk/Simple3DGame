#pragma once
#include <string>
#include <cstdint>

// Simple resource type enum
enum ResourceType 
{
    WOOD = 0,
    STONE = 1,
    FOOD = 2,
    METAL = 3,
    GOLD = 4
};

// Basic resource structure
struct ResourceInfo 
{
    ResourceType type;
    int amount;
    std::string source;
};