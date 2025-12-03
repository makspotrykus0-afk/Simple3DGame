#pragma once

#include <memory>
#include <vector>
#include <algorithm>
#include "IGameSystem.h" // Include full definition for dynamic_cast
#include "EventSystem.h"
#include "../game/Item.h" // Include for Item definitions

class IGameSystem;
class EventSystem;

class GameEngine {
private:
    std::vector<std::unique_ptr<IGameSystem>> systems;
    std::unique_ptr<EventSystem> eventSystem;

    // Private constructor for Singleton
    GameEngine() : eventSystem(std::make_unique<EventSystem>()) {}

public:
    static GameEngine& getInstance();

    // Usunięto wirtualność i czystą wirtualność, ponieważ usuwamy Pimpl
    // Inicjalizacja silnika gry
    void initialize();

    // Aktualizacja silnika gry
    void update(float deltaTime);

    // Renderowanie silnika gry
    void render();

    // Zamykanie silnika gry
    void shutdown();

    // Dodaj system do silnika gry
    void addSystem(std::unique_ptr<IGameSystem> system);
    
    // Register system (overload for unique_ptr)
    void registerSystem(std::unique_ptr<IGameSystem> system);

    // Register system (wrapper for raw pointers, primarily for backward compatibility if needed, but prefer unique_ptr)
    void registerSystem(IGameSystem* system) = delete; // DEPRECATED: Removed to prevent double free issues

    // Pobierz system z silnika gry
    template<typename T>
    T* getSystem() const {
        for (const auto& system : systems) {
            if (T* t = dynamic_cast<T*>(system.get())) {
                return t;
            }
        }
        return nullptr;
    }

    // Metoda do pobierania systemu zdarzeń
    EventSystem* getEventSystem() const;

    // --- Global Item Drop Management ---
    // Usually this should be in an ItemSystem, but for now we centralize it here or forward it.
    // We will use a callback or std::function if we want to decouple from main.cpp,
    // OR we can just expose a vector here.
    // BUT, since main.cpp has worldDroppedItems, let's try to keep it simple.
    // Ideally GameEngine should manage entities/items.
    
    // Let's add a method to spawn item in world.
    // For now, we'll just store a static callback to avoid major refactoring of main.cpp
    // or we can expose the method if we link main.cpp logic here.
    
    // Better approach for this codebase:
    // We will declare a static function pointer that main.cpp can set.
    using DropItemCallback = void(*)(Vector3 position, Item* item);
    static DropItemCallback dropItemCallback;
    
    static void spawnItem(Vector3 position, Item* item) {
        if (dropItemCallback) {
            dropItemCallback(position, item);
        }
    }
};