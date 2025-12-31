#include "InteractableObject.h"
#include "../entities/GameEntity.h"
#include "../components/ResourceComponent.h"
#include "raymath.h"
#include <cmath>

// ============================================================================
// BaseInteractableObject - Bazowa implementacja
// ============================================================================

BaseInteractableObject::BaseInteractableObject(const std::string& name, InteractionType type,
                                               Vector3 position, float range)
    : m_name(name)
    , m_description("")
    , m_interactionType(type)
    , m_position(position)
    , m_interactionRange(range)
    , m_isActive(true)
    , m_lastInteractionTime(0.0f)
    , m_interactionCooldown(0.5f)
{
}

bool BaseInteractableObject::canInteract(GameEntity* player) const {
    if (!player || !m_isActive) {
        return false;
    }

    // Sprawdź odległość od gracza
    // TODO: Pobierz pozycję gracza z komponentu pozycji
    // Na razie zakładamy że gracz jest w zasięgu
    
    return true;
}

InteractionInfo BaseInteractableObject::getDisplayInfo() const {
    InteractionInfo info;
    info.objectName = m_name;
    info.objectDescription = m_description;
    info.type = m_interactionType;
    info.position = m_position;
    info.distance = 0.0f; // TODO: Oblicz rzeczywistą odległość
    info.canInteract = m_isActive;
    
    // Ustaw podpowiedź w zależności od typu interakcji
    switch (m_interactionType) {
        case InteractionType::RESOURCE_GATHERING:
            info.interactionPrompt = "Naciśnij E aby zebrać";
            info.availableActions.push_back("Zbierz");
            break;
        case InteractionType::ITEM_PICKUP:
            info.interactionPrompt = "Naciśnij E aby podnieść";
            info.availableActions.push_back("Podnieś");
            break;
        case InteractionType::INSPECTION:
            info.interactionPrompt = "Naciśnij E aby zbadać";
            info.availableActions.push_back("Zbadaj");
            break;
        case InteractionType::CONTAINER:
            info.interactionPrompt = "Naciśnij E aby otworzyć";
            info.availableActions.push_back("Otwórz");
            break;
        case InteractionType::BUILDING_CONSTRUCTION:
            info.interactionPrompt = "Naciśnij E aby budować";
            info.availableActions.push_back("Buduj");
            break;
        case InteractionType::CRAFTING:
            info.interactionPrompt = "Naciśnij E aby wytwarzać";
            info.availableActions.push_back("Wytwarzaj");
            break;
        case InteractionType::SWITCH:
            info.interactionPrompt = "Naciśnij E aby przełączyć";
            info.availableActions.push_back("Przełącz");
            break;
        default:
            info.interactionPrompt = "Naciśnij E aby wejść w interakcję";
            info.availableActions.push_back("Interakcja");
            break;
    }
    
    return info;
}

void BaseInteractableObject::renderInteractionPrompt(Camera camera) const {
    if (!m_isActive) {
        return;
    }

    // Konwertuj pozycję 3D na 2D ekranu
    Vector2 screenPos = GetWorldToScreen(m_position, camera);
    
    // Rysuj podpowiedź nad obiektem
    const char* prompt = m_name.c_str();
    int textWidth = MeasureText(prompt, 20);
    
    // Tło dla tekstu
    DrawRectangle(
        static_cast<int>(screenPos.x) - textWidth / 2 - 5,
        static_cast<int>(screenPos.y) - 30,
        textWidth + 10,
        25,
        Fade(BLACK, 0.7f)
    );
    
    // Tekst
    DrawText(
        prompt,
        static_cast<int>(screenPos.x) - textWidth / 2,
        static_cast<int>(screenPos.y) - 25,
        20,
        WHITE
    );
}

// ============================================================================
// CollectableObject - Obiekt zbieralny
// ============================================================================

CollectableObject::CollectableObject(const std::string& name, Vector3 position,
                                     const std::string& resourceType, int amount)
    : BaseInteractableObject(name, InteractionType::RESOURCE_GATHERING, position, 3.0f)
    , m_resourceType(resourceType)
    , m_amount(amount)
{
    m_description = "Zasób: " + resourceType + " (x" + std::to_string(amount) + ")";
}

InteractionResult CollectableObject::interact(GameEntity* player) {
    if (!canInteract(player)) {
        return InteractionResult(false, "Nie można zebrać zasobu", 0.0f, false);
    }

    if (m_amount <= 0) {
        return InteractionResult(false, "Zasób został już zebrany", 0.0f, false);
    }

    // Dodaj zasób do gracza
    auto resourceComp = player->getComponent<ResourceComponent>();
    if (resourceComp) {
        int32_t added = resourceComp->addResource(m_resourceType, m_amount);
        
        if (added > 0) {
            m_amount -= added;
            
            // Jeśli zasób został całkowicie zebrany, dezaktywuj obiekt
            if (m_amount <= 0) {
                m_isActive = false;
            }
            
            std::string message = "Zebrano " + std::to_string(added) + "x " + m_resourceType;
            return InteractionResult(true, message, m_interactionCooldown, true);
        }
    }

    return InteractionResult(false, "Nie można dodać zasobu do ekwipunku", 0.0f, false);
}

// ============================================================================
// ContainerObject - Obiekt kontenera
// ============================================================================

ContainerObject::ContainerObject(const std::string& name, Vector3 position, float capacity)
    : BaseInteractableObject(name, InteractionType::CONTAINER, position, 2.0f)
    , m_isOpen(false)
    , m_capacity(capacity)
{
    m_description = "Kontener (pojemność: " + std::to_string(static_cast<int>(capacity)) + ")";
}

InteractionResult ContainerObject::interact(GameEntity* player) {
    if (!canInteract(player)) {
        return InteractionResult(false, "Nie można otworzyć kontenera", 0.0f, false);
    }

    // Przełącz stan otwarcia
    m_isOpen = !m_isOpen;
    
    std::string message = m_isOpen ? "Otwarto kontener" : "Zamknięto kontener";
    return InteractionResult(true, message, 0.2f, true);
}

// ============================================================================
// SwitchObject - Obiekt przełącznika
// ============================================================================

SwitchObject::SwitchObject(const std::string& name, Vector3 position,
                          std::function<void()> onActivate)
    : BaseInteractableObject(name, InteractionType::SWITCH, position, 2.0f)
    , m_isOn(false)
    , m_onActivate(onActivate)
{
    m_description = "Przełącznik";
}

InteractionResult SwitchObject::interact(GameEntity* player) {
    if (!canInteract(player)) {
        return InteractionResult(false, "Nie można użyć przełącznika", 0.0f, false);
    }

    // Przełącz stan
    m_isOn = !m_isOn;
    
    // Wywołaj funkcję aktywacji jeśli istnieje
    if (m_onActivate && m_isOn) {
        m_onActivate();
    }
    
    std::string message = m_isOn ? "Przełącznik włączony" : "Przełącznik wyłączony";
    return InteractionResult(true, message, 0.5f, true);
}
