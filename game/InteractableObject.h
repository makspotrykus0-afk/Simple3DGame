#pragma once

#include <string>
#include <vector>
#include <functional>
#include <raylib.h>

// Forward declarations
class GameEntity;

/**
 * @brief Typy interakcji z obiektami
 */
enum class InteractionType : uint32_t {
    RESOURCE_GATHERING = 0,  // Zbieranie zasobów
    BUILDING_CONSTRUCTION = 1, // Budowanie struktur
    ITEM_PICKUP = 2,         // Podnoszenie przedmiotów
    INSPECTION = 3,          // Inspekcja obiektu
    CONVERSATION = 4,        // Rozmowa z NPC
    CRAFTING = 5,            // Wytwarzanie
    CONTAINER = 6,           // Otwieranie kontenera
    DOOR = 7,                // Otwieranie/zamykanie drzwi
    SWITCH = 8,              // Przełącznik
    HUNTING = 9              // Polowanie
};

/**
 * @brief Wynik interakcji
 */
struct InteractionResult {
    bool success;                    // Czy interakcja się powiodła
    std::string message;             // Wiadomość zwrotna
    float cooldownTime;              // Czas cooldownu przed następną interakcją
    bool consumeAction;              // Czy interakcja zużywa akcję

    InteractionResult(bool s = false, const std::string& msg = "", float cd = 0.0f, bool consume = true)
        : success(s), message(msg), cooldownTime(cd), consumeAction(consume) {}
};

/**
 * @brief Informacje o interakcji wyświetlane graczowi
 */
struct InteractionInfo {
    std::string objectName;          // Nazwa obiektu
    std::string objectDescription;   // Opis obiektu
    InteractionType type;            // Typ interakcji
    Vector3 position;                // Pozycja obiektu
    float distance;                  // Odległość od gracza
    std::vector<std::string> availableActions; // Dostępne akcje
    bool canInteract;                // Czy można wchodzić w interakcję
    std::string interactionPrompt;   // Podpowiedź interakcji (np. "Naciśnij E aby zebrać")

    InteractionInfo()
        : type(InteractionType::INSPECTION)
        , position({0, 0, 0})
        , distance(0.0f)
        , canInteract(false) {}
};

/**
 * @brief Interfejs dla obiektów z którymi można wchodzić w interakcję
 */
class InteractableObject {
public:
    /**
     * @brief Wirtualny destruktor
     */
    virtual ~InteractableObject() = default;

    /**
     * @brief Pobiera typ interakcji
     * @return Typ interakcji
     */
    virtual InteractionType getInteractionType() const = 0;

    /**
     * @brief Pobiera zasięg interakcji
     * @return Zasięg w metrach
     */
    virtual float getInteractionRange() const = 0;

    /**
     * @brief Sprawdza czy gracz może wejść w interakcję
     * @param player Encja gracza
     * @return true jeśli interakcja jest możliwa
     */
    virtual bool canInteract(GameEntity* player) const = 0;

    /**
     * @brief Wykonuje interakcję
     * @param player Encja gracza
     * @return Wynik interakcji
     */
    virtual InteractionResult interact(GameEntity* player) = 0;

    /**
     * @brief Pobiera informacje o interakcji do wyświetlenia
     * @return Informacje o interakcji
     */
    virtual InteractionInfo getDisplayInfo() const = 0;

    /**
     * @brief Pobiera pozycję obiektu
     * @return Pozycja w świecie
     */
    virtual Vector3 getPosition() const = 0;

    /**
     * @brief Sprawdza czy obiekt jest aktywny
     * @return true jeśli aktywny
     */
    virtual bool isActive() const { return true; }

    /**
     * @brief Pobiera nazwę obiektu
     * @return Nazwa obiektu
     */
    virtual std::string getName() const = 0;

    /**
     * @brief Renderuje podpowiedź interakcji
     * @param camera Kamera gracza
     */
    virtual void renderInteractionPrompt(Camera /*camera*/) const {}

    /**
     * @brief Aktualizuje obiekt interaktywny
     * @param deltaTime Czas od ostatniej aktualizacji
     */
    virtual void update(float /*deltaTime*/) {}
    
    /**
     * @brief Pobiera bounding box obiektu (opcjonalnie)
     * Domyślnie zwraca pusty BoundingBox
     */
    virtual BoundingBox getBoundingBox() const { return BoundingBox{ {0,0,0}, {0,0,0} }; }
};

/**
 * @brief Bazowa implementacja obiektu interaktywnego
 */
class BaseInteractableObject : public InteractableObject {
public:
    /**
     * @brief Konstruktor
     * @param name Nazwa obiektu
     * @param type Typ interakcji
     * @param position Pozycja obiektu
     * @param range Zasięg interakcji
     */
    BaseInteractableObject(const std::string& name, InteractionType type,
                          Vector3 position, float range = 3.0f);

    /**
     * @brief Destruktor
     */
    virtual ~BaseInteractableObject() = default;

    // Implementacja interfejsu InteractableObject
    InteractionType getInteractionType() const override { return m_interactionType; }
    float getInteractionRange() const override { return m_interactionRange; }
    Vector3 getPosition() const override { return m_position; }
    std::string getName() const override { return m_name; }
    bool isActive() const override { return m_isActive; }

    /**
     * @brief Ustawia pozycję obiektu
     * @param position Nowa pozycja
     */
    void setPosition(const Vector3& position) { m_position = position; }

    /**
     * @brief Ustawia zasięg interakcji
     * @param range Nowy zasięg
     */
    void setInteractionRange(float range) { m_interactionRange = range; }

    /**
     * @brief Ustawia czy obiekt jest aktywny
     * @param active Flaga aktywności
     */
    void setActive(bool active) { m_isActive = active; }

    /**
     * @brief Ustawia nazwę obiektu
     * @param name Nowa nazwa
     */
    void setName(const std::string& name) { m_name = name; }

    /**
     * @brief Ustawia opis obiektu
     * @param description Nowy opis
     */
    void setDescription(const std::string& description) { m_description = description; }

    /**
     * @brief Pobiera opis obiektu
     * @return Opis
     */
    const std::string& getDescription() const { return m_description; }

    /**
     * @brief Sprawdza czy gracz może wejść w interakcję (bazowa implementacja)
     * @param player Encja gracza
     * @return true jeśli interakcja jest możliwa
     */
    bool canInteract(GameEntity* player) const override;

    /**
     * @brief Pobiera informacje o interakcji (bazowa implementacja)
     * @return Informacje o interakcji
     */
    InteractionInfo getDisplayInfo() const override;

    /**
     * @brief Renderuje podpowiedź interakcji
     * @param camera Kamera gracza
     */
    void renderInteractionPrompt(Camera camera) const override;

protected:
    /** Nazwa obiektu */
    std::string m_name;

    /** Opis obiektu */
    std::string m_description;

    /** Typ interakcji */
    InteractionType m_interactionType;

    /** Pozycja obiektu */
    Vector3 m_position;

    /** Zasięg interakcji */
    float m_interactionRange;

    /** Czy obiekt jest aktywny */
    bool m_isActive;

    /** Czas ostatniej interakcji */
    float m_lastInteractionTime;

    /** Cooldown interakcji */
    float m_interactionCooldown;
};

/**
 * @brief Obiekt zbieralny (zasoby, przedmioty)
 */
class CollectableObject : public BaseInteractableObject {
public:
    /**
     * @brief Konstruktor
     * @param name Nazwa obiektu
     * @param position Pozycja
     * @param resourceType Typ zasobu
     * @param amount Ilość zasobu
     */
    CollectableObject(const std::string& name, Vector3 position,
                     const std::string& resourceType, int amount);

    /**
     * @brief Wykonuje interakcję zbierania
     * @param player Encja gracza
     * @return Wynik interakcji
     */
    InteractionResult interact(GameEntity* player) override;

    /**
     * @brief Pobiera typ zasobu
     * @return Typ zasobu
     */
    const std::string& getResourceType() const { return m_resourceType; }

    /**
     * @brief Pobiera ilość zasobu
     * @return Ilość
     */
    int getAmount() const { return m_amount; }

    /**
     * @brief Ustawia ilość zasobu
     * @param amount Nowa ilość
     */
    void setAmount(int amount) { m_amount = amount; }

private:
    /** Typ zasobu */
    std::string m_resourceType;

    /** Ilość zasobu */
    int m_amount;
};

/**
 * @brief Obiekt kontenera (skrzynia, magazyn)
 */
class ContainerObject : public BaseInteractableObject {
public:
    /**
     * @brief Konstruktor
     * @param name Nazwa kontenera
     * @param position Pozycja
     * @param capacity Pojemność
     */
    ContainerObject(const std::string& name, Vector3 position, float capacity = 100.0f);

    /**
     * @brief Wykonuje interakcję otwierania kontenera
     * @param player Encja gracza
     * @return Wynik interakcji
     */
    InteractionResult interact(GameEntity* player) override;

    /**
     * @brief Sprawdza czy kontener jest otwarty
     * @return true jeśli otwarty
     */
    bool isOpen() const { return m_isOpen; }

    /**
     * @brief Otwiera kontener
     */
    void open() { m_isOpen = true; }

    /**
     * @brief Zamyka kontener
     */
    void close() { m_isOpen = false; }

    /**
     * @brief Pobiera pojemność kontenera
     * @return Pojemność
     */
    float getCapacity() const { return m_capacity; }

private:
    /** Czy kontener jest otwarty */
    bool m_isOpen;

    /** Pojemność kontenera */
    float m_capacity;
};

/**
 * @brief Obiekt przełącznika
 */
class SwitchObject : public BaseInteractableObject {
public:
    /**
     * @brief Konstruktor
     * @param name Nazwa przełącznika
     * @param position Pozycja
     * @param onActivate Funkcja wywoływana przy aktywacji
     */
    SwitchObject(const std::string& name, Vector3 position,
                std::function<void()> onActivate = nullptr);

    /**
     * @brief Wykonuje interakcję przełącznika
     * @param player Encja gracza
     * @return Wynik interakcji
     */
    InteractionResult interact(GameEntity* player) override;

    /**
     * @brief Sprawdza czy przełącznik jest włączony
     * @return true jeśli włączony
     */
    bool isOn() const { return m_isOn; }

    /**
     * @brief Ustawia funkcję aktywacji
     * @param onActivate Funkcja do wywołania
     */
    void setOnActivate(std::function<void()> onActivate) { m_onActivate = onActivate; }

private:
    /** Czy przełącznik jest włączony */
    bool m_isOn;

    /** Funkcja wywoływana przy aktywacji */
    std::function<void()> m_onActivate;
};
