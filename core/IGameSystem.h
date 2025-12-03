#pragma once

#include <string>

class IGameSystem {
public:
    virtual ~IGameSystem() = default;

    // Inicjalizacja systemu
    virtual void initialize() = 0;

    // Aktualizacja systemu
    virtual void update(float deltaTime) = 0;

    // Renderowanie systemu
    virtual void render() = 0;

    // Zamykanie systemu
    virtual void shutdown() = 0;

    // Pobierz nazwę systemu
    virtual std::string getName() const = 0;

    // Pobierz priorytet systemu
    virtual int getPriority() const = 0;
};