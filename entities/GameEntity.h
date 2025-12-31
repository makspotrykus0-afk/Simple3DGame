#pragma once

#include <string>
#include <vector>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <utility> // For std::move
#include <raylib.h> // Pozostawiam jedno dołączenie raylib.h

#include "../core/IComponent.h"  // Poprawiona ścieżka do IComponent.h

class GameEntity {
public:
    GameEntity(const std::string& id) : m_id(id), m_visible(true) {}

    virtual ~GameEntity() = default;

    const std::string& getId() const { return m_id; }

    // Dodaj komponent do encji
    // Akceptuje std::shared_ptr<T> i przechowuje go jako std::shared_ptr<IComponent>
    // Overload for adding components that are already derived from IComponent
    void addComponent(std::shared_ptr<IComponent> component) {
        if (component) {
            m_components[component->getComponentType()] = component;
        }
    }

    // Template to add components derived from IComponent
    template<typename T>
    void addComponent(std::shared_ptr<T> component) {
        if (component) {
            // Use dynamic_pointer_cast for safer casting when adding components
            // Ensure T itself is derived from IComponent or can be cast to it
            auto baseComponent = std::dynamic_pointer_cast<IComponent>(component);
            if (baseComponent) {
                m_components[typeid(T)] = baseComponent;
            } else {
                // Log an error or handle the case where T is not compatible with IComponent
                // For now, we'll assume it should be compatible if this template is used.
            }
        }
    }

    // Pobierz komponent z encji (const version)
    // Zwraca std::shared_ptr<const T>
    template<typename T>
    std::shared_ptr<const T> getComponent() const {
        auto it = m_components.find(typeid(T));
        if (it != m_components.end()) {
            // Użyj dynamic_pointer_cast do bezpiecznego rzutowania na typ T
            return std::dynamic_pointer_cast<const T>(it->second);
        }
        return nullptr;
    }

    // Pobierz komponent z encji (non-const version)
    // Zwraca std::shared_ptr<T>
    template<typename T>
    std::shared_ptr<T> getComponent() {
        auto it = m_components.find(typeid(T));
        if (it != m_components.end()) {
            // Użyj dynamic_pointer_cast do bezpiecznego rzutowania na typ T
            return std::dynamic_pointer_cast<T>(it->second);
        }
        return nullptr;
    }

    // Usuń komponent z encji
    template<typename T>
    void removeComponent() {
        m_components.erase(typeid(T));
    }

    // Aktualizacja encji
    virtual void update(float deltaTime) {
        // Unused parameter warning fix if needed, but using it in loop
        (void)deltaTime; 
        for (auto& pair : m_components) {
            pair.second->update(deltaTime);
        }
    }

    // Renderowanie encji
    virtual void render() {
        for (auto& pair : m_components) {
            pair.second->render();
        }
    }

    // Sprawdź czy encja jest widoczna
    bool isVisible() const { return m_visible; }

    // Ustaw widoczność encji
    void setVisible(bool visible) { m_visible = visible; }

    // Pobierz pozycję encji
    virtual Vector3 getPosition() const = 0;

    // Ustaw pozycję encji
    virtual void setPosition(const Vector3& position) = 0;

protected:
    std::string m_id;
    bool m_visible;
    // Use shared_ptr to manage component lifetimes and polymorphism more easily
    std::unordered_map<std::type_index, std::shared_ptr<IComponent>> m_components;
};