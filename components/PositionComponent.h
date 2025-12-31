#pragma once

#include "raylib.h"
#include "raymath.h" // Używamy Vector3 z raylib zamiast własnej definicji
#include "../core/IComponent.h" // Dołączamy bazową klasę komponentu
#include <typeindex> // Dla typeid

// Upewniamy się, że PositionComponent jest polimorficzny, dziedzicząc po IComponent
// i dodając wirtualny destruktor, co jest wymagane dla dynamic_cast.
class PositionComponent : public IComponent {
public:
    // Zwraca typ komponentu, aby systemy mogły go identyfikować.
    std::type_index getComponentType() const override {
        return typeid(PositionComponent);
    }

    // Wirtualny destruktor jest kluczowy dla polimorfizmu
    virtual ~PositionComponent() = default;

    PositionComponent(Vector3 pos = {0.0f, 0.0f, 0.0f}) : m_position(pos) {}

    Vector3 getPosition() const { return m_position; }
    void setPosition(Vector3 pos) { m_position = pos; }
    void setPosition(float x, float y, float z) { m_position = {x, y, z}; }

    // Implementacja czysto wirtualnych metod z IComponent
    void update(float /*deltaTime*/) override {
        // Implementacja logiki aktualizacji pozycji, jeśli jest potrzebna
        // Na razie pusta, ponieważ pozycja jest zarządzana przez setPosition
    }

    void render() override {
        // Komponent pozycji zazwyczaj nie renderuje niczego bezpośrednio
    }

    void initialize() override {
        // Inicjalizacja, jeśli potrzebna
    }

    void shutdown() override {
        // Czyszczenie, jeśli potrzebne
    }

private:
    Vector3 m_position;
};