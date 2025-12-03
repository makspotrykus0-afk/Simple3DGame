#pragma once

#ifndef RAYLIB_INCLUDED
#define RAYLIB_INCLUDED
#include "raylib.h"
#endif

#include "../core/GameEngine.h"
#include "ResourceSystem.h"
#include "FoodSystem.h"
#include "../game/Colony.h" // Access to Settler
#include "../game/BuildingBlueprint.h" // Access to blueprints
#include "../systems/InventorySystem.h" // Added for item moving
#include "../systems/BuildingSystem.h" // Added for BuildingInstance
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <functional>
#include <chrono>
#include <atomic>

class Colony;
// Forward declaration of BuildingInstance
class BuildingInstance;

/**
 * @brief Typy elementów UI
 */
enum class UIElementType : uint32_t {
    BUTTON = 0,
    LABEL = 1,
    IMAGE = 2,
    PROGRESS_BAR = 3,
    TEXT_BOX = 4,
    CONTAINER = 5,
    PANEL = 6,
    LIST = 7,
    TREE = 8,
    CUSTOM_START = 1000
};

/**
 * @brief Stany elementów UI
 */
enum class UIState : uint8_t {
    NORMAL = 0,
    HOVERED = 1,
    PRESSED = 2,
    DISABLED = 3,
    FOCUSED = 4,
    HIDDEN = 5
};

/**
 * @brief Typy animacji UI
 */
enum class UIAnimationType : uint32_t {
    FADE_IN = 0,
    FADE_OUT = 1,
    SLIDE_LEFT = 2,
    SLIDE_RIGHT = 3,
    SLIDE_UP = 4,
    SLIDE_DOWN = 5,
    SCALE = 6,
    ROTATE = 7,
    BOUNCE = 8,
    ELASTIC = 9
};

/**
 * @brief Struktura pozycji w UI
 */
struct UIPosition {
    float x, y;
    float width, height;
    int zIndex;  // Kolejność renderowania
    
    UIPosition() : x(0), y(0), width(100), height(100), zIndex(0) {}
    UIPosition(float px, float py, float w, float h, int z = 0)
        : x(px), y(py), width(w), height(h), zIndex(z) {}
};

/**
 * @brief Struktura stylu elementu UI
 */
struct UIStyle {
    std::string backgroundColor;
    std::string borderColor;
    std::string textColor;
    std::string hoverColor;
    std::string pressedColor;
    std::string disabledColor;
    float borderWidth;
    float cornerRadius;
    float opacity;
    std::string fontName;
    int fontSize;
    bool isVisible;
    
    UIStyle() : backgroundColor("#FFFFFF"), borderColor("#000000"), textColor("#000000"),
                hoverColor("#E0E0E0"), pressedColor("#C0C0C0"), disabledColor("#808080"),
                borderWidth(1.0f), cornerRadius(4.0f), opacity(1.0f),
                fontName("Arial"), fontSize(12), isVisible(true) {}
};

/**
 * @brief Zdarzenie kliknięcia w element UI
 */
struct UIClickedEvent {
    std::string elementId;
    std::string elementType;
    float clickX;
    float clickY;
    std::string playerId;
    std::chrono::high_resolution_clock::time_point clickTime;
};

/**
 * @brief Zdarzenie zmiany stanu elementu UI
 */
struct UIStateChangedEvent {
    std::string elementId;
    UIState oldState;
    UIState newState;
    std::string playerId;
};

/**
 * @brief Zdarzenie aktualizacji paska postępu
 */
struct UIProgressUpdatedEvent {
    std::string elementId;
    float oldProgress;
    float newProgress;
    std::string playerId;
};

/**
 * @brief Responsywny system UI implementujący lazy loading i event caching
 */
class UISystem : public IGameSystem {
public:
    /**
     * @brief Konfiguracja typu elementu UI
     */
    struct UIElementConfig {
        UIElementType type;
        std::string name;
        std::string description;
        std::string defaultStyle;
        float minWidth;
        float minHeight;
        float maxWidth;
        float maxHeight;
        bool isResizable;
        bool isDraggable;
        bool isFocusable;
        bool requiresPlayerId;

        UIElementConfig() : type(UIElementType::BUTTON), name(""), description(""), defaultStyle("default"),
              minWidth(10), minHeight(10), maxWidth(1920), maxHeight(1080),
              isResizable(false), isDraggable(false), isFocusable(true),
              requiresPlayerId(true) {}
        
        UIElementConfig(UIElementType t, const std::string& n, const std::string& desc = "")
            : type(t), name(n), description(desc), defaultStyle("default"),
              minWidth(10), minHeight(10), maxWidth(1920), maxHeight(1080),
              isResizable(false), isDraggable(false), isFocusable(true),
              requiresPlayerId(true) {}
    };

    /**
     * @brief Instancja elementu UI
     */
    struct UIElement {
        std::string id;
        UIElementType type;
        UIState state;
        UIPosition position;
        UIStyle style;
        std::string text;
        std::string texturePath;
        std::unordered_map<std::string, std::string> properties;
        std::chrono::high_resolution_clock::time_point lastInteraction;
        std::function<void()> onClick;
        std::function<void()> onHover;
        std::function<void()> onFocus;
        
        UIElement() : state(UIState::NORMAL) {}
        
        /**
         * @brief Sprawdza czy element jest widoczny
         * @return true jeśli element jest widoczny
         */
        bool isVisible() const { return style.isVisible && style.opacity > 0.0f; }
        
        /**
         * @brief Sprawdza czy element jest włączony
         * @return true jeśli element jest włączony
         */
        bool isEnabled() const { return state != UIState::DISABLED; }
        
        /**
         * @brief Pobiera pozycję w przestrzeni ekranu
         * @return Pozycja ekranowa
         */
        Vector3 getScreenPosition() const;
    };

    /**
     * @brief Animacja elementu UI
     */
    struct UIAnimation {
        UIAnimationType type;
        float duration;
        float startTime;
        float progress;
        std::function<void(float)> easingFunction;
        
        UIAnimation(UIAnimationType t, float dur = 1.0f) 
            : type(t), duration(dur), startTime(0), progress(0),
              easingFunction([](float p) { return p; }) {}
    };

    /**
     * @brief Factory dla elementów UI
     */
    class UIFactory {
    public:
        /**
         * @brief Tworzy podstawowe konfiguracje elementów UI
         * @return Mapa konfiguracji elementów UI
         */
        static std::unordered_map<UIElementType, UIElementConfig> createBaseElementTypes() {
            std::unordered_map<UIElementType, UIElementConfig> elementTypes;
            
            // Przycisk
            {
                UIElementConfig button(UIElementType::BUTTON, "Przycisk", "Interaktywny przycisk");
                button.minWidth = 80;
                button.minHeight = 30;
                button.isFocusable = true;
                button.isDraggable = false;
                elementTypes.emplace(UIElementType::BUTTON, std::move(button));
            }
            
            // Etykieta
            {
                UIElementConfig label(UIElementType::LABEL, "Etykieta", "Tekst wyświetlający informacje");
                label.minWidth = 50;
                label.minHeight = 20;
                label.isFocusable = false;
                label.isDraggable = false;
                elementTypes.emplace(UIElementType::LABEL, std::move(label));
            }
            
            // Pasek postępu
            {
                UIElementConfig progressBar(UIElementType::PROGRESS_BAR, "Pasek postępu", "Wizualizacja postępu");
                progressBar.minWidth = 100;
                progressBar.minHeight = 20;
                progressBar.isFocusable = false;
                progressBar.isDraggable = false;
                elementTypes.emplace(UIElementType::PROGRESS_BAR, std::move(progressBar));
            }
            
            // Kontener
            {
                UIElementConfig container(UIElementType::CONTAINER, "Kontener", "Pojemnik na inne elementy");
                container.minWidth = 50;
                container.minHeight = 50;
                container.isResizable = true;
                container.isDraggable = true;
                container.isFocusable = true;
                elementTypes.emplace(UIElementType::CONTAINER, std::move(container));
            }
            
            return elementTypes;
        }
        
        /**
         * @brief Tworzy gotowy element UI
         * @param type Typ elementu
         * @param playerId ID gracza
         * @param position Pozycja elementu
         * @return Nowa instancja elementu UI
         */
        static UIElement createUIElement(UIElementType type, const std::string& playerId, const UIPosition& position);
    };

    /**
     * @brief System layoutów dla responsywności
     */
    class UILayoutManager {
    public:
        /**
         * @brief Typy layoutów
         */
        enum class LayoutType {
            ABSOLUTE,    // Pozycjonowanie bezwzględne
            FLOW_H,      // Przepływ poziomy
            FLOW_V,      // Przepływ pionowy
            GRID,        // Siatka
            FLEX         // Flexbox
        };
        
        /**
         * @brief Oblicza pozycje elementów zgodnie z layoutem
         * @param elements Lista elementów do ułożenia
         * @param layoutType Typ layoutu
         * @param containerSize Rozmiar kontenera
         */
        static void applyLayout(std::vector<UIElement*>& elements, LayoutType layoutType, const Vector2& containerSize);
        
        /**
         * @brief Responsywność - dostosowuje layout do rozdzielczości
         * @param elements Elementy do dostosowania
         * @param screenSize Rozmiar ekranu
         */
        static void makeResponsive(std::vector<UIElement*>& elements, const Vector2& screenSize);
        
    private:
        static void applyAbsoluteLayout(std::vector<UIElement*>& elements);
        static void applyFlowLayout(std::vector<UIElement*>& elements, bool horizontal);
        static void applyGridLayout(std::vector<UIElement*>& elements, const Vector2& containerSize);
    };

public:
    /**
     * @brief Konstruktor
     */
    UISystem();

    /**
     * @brief Destruktor
     */
    virtual ~UISystem() override;

    // IGameSystem interface
    void initialize() override;
    void update(float deltaTime) override;
    void render() override;
    void shutdown() override;
    std::string getName() const override { return m_name; }
    int getPriority() const override { return 5; }

    // --- Colony Integration ---
    void setColony(Colony* colony) { m_colony = colony; }

    /**
     * @brief Rejestruje nowy typ elementu UI
     * @param config Konfiguracja elementu
     * @return true jeśli rejestracja się powiodła
     */
    bool registerUIElementType(const UIElementConfig& config);

    /**
     * @brief Tworzy nowy element UI
     * @param type Typ elementu
     * @param playerId ID gracza
     * @param position Pozycja elementu
     * @return ID nowego elementu lub pusty string przy błędzie
     */
    std::string createUIElement(UIElementType type, const std::string& playerId, const UIPosition& position);

    /**
     * @brief Usuwa element UI
     * @param elementId ID elementu
     * @param playerId ID gracza
     * @return true jeśli usunięcie się powiodło
     */
    bool deleteUIElement(const std::string& elementId, const std::string& playerId);

    /**
     * @brief Pobiera element UI
     * @param elementId ID elementu
     * @param playerId ID gracza
     * @return Wskaźnik do elementu lub nullptr
     */
    UIElement* getUIElement(const std::string& elementId, const std::string& playerId) const;

    /**
     * @brief Pobiera wszystkie elementy gracza
     * @param playerId ID gracza
     * @return Wektor wskaźników do elementów
     */
    std::vector<UIElement*> getPlayerUIElements(const std::string& playerId) const;

    /**
     * @brief Aktualizuje pozycję elementu
     * @param elementId ID elementu
     * @param playerId ID gracza
     * @param position Nowa pozycja
     * @return true jeśli aktualizacja się powiodła
     */
    bool updateUIElementPosition(const std::string& elementId, const std::string& playerId, const UIPosition& position);

    /**
     * @brief Aktualizuje tekst elementu
     * @param elementId ID elementu
     * @param playerId ID gracza
     * @param text Nowy tekst
     * @return true jeśli aktualizacja się powiodła
     */
    bool updateUIElementText(const std::string& elementId, const std::string& playerId, const std::string& text);

    /**
     * @brief Ustawia stan elementu
     * @param elementId ID elementu
     * @param playerId ID gracza
     * @param state Nowy stan
     * @return true jeśli aktualizacja się powiodła
     */
    bool setUIElementState(const std::string& elementId, const std::string& playerId, UIState state);

    /**
     * @brief Dodaje animację do elementu
     * @param elementId ID elementu
     * @param playerId ID gracza
     * @param animation Animacja do dodania
     * @return true jeśli dodanie się powiodło
     */
    bool addUIElementAnimation(const std::string& elementId, const std::string& playerId, const UIAnimation& animation);

    /**
     * @brief Usuwa wszystkie animacje elementu
     * @param elementId ID elementu
     * @param playerId ID gracza
     * @return true jeśli usunięcie się powiodło
     */
    bool clearUIElementAnimations(const std::string& elementId, const std::string& playerId);

    /**
     * @brief Tworzy responsywny dashboard gracza
     * @param playerId ID gracza
     * @return true jeśli dashboard został utworzony
     */
    bool createPlayerDashboard(const std::string& playerId);

    /**
     * @brief Aktualizuje UI gracza na podstawie stanu gry
     * @param playerId ID gracza
     */
    void updatePlayerUI(const std::string& playerId);

    /**
     * @brief Obsługuje kliknięcie w element
     * @param elementId ID elementu
     * @param playerId ID gracza
     * @param clickX Pozycja kliknięcia X
     * @param clickY Pozycja kliknięcia Y
     * @return true jeśli obsługa się powiodła
     */
    bool handleUIClick(const std::string& elementId, const std::string& playerId, float clickX, float clickY);

    /**
     * @brief Pobiera statystyki systemu UI
     */
    struct UISystemStats {
        size_t totalUIElementTypes;
        size_t totalUIElements;
        size_t totalAnimations;
        size_t cacheHits;
        size_t cacheMisses;
        float averageRenderTime;
        size_t totalInteractions;
    };

    /**
     * @brief Pobiera statystyki systemu
     * @return Statystyki
     */
    UISystemStats getStats() const;

    // ==========================================
    // IMMEDIATE MODE GUI HELPERS
    // ==========================================
    
    /**
     * @brief Draws the top resource bar
     * @param wood Amount of wood
     * @param food Amount of food
     * @param stone Amount of stone (mock for now)
     * @param screenWidth Width of the screen
     */
    void DrawResourceBar(int wood, int food, int stone, int screenWidth);

    /**
     * @brief Draws the bottom action panel with buttons and info
     * @param isBuildingMode Current state of build mode
     * @param cameraMode Current camera mode
     * @param screenWidth Width of screen
     * @param screenHeight Height of screen
     */
    void DrawBottomPanel(bool isBuildingMode, bool cameraMode, int screenWidth, int screenHeight);

    /**
     * @brief Draws building selection cards when in build mode
     * @param blueprints List of available blueprints
     * @param selectedId ID of currently selected blueprint
     * @param screenWidth Width of screen
     * @param screenHeight Height of screen
     */
    void DrawBuildingSelectionPanel(const std::vector<BuildingBlueprint*>& blueprints, const std::string& selectedId, int screenWidth, int screenHeight);

    /**
     * @brief Draws information about selected units
     * @param selectedSettlers List of selected settlers
     * @param screenWidth Width of screen
     * @param screenHeight Height of screen
     */
    void DrawSelectionInfo(const std::vector<Settler*>& selectedSettlers, int screenWidth, int screenHeight);

    /**
     * @brief Draws statistics overlay for settlers (Health, Energy)
     * @param settlers List of all settlers
     * @param camera Current camera for world-to-screen projection
     * @param screenWidth Width of screen
     * @param screenHeight Height of screen
     */
    void DrawSettlerStatsOverlay(const std::vector<Settler*>& settlers, Camera3D camera, int screenWidth, int screenHeight);
    
    /**
     * @brief Displays information about selected building
     * @param building Pointer to the selected building
     * @param screenWidth Width of screen
     * @param screenHeight Height of screen
     */
    void ShowBuildingInfo(BuildingInstance* building, int screenWidth, int screenHeight);

    // === NEW INPUT HANDLING ===
    /**
     * @brief Checks if a click interacts with the Selection Panel (Tabs)
     * @param screenWidth Width of screen
     * @param screenHeight Height of screen
     * @return true if interaction handled
     */
    bool HandleSelectionPanelClick(int screenWidth, int screenHeight);
    
    // --- Building Selection ---
    void SelectBuilding(BuildingInstance* building) { m_selectedBuilding = building; }
    void setSelectedBuilding(BuildingInstance* building) { m_selectedBuilding = building; }
    BuildingInstance* getSelectedBuilding() const { return m_selectedBuilding; }
    
    // --- Added DeselectBuilding for compatibility ---
    void DeselectBuilding() { m_selectedBuilding = nullptr; }

    // --- Input State ---
    /**
     * @brief Checks if mouse is over any UI element
     * @return true if mouse is over UI
     */
    bool IsMouseOverUI();

private:
    /**
     * @brief Inicjalizuje podstawowe typy elementów UI
     */
    void initializeBaseUIElementTypes();

    /**
     * @brief Generuje unikalne ID elementu UI
     * @return Nowe ID elementu
     */
    std::string generateUIElementId();

    /**
     * @brief Aktualizuje wszystkie animacje UI
     * @param deltaTime Czas od ostatniej aktualizacji
     */
    void updateUIAnimations(double deltaTime);

    /**
     * @brief Renderuje wszystkie elementy UI
     * @param playerId ID gracza
     */
    void renderPlayerUI(const std::string& playerId);

    /**
     * @brief Lazy loading dla elementów UI
     * @param playerId ID gracza
     */
    void lazyLoadPlayerUI(const std::string& playerId);

    /**
     * @brief Powiadamia o zdarzeniu UI
     * @param event Zdarzenie do wysłania
     */
    void notifyUIEvent(void* event);

    /**
     * @brief Cache dla często używanych elementów
     */
    struct UICache {
        std::unordered_map<std::string, std::vector<std::string>> playerElements;
        std::unordered_map<std::string, UIElement> cachedElements;
        std::chrono::high_resolution_clock::time_point lastCleanup;
        
        UICache() : lastCleanup(std::chrono::high_resolution_clock::now()) {}
        
        void addPlayerElements(const std::string& playerId, const std::vector<std::string>& elements) {
            playerElements[playerId] = elements;
        }
        
        bool getPlayerElements(const std::string& playerId, std::vector<std::string>& elements) const {
            auto it = playerElements.find(playerId);
            if (it != playerElements.end()) {
                elements = it->second;
                return true;
            }
            return false;
        }
        
        void cleanup() {
            auto now = std::chrono::high_resolution_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - lastCleanup).count() > 60) {
                playerElements.clear();
                cachedElements.clear();
                lastCleanup = now;
            }
        }
    };

private:
    /** Nazwa systemu */
    std::string m_name;
    
    /** Mapa konfiguracji typów elementów UI */
    std::unordered_map<UIElementType, UIElementConfig> m_uiElementTypes;
    
    /** Mapa elementów UI graczy */
    std::unordered_map<std::string, std::unordered_map<std::string, UIElement>> m_playerUIElements;
    
    /** Mapa graczy do ich elementów UI */
    std::unordered_map<std::string, std::vector<std::string>> m_playerElementIds;
    
    /** Mapa aktywnych animacji */
    std::unordered_map<std::string, std::vector<UIAnimation>> m_activeAnimations;
    
    /** Cache dla optymalizacji */
    UICache m_cache;
    
    /** Statystyki cache */
    mutable std::atomic<size_t> m_cacheHits;
    mutable std::atomic<size_t> m_cacheMisses;
    
    /** Statystyki renderowania */
    mutable std::atomic<size_t> m_totalRenderTime;
    mutable std::atomic<size_t> m_interactionCount;
    
    /** Generator ID dla elementów UI */
    std::atomic<uint64_t> m_nextUIElementId;

    // UI STATE
    int m_activeSelectionTab = 0; // 0: Stats, 1: Skills, 2: Inventory
    int m_selectedInventorySlot = -1; // Selected slot index (-1 for none)
    BuildingInstance* m_selectedBuilding = nullptr; // Currently selected building

    // Reference to Colony for accessing units
    Colony* m_colony = nullptr;
};