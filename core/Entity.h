#pragma once

#include <any>
#include <map>
#include <string>
#include <typeindex>
#include <utility> // For std::move

// Basic Entity class with component management
class Entity {
public:
    Entity(const std::string& id = "") : m_id(id) {}
    ~Entity() = default;

    template<typename T>
    void addComponent(T component) {
        m_components[std::type_index(typeid(T))] = std::make_any<T>(std::move(component));
    }

    template<typename T>
    T* getComponent() {
        auto it = m_components.find(std::type_index(typeid(T)));
        if (it != m_components.end()) {
            try {
                // Cast to T& and return its address. Assumes T is stored by value.
                return std::any_cast<T>(&it->second);
            } catch (const std::bad_any_cast&) {
                return nullptr;
            }
        }
        return nullptr;
    }

    template<typename T>
    const T* getComponent() const {
        auto it = m_components.find(std::type_index(typeid(T)));
        if (it != m_components.end()) {
            try {
                return std::any_cast<const T>(&it->second);
            } catch (const std::bad_any_cast&) {
                return nullptr;
            }
        }
        return nullptr;
    }

    template<typename T>
    bool hasComponent() const {
        return m_components.count(std::type_index(typeid(T)));
    }

    std::string getID() const {
        return m_id;
    }

    void setID(const std::string& id) {
        m_id = id;
    }

private:
    std::string m_id; // ID for the entity
    std::map<std::type_index, std::any> m_components;
};