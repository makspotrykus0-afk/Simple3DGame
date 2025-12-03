#pragma once

#include <typeindex>

// Bazowa klasa dla komponentów - używana przez GameComponent
class IComponent {
public:
    virtual ~IComponent() = default;

    // Aktualizacja komponentu
    virtual void update(float deltaTime) = 0;

    // Renderowanie komponentu
    virtual void render() = 0;

    // Inicjalizacja komponentu
    virtual void initialize() = 0;

    // Zamykanie komponentu
    virtual void shutdown() = 0;

    // Pobierz typ komponentu
    virtual std::type_index getComponentType() const = 0;
};

// Alias dla kompatybilności wstecznej
typedef IComponent GameComponent;
