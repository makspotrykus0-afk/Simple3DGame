#pragma once

#include <typeindex>
#include <string>

// Forward declaration to avoid circular dependency
class IComponent;

// Alias dla kompatybilności wstecznej - GameComponent to teraz IComponent
typedef IComponent GameComponent;