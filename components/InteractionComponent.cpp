#include "InteractionComponent.h"
#include "../entities/GameEntity.h"
#include <algorithm>

InteractionComponent::InteractionComponent()
    : m_interactionRange(5.0f)
    , m_canInteract(true)
    , m_currentTarget(nullptr)
    , m_lastInteractionTime(0.0f)
{
    // Domyślnie dodaj wszystkie typy interakcji
    m_interactableTypes = {
        InteractionType::RESOURCE_GATHERING,
        InteractionType::ITEM_PICKUP,
        InteractionType::INSPECTION,
        InteractionType::CONTAINER,
        InteractionType::BUILDING_CONSTRUCTION,
        InteractionType::CRAFTING,
        InteractionType::SWITCH
    };
}

void InteractionComponent::update(float deltaTime) {
    // Aktualizacja komponentu interakcji
    // Tutaj można dodać logikę automatycznego czyszczenia nieaktywnych celów
    if (m_currentTarget && !m_currentTarget->isActive()) {
        m_currentTarget = nullptr;
    }
}

void InteractionComponent::render() {
    // Renderowanie komponentu interakcji
    // Można tutaj dodać wizualizację zasięgu interakcji w trybie debug
}

void InteractionComponent::initialize() {
    // Inicjalizacja komponentu
    m_lastInteractionTime = 0.0f;
    m_currentTarget = nullptr;
}

void InteractionComponent::shutdown() {
    // Czyszczenie komponentu
    m_currentTarget = nullptr;
    m_interactableTypes.clear();
}

std::type_index InteractionComponent::getComponentType() const {
    return std::type_index(typeid(InteractionComponent));
}

void InteractionComponent::addInteractableType(InteractionType type) {
    // Sprawdź czy typ już nie istnieje
    auto it = std::find(m_interactableTypes.begin(), m_interactableTypes.end(), type);
    if (it == m_interactableTypes.end()) {
        m_interactableTypes.push_back(type);
    }
}

void InteractionComponent::removeInteractableType(InteractionType type) {
    auto it = std::find(m_interactableTypes.begin(), m_interactableTypes.end(), type);
    if (it != m_interactableTypes.end()) {
        m_interactableTypes.erase(it);
    }
}

bool InteractionComponent::canInteractWith(InteractionType type) const {
    if (!m_canInteract) {
        return false;
    }

    auto it = std::find(m_interactableTypes.begin(), m_interactableTypes.end(), type);
    return it != m_interactableTypes.end();
}

bool InteractionComponent::isOnCooldown(float currentTime, float cooldown) const {
    return (currentTime - m_lastInteractionTime) < cooldown;
}

InteractionResult InteractionComponent::performInteraction(GameEntity* player) {
    if (!m_canInteract || !m_currentTarget) {
        return InteractionResult(false, "Brak celu interakcji", 0.0f, false);
    }

    // Sprawdź czy można wchodzić w interakcję z tym typem obiektu
    if (!canInteractWith(m_currentTarget->getInteractionType())) {
        return InteractionResult(false, "Nie można wejść w interakcję z tym obiektem", 0.0f, false);
    }

    // Wykonaj interakcję
    InteractionResult result = m_currentTarget->interact(player);

    // Jeśli interakcja się powiodła, zaktualizuj czas ostatniej interakcji
    if (result.success) {
        m_lastInteractionTime = GetTime();
    }

    return result;
}
