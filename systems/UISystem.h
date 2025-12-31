
#pragma once

#ifndef RAYLIB_INCLUDED

#define RAYLIB_INCLUDED

#include "raylib.h"

#endif

#include "../core/GameEngine.h"

#include "ResourceSystem.h"

#include "FoodSystem.h"

#include "../game/Colony.h"

#include "../game/BuildingBlueprint.h"

#include "../systems/InventorySystem.h"

#include "../systems/BuildingSystem.h"

#include "../systems/CraftingSystem.h"

#include <memory>

#include <unordered_map>

#include <vector>

#include <string>

#include <functional>

#include <chrono>

#include <atomic>
class Colony;

class BuildingInstance;
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
enum class UIState : uint8_t {

NORMAL = 0,

HOVERED = 1,

PRESSED = 2,

DISABLED = 3,

FOCUSED = 4,

HIDDEN = 5

};
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
struct UIPosition {

float x, y;

float width, height;

int zIndex;

UIPosition() : x(0), y(0), width(100), height(100), zIndex(0) {}

UIPosition(float px, float py, float w, float h, int z = 0)

: x(px), y(py), width(w), height(h), zIndex(z) {}

};
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
struct UIClickedEvent {

std::string elementId;

std::string elementType;

float clickX;

float clickY;

std::string playerId;

std::chrono::high_resolution_clock::time_point clickTime;

};
struct UIStateChangedEvent {

std::string elementId;

UIState oldState;

UIState newState;

std::string playerId;

};
struct UIProgressUpdatedEvent {

std::string elementId;

float oldProgress;

float newProgress;

std::string playerId;

};
class UISystem : public IGameSystem {

public:

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
    
    bool isVisible() const { return style.isVisible && style.opacity > 0.0f; }
    bool isEnabled() const { return state != UIState::DISABLED; }
    Vector3 getScreenPosition() const;
};

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

class UIFactory {
public:
    static std::unordered_map<UIElementType, UIElementConfig> createBaseElementTypes() {
        std::unordered_map<UIElementType, UIElementConfig> elementTypes;
        
        {
            UIElementConfig button(UIElementType::BUTTON, "Przycisk", "Interaktywny przycisk");
            button.minWidth = 80;
            button.minHeight = 30;
            button.isFocusable = true;
            button.isDraggable = false;
            elementTypes.emplace(UIElementType::BUTTON, std::move(button));
        }
        
        {
            UIElementConfig label(UIElementType::LABEL, "Etykieta", "Tekst wyświetlający informacje");
            label.minWidth = 50;
            label.minHeight = 20;
            label.isFocusable = false;
            label.isDraggable = false;
            elementTypes.emplace(UIElementType::LABEL, std::move(label));
        }
        
        {
            UIElementConfig progressBar(UIElementType::PROGRESS_BAR, "Pasek postępu", "Wizualizacja postępu");
            progressBar.minWidth = 100;
            progressBar.minHeight = 20;
            progressBar.isFocusable = false;
            progressBar.isDraggable = false;
            elementTypes.emplace(UIElementType::PROGRESS_BAR, std::move(progressBar));
        }
        
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
    
    static UIElement createUIElement(UIElementType type, const std::string& playerId, const UIPosition& position);
};

class UILayoutManager {
public:
    enum class LayoutType {
        ABSOLUTE,
        FLOW_H,
        FLOW_V,
        GRID,
        FLEX
    };
    
    static void applyLayout(std::vector<UIElement*>& elements, LayoutType layoutType, const Vector2& containerSize);
    static void makeResponsive(std::vector<UIElement*>& elements, const Vector2& screenSize);
    
private:
    static void applyAbsoluteLayout(std::vector<UIElement*>& elements);
    static void applyFlowLayout(std::vector<UIElement*>& elements, bool horizontal);
    static void applyGridLayout(std::vector<UIElement*>& elements, const Vector2& containerSize);
};


public:

UISystem();

virtual ~UISystem() override;

void initialize() override;

void update(float deltaTime) override;

void render() override;

void shutdown() override;

std::string getName() const override { return m_name; }

int getPriority() const override { return 5; }

void setColony(Colony* colony) { m_colony = colony; }

bool registerUIElementType(const UIElementConfig& config);

std::string createUIElement(UIElementType type, const std::string& playerId, const UIPosition& position);

bool deleteUIElement(const std::string& elementId, const std::string& playerId);

UIElement* getUIElement(const std::string& elementId, const std::string& playerId) const;

std::vector<UIElement*> getPlayerUIElements(const std::string& playerId) const;

bool updateUIElementPosition(const std::string& elementId, const std::string& playerId, const UIPosition& position);

bool updateUIElementText(const std::string& elementId, const std::string& playerId, const std::string& text);

bool setUIElementState(const std::string& elementId, const std::string& playerId, UIState state);

bool addUIElementAnimation(const std::string& elementId, const std::string& playerId, const UIAnimation& animation);

bool clearUIElementAnimations(const std::string& elementId, const std::string& playerId);

bool createPlayerDashboard(const std::string& playerId);

void updatePlayerUI(const std::string& playerId);

bool handleUIClick(const std::string& elementId, const std::string& playerId, float clickX, float clickY);

struct UISystemStats {
    size_t totalUIElementTypes;
    size_t totalUIElements;
    size_t totalAnimations;
    size_t cacheHits;
    size_t cacheMisses;
    float averageRenderTime;
    size_t totalInteractions;
};

UISystemStats getStats() const;
void DrawResourceBar(int wood, int food, int stone, int screenWidth);
void DrawBottomPanel(bool isBuildingMode, bool cameraMode, int screenWidth, int screenHeight);
void DrawBuildingSelectionPanel(const std::vector<BuildingBlueprint*>& blueprints, const std::string& selectedId, int screenWidth, int screenHeight);
void DrawSelectionInfo(const std::vector<Settler*>& selectedSettlers, int screenWidth, int screenHeight);
void DrawSettlerStatsOverlay(const std::vector<Settler*>& settlers, Camera3D camera, int screenWidth, int screenHeight);
void DrawSettlersStatsPanel(const std::vector<Settler*>& settlers, int screenWidth, int screenHeight);
void DrawSettlerOverheadUI(const Settler& settler, const Camera& camera);
void ShowBuildingInfo(BuildingInstance* building, int screenWidth, int screenHeight);
bool HandleSelectionPanelClick(int screenWidth, int screenHeight);
std::string HandleBuildingSelectionPanelClick(const std::vector<BuildingBlueprint*>& blueprints, int screenWidth, int screenHeight);
void DrawCraftingPanel(int screenWidth, int screenHeight);
bool HandleCraftingPanelClick(int screenWidth, int screenHeight);
void SetCraftingPanelVisible(bool visible) { m_isCraftingPanelVisible = visible; }
bool IsCraftingPanelVisible() const { return m_isCraftingPanelVisible; }
void SelectBuilding(BuildingInstance* building) { m_selectedBuilding = building; }
void setSelectedBuilding(BuildingInstance* building) { m_selectedBuilding = building; }
BuildingInstance* getSelectedBuilding() const { return m_selectedBuilding; }
void DeselectBuilding() { m_selectedBuilding = nullptr; }
bool IsMouseOverUI();
void SetBuildingMode(bool active) { m_isBuildingMode = active; }


private:

void initializeBaseUIElementTypes();

std::string generateUIElementId();

void updateUIAnimations(double deltaTime);

void renderPlayerUI(const std::string& playerId);

void lazyLoadPlayerUI(const std::string& playerId);

void notifyUIEvent(void* event);

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

std::string m_name;

std::unordered_map<UIElementType, UIElementConfig> m_uiElementTypes;

std::unordered_map<std::string, std::unordered_map<std::string, UIElement>> m_playerUIElements;

std::unordered_map<std::string, std::vector<std::string>> m_playerElementIds;

std::unordered_map<std::string, std::vector<UIAnimation>> m_activeAnimations;

UICache m_cache;

mutable std::atomic<size_t> m_cacheHits;

mutable std::atomic<size_t> m_cacheMisses;

mutable std::atomic<size_t> m_totalRenderTime;

mutable std::atomic<size_t> m_interactionCount;

std::atomic<uint64_t> m_nextUIElementId;

int m_activeSelectionTab = 0;

int m_selectedInventorySlot = -1;

BuildingInstance* m_selectedBuilding = nullptr;

Colony* m_colony = nullptr;

class NeedsSystem* m_needsSystem = nullptr;

bool m_isBuildingMode = false;

bool m_isCraftingPanelVisible = false;
public:

void setNeedsSystem(class NeedsSystem* system) { m_needsSystem = system; }

};
