#pragma once

#include "../core/IComponent.h"
#include "../game/InteractableObject.h"
#include <vector>
#include <memory>
#include <typeindex>

// Forward declarations
class GameEntity;

/**
 * @brief Komponent interakcji dla encji
 * Zarządza możliwością interakcji encji z obiektami w świecie
 */
class InteractionComponent : public IComponent {
public:
    /**
     * @brief Konstruktor
     */
    InteractionComponent();

    /**
     * @brief Destruktor
     */
    virtual ~InteractionComponent() = default;

    // IComponent interface
    void update(float deltaTime) override;
    void render() override;
    void initialize() override;
    void shutdown() override;
    std::type_index getComponentType() const override;

    /**
     * @brief Ustawia zasięg interakcji
     * @param range Zasięg w metrach
     */
    void setInteractionRange(float range) { m_interactionRange = range; }

    /**
     * @brief Pobiera zasięg interakcji
     * @return Zasięg w metrach
     */
    float getInteractionRange() const { return m_interactionRange; }

    /**
     * @brief Ustawia czy encja może wchodzić w interakcje
     * @param canInteract Flaga możliwości interakcji
     */
    void setCanInteract(bool canInteract) { m_canInteract = canInteract; }

    /**
     * @brief Sprawdza czy encja może wchodzić w interakcje
     * @return true jeśli może
     */
    bool getCanInteract() const { return m_canInteract; }

    /**
     * @brief Dodaje typ interakcji z którym encja może wchodzić w interakcję
     * @param type Typ interakcji
     */
    void addInteractableType(InteractionType type);

    /**
     * @brief Usuwa typ interakcji
     * @param type Typ interakcji
     */
    void removeInteractableType(InteractionType type);

    /**
     * @brief Sprawdza czy encja może wchodzić w interakcję z danym typem
     * @param type Typ interakcji
     * @return true jeśli może
     */
    bool canInteractWith(InteractionType type) const;

    /**
     * @brief Ustawia aktualny cel interakcji
     * @param target Obiekt docelowy
     */
    void setCurrentInteractionTarget(InteractableObject* target) { m_currentTarget = target; }

    /**
     * @brief Pobiera aktualny cel interakcji
     * @return Wskaźnik do obiektu lub nullptr
     */
    InteractableObject* getCurrentInteractionTarget() const { return m_currentTarget; }

    /**
     * @brief Ustawia czas ostatniej interakcji
     * @param time Czas w sekundach
     */
    void setLastInteractionTime(float time) { m_lastInteractionTime = time; }

    /**
     * @brief Pobiera czas ostatniej interakcji
     * @return Czas w sekundach
     */
    float getLastInteractionTime() const { return m_lastInteractionTime; }

    /**
     * @brief Sprawdza czy interakcja jest na cooldownie
     * @param currentTime Aktualny czas
     * @param cooldown Czas cooldownu
     * @return true jeśli na cooldownie
     */
    bool isOnCooldown(float currentTime, float cooldown = 0.5f) const;

    /**
     * @brief Wykonuje interakcję z aktualnym celem
     * @param player Encja wykonująca interakcję
     * @return Wynik interakcji
     */
    InteractionResult performInteraction(GameEntity* player);

    /**
     * @brief Czyści aktualny cel interakcji
     */
    void clearCurrentTarget() { m_currentTarget = nullptr; }

private:
    /** Zasięg interakcji */
    float m_interactionRange;

    /** Czy encja może wchodzić w interakcje */
    bool m_canInteract;

    /** Typy interakcji z którymi encja może wchodzić w interakcję */
    std::vector<InteractionType> m_interactableTypes;

    /** Aktualny cel interakcji */
    InteractableObject* m_currentTarget;

    /** Czas ostatniej interakcji */
    float m_lastInteractionTime;
};
