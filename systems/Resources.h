#ifndef RESOURCES_H
#define RESOURCES_H

#include <string>
#include <unordered_map>
#include <cstdint>

// Definicje typów zasobów - używamy konsystentnych definicji z ResourceTypes.h
#include "ResourceTypes.h"

// Struktury dla systemu zasobów
struct ResourceConfig {
    std::string name;
    int maxStackSize;
    int baseValue;
};

struct PlayerResource {
    int32_t currentAmount;
    int32_t reservedAmount;
};

struct ResourceChangedEvent {
    std::string playerId;
    ResourceType type;
    int32_t oldAmount;
    int32_t newAmount;
    int32_t change;
    std::string source;
    double timestamp;
};

struct ResourceSystemStats {
    size_t totalResourceTypes;
    size_t totalPlayers;
    size_t totalTransactions;
};

// Struktura dla informacji o zasobie
struct Resource {
    ResourceType type;
    int amount;
    std::string source;
};

#endif // RESOURCES_H