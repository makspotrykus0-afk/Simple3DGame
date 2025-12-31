#include "FoodSystem.h"
#include "../core/EventSystem.h"

FoodSystem::FoodSystem() : m_name("FoodSystem") {
}

void FoodSystem::initialize() {
    // Inicjalizacja systemu żywności
}

void FoodSystem::update(float deltaTime) {
    // Aktualizacja systemu żywności
    // Tutaj można dodać logikę zmniejszania głodu z czasem
}

void FoodSystem::render() {
    // Renderowanie systemu żywności
}

void FoodSystem::shutdown() {
    // Zamykanie systemu żywności
    m_playerNutrition.clear();
}

std::string FoodSystem::getName() const {
    return m_name;
}

int FoodSystem::getPriority() const {
    return 11; // Średni priorytet
}
