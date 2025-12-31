#pragma once

#include "../core/IGameSystem.h" // InteractionSystem dziedziczy po IGameSystem
#include "../game/InteractableObject.h"
#include <raylib.h>

#include <vector>
#include <memory>
#include <typeindex>

// Forward declarations
class GameEntity;
class Colony; // Forward declaration of Colony to allow usage in InteractionSystem
class BuildingSystem; // Forward declaration of BuildingSystem

/**
 * @brief Wynik raycastu
 */
struct RaycastHit {
    InteractableObject* hitObject;   // Trafiony obiekt
    Vector3 hitPosition;             // Pozycja trafienia
    float distance;                  // Odległość od źródła
    Vector3 surfaceNormal;           // Normalna powierzchni
    bool hit;                        // Czy coś zostało trafione

    RaycastHit()
        : hitObject(nullptr)
        , hitPosition({0, 0, 0})
        , distance(0.0f)
        , surfaceNormal({0, 1, 0})
        , hit(false)
    {}

    /**
     * @brief Pobiera trafiony obiekt
     * @return Wskaźnik do obiektu lub nullptr
     */
    InteractableObject* getHitObject() const { return hitObject; }

    /**
     * @brief Sprawdza czy raycast trafił
     * @return true jeśli trafił
     */
    bool isValid() const { return hit && hitObject != nullptr; }
};

/**
 * @brief System interakcji z obiektami
 * Zarządza wykrywaniem i wykonywaniem interakcji z obiektami w świecie
 */
class InteractionSystem : public IGameSystem { // Zmiana dziedziczenia na IGameSystem
public:
    /**
     * @brief Konstruktor
     */
    InteractionSystem();

    /**
     * @brief Destruktor
     */
    virtual ~InteractionSystem() = default;

    // IGameSystem interface
    void update(float deltaTime) override;
    void render() override;
    void initialize() override;
    void shutdown() override;

    // Implementacja brakujących metod wirtualnych z IGameSystem
    std::string getName() const override { return "InteractionSystem"; }
    int getPriority() const override { return 100; }

    // getSystemType nie istnieje w IGameSystem, usuwamy 'override'
    std::type_index getSystemType() const;

    // Setters for dependencies
    void setColony(Colony* colony) { m_colony = colony; }
    void setBuildingSystem(BuildingSystem* buildingSystem) { m_buildingSystem = buildingSystem; }

    /**
     * @brief Ustawia encję gracza
     * @param player Wskaźnik do encji gracza
     */
    void setPlayerEntity(GameEntity* player) { m_playerEntity = player; }

    /**
     * @brief Pobiera encję gracza
     * @return Wskaźnik do encji gracza
     */
    GameEntity* getPlayerEntity() const { return m_playerEntity; }

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
     * @brief Wykonuje raycast z kamery
     * @param camera Kamera gracza
     * @return Wynik raycastu
     */
    RaycastHit raycastFromCamera(Camera camera);

    /**
     * @brief Wykonuje raycast ze wszystkimi obiektami w zasięgu
     * @param camera Kamera gracza
     * @return Wektor wyników raycastu
     */
    std::vector<RaycastHit> raycastAllInRange(Camera camera);

    /**
     * @brief Przetwarza input gracza
     * @param camera Kamera gracza
     */
    void processPlayerInput(Camera camera);

    // REMOVED: Ambiguous inline alias
    // void handleInput(Camera camera) { processPlayerInput(camera); }

    /**
     * @brief Obsługuje wejście za pomocą promienia (dla Raycast z main.cpp)
     * @param ray Promień
     * @return true jeśli interakcja została obsłużona (np. podniesiono przedmiot)
     */
    bool handleInput(Ray ray);

    /**
     * @brief Obsługuje wybieranie jednostek za pomocą promienia (Left Click)
     * @param ray Promień
     * @return true jeśli wybrano osadnika
     */
    bool handleSelection(Ray ray);

    /**
     * @brief Pokazuje UI interakcji
     * @param target Obiekt docelowy
     */
    void showInteractionUI(InteractableObject* target);

    /**
     * @brief Ukrywa UI interakcji
     */
    void hideInteractionUI();

    /**
     * @brief Aktualizuje UI interakcji
     * @param info Informacje o interakcji
     */
    void updateInteractionUI(const InteractionInfo& info);

    /**
     * @brief Rejestruje obiekt interaktywny
     * @param object Obiekt do zarejestrowania
     */
    void registerInteractableObject(InteractableObject* object);

    /**
     * @brief Wyrejestrowuje obiekt interaktywny
     * @param object Obiekt do wyrejestrowania
     */
    void unregisterInteractableObject(InteractableObject* object);

    /**
     * @brief Pobiera wszystkie zarejestrowane obiekty
     * @return Wektor obiektów
     */
    const std::vector<InteractableObject*>& getInteractableObjects() const {
        return m_interactableObjects;
    }

    /**
     * @brief Pobiera aktualny cel interakcji
     * @return Wskaźnik do obiektu lub nullptr
     */
    InteractableObject* getCurrentTarget() const { return m_currentTarget; }

    // Accessors needed by other systems
    Colony* getColony() const { return m_colony; }
    BuildingSystem* getBuildingSystem() const { return m_buildingSystem; }

    /**
     * @brief Ustawia czy input jest włączony
     * @param enabled Flaga włączenia
     */
    void setInputEnabled(bool enabled) { m_inputEnabled = enabled; }

    /**
     * @brief Sprawdza czy input jest włączony
     * @return true jeśli włączony
     */
    bool isInputEnabled() const { return m_inputEnabled; }

    /**
     * @brief Ustawia czy UI jest widoczne
     * @param visible Flaga widoczności
     */
    void setUIVisible(bool visible) { m_uiVisible = visible; }

    /**
     * @brief Sprawdza czy UI jest widoczne
     * @return true jeśli widoczne
     */
    bool isUIVisible() const { return m_uiVisible; }

    /**
     * @brief Sprawdza czy interakcja została obsłużona w ostatniej klatce
     * Używane do blokowania innych akcji (np. ruchu) gdy nastąpiła interakcja
     * @return true jeśli interakcja wystąpiła
     */
    bool wasInteractionHandled() const { return m_wasInteractionHandled; }

    /**
     * @brief Czyści flagę obsłużonej interakcji
     */
    void clearInteractionHandled() { m_wasInteractionHandled = false; }

    void beginFrame(); // Deklaracja beginFrame

    /**
     * @brief Renderuje UI interakcji (2D)
     * @param camera Kamera gracza
     */
    void renderUI(Camera camera); // Deklaracja renderUI - publiczna, bo wołana z main

    /**
     * @brief Ustawia cel ruchu
     * @param target Pozycja celu
     */
    void setMovementTarget(Vector3 target) { m_movementTarget = target; }

    /**
     * @brief Pobiera cel ruchu
     * @return Pozycja celu
     */
    Vector3 getMovementTarget() const { return m_movementTarget; }

    /**
     * @brief Sprawdza czy postać się porusza
     * @return true jeśli postać się porusza
     */
    bool isMoving() const { return m_isMoving; }

    /**
     * @brief Ustawia czy postać się porusza
     * @param moving Flaga ruchu
     */
    void setMoving(bool moving) { m_isMoving = moving; }

    /**
     * @brief Ustawia prędkość ruchu
     * @param speed Prędkość ruchu
     */
    void setMovementSpeed(float speed) { m_movementSpeed = speed; }

    /**
     * @brief Pobiera prędkość ruchu
     * @return Prędkość ruchu
     */
    float getMovementSpeed() const { return m_movementSpeed; }

    /**
     * @brief Ustawia dystans aktywacji interakcji
     * @param distance Dystans aktywacji
     */
    void setActivationDistance(float distance) { m_activationDistance = distance; }

    /**
     * @brief Pobiera dystans aktywacji interakcji
     * @return Dystans aktywacji
     */
    float getActivationDistance() const { return m_activationDistance; }

private:
    /**
     * @brief Znajduje najbliższy obiekt interaktywny
     * @param position Pozycja gracza
     * @param direction Kierunek patrzenia
     * @return Wynik raycastu
     */
    RaycastHit findClosestInteractable(Vector3 position, Vector3 direction);

    /**
     * @brief Sprawdza kolizję raycastu z obiektem
     * @param rayOrigin Początek promienia
     * @param rayDirection Kierunek promienia
     * @param object Obiekt do sprawdzenia
     * @return Wynik raycastu
     */
    RaycastHit checkRayObjectCollision(Vector3 rayOrigin, Vector3 rayDirection,
                                       InteractableObject* object);

    /**
     * @brief Renderuje podpowiedź interakcji
     * @param camera Kamera gracza
     */
    void renderInteractionPrompt(Camera camera);

    /**
     * @brief Aktualizuje ruch do celu
     * @param deltaTime Czas od ostatniej klatki
     */
    void updateMovement(float deltaTime);

    /**
     * @brief Sprawdza czy postać dotarła do celu
     * @return true jeśli postać dotarła do celu
     */
    bool hasReachedTarget() const;

private:
    /** Encja gracza */
    GameEntity* m_playerEntity;

    Colony* m_colony = nullptr;
    BuildingSystem* m_buildingSystem = nullptr;

    /** Zasięg interakcji */
    float m_interactionRange;

    /** Zarejestrowane obiekty interaktywne */
    std::vector<InteractableObject*> m_interactableObjects;

    /** Aktualny cel interakcji */
    InteractableObject* m_currentTarget;

    /** Czy input jest włączony */
    bool m_inputEnabled;

    /** Czas ostatniej interakcji */
    float m_lastInteractionTime;

    /** Czy UI jest widoczne */
    bool m_uiVisible;

    /** Flaga czy interakcja została obsłużona w bieżącej klatce */
    bool m_wasInteractionHandled;

    /** Informacje o aktualnej interakcji */
    InteractionInfo m_currentInteractionInfo;

    // Zmienne do obsługi progresu ścinania
    float m_chopTimer;
    const float m_timeToChop = 2.0f;
    bool m_isChopping; // Nowa flaga: czy trwa proces ścinania
    InteractableObject* m_activeChopTarget; // Nowa zmienna: cel aktualnego ścinania

    // Zmienne do obsługi ruchu
    Vector3 m_movementTarget;
    bool m_isMoving;
    float m_movementSpeed;
    float m_activationDistance;
};
